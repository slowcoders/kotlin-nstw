/*
 * Copyright 2010-2019 JetBrains s.r.o. Use of this source code is governed by the Apache 2.0 license
 * that can be found in the license/LICENSE.txt file.
 */

package org.jetbrains.kotlin.gradle.plugin.mpp

import org.gradle.api.DefaultTask
import org.gradle.api.artifacts.component.ModuleComponentIdentifier
import org.gradle.api.artifacts.component.ProjectComponentIdentifier
import org.gradle.api.artifacts.result.ResolvedComponentResult
import org.gradle.api.artifacts.result.ResolvedDependencyResult
import org.gradle.api.file.FileCollection
import org.gradle.api.file.ProjectLayout
import org.gradle.api.file.RegularFile
import org.gradle.api.provider.ListProperty
import org.gradle.api.provider.MapProperty
import org.gradle.api.provider.Provider
import org.gradle.api.tasks.*
import org.gradle.work.DisableCachingByDefault
import org.jetbrains.kotlin.gradle.plugin.PropertiesProvider.Companion.kotlinPropertiesProvider
import org.jetbrains.kotlin.gradle.plugin.mpp.internal.MetadataJsonSerialisationTool
import org.jetbrains.kotlin.gradle.utils.LazyResolvedConfiguration
import java.io.File
import javax.inject.Inject

internal const val SOURCE_SET_METADATA = "source-set-metadata-locations.json"
internal const val MULTIPLATFORM_PROJECT_METADATA_FILE_NAME = "kotlin-project-structure-metadata.xml"
internal const val MULTIPLATFORM_PROJECT_METADATA_JSON_FILE_NAME = "kotlin-project-structure-metadata.json"
internal const val EMPTY_PROJECT_STRUCTURE_METADATA_FILE_NAME = "empty-kotlin-project-structure-metadata"

@DisableCachingByDefault
abstract class GenerateProjectStructureMetadata : DefaultTask() {

    @get:Inject
    abstract internal val projectLayout: ProjectLayout

    @get:Internal
    internal lateinit var lazyKotlinProjectStructureMetadata: Lazy<KotlinProjectStructureMetadata>

    @get:Nested
    internal val kotlinProjectStructureMetadata: KotlinProjectStructureMetadata
        get() = lazyKotlinProjectStructureMetadata.value

    /**
     * Map of Source Set dependencies, for project 2 project dependencies should resolve into [KotlinProjectCoordinatesData] file.
     */
    @get:Internal
    internal abstract val sourceSetDependencies: MapProperty<String, LazyResolvedConfiguration>

    /**
     * Gradle InputFiles view of [sourceSetDependencies]
     */
    @get:PathSensitive(PathSensitivity.RELATIVE)
    @get:InputFiles
    internal val sourceSetDependenciesInputFiles: List<FileCollection> get() = sourceSetDependencies.get().values.map { it.files }

    @get:OutputFile
    val resultFile: File
        get() = projectLayout.buildDirectory.file(
            "kotlinProjectStructureMetadata/$MULTIPLATFORM_PROJECT_METADATA_JSON_FILE_NAME"
        ).get().asFile

    @get:OutputFile
    internal val sourceSetMetadataOutputsFile: Provider<RegularFile> =
        project.layout.buildDirectory.file("internal/kmp/$SOURCE_SET_METADATA")

    private val kmpIsolatedProjectsSupport: Boolean = project.kotlinPropertiesProvider.kotlinKmpProjectIsolationEnabled

    @get:Nested
    internal abstract val sourceSetOutputs: ListProperty<SourceSetMetadataOutput>

    /**
     * @param projectCoordinatesConfiguration Should contain resolved configuration with [KotlinProjectCoordinatesData] in artifacts
     */
    private fun ResolvedDependencyResult.moduleDependencyIdentifier(
        projectCoordinatesConfiguration: LazyResolvedConfiguration
    ): ModuleDependencyIdentifier = when(selected.id) {
        is ProjectComponentIdentifier -> tryReadFromKotlinProjectCoordinatesData(projectCoordinatesConfiguration)
            ?: selected.moduleDependencyIdentifier()
        is ModuleComponentIdentifier -> selected.moduleDependencyIdentifier()
        else -> error("Unknown ComponentIdentifier: $selected")
    }

    private fun ResolvedDependencyResult.tryReadFromKotlinProjectCoordinatesData(
        projectCoordinatesConfiguration: LazyResolvedConfiguration
    ): ModuleDependencyIdentifier? {
        val projectCoordinatesFile = projectCoordinatesConfiguration
            .getArtifacts(this)
            .singleOrNull()
            ?: return null
        val projectCoordinates = parseKotlinProjectCoordinatesOrNull(projectCoordinatesFile.file) ?: return null
        return projectCoordinates.moduleId
    }

    private fun ResolvedComponentResult.moduleDependencyIdentifier() = ModuleDependencyIdentifier(
        groupId = moduleVersion?.group,
        moduleId = moduleVersion?.name ?: "unspecified".also { logger.warn("[Kotlin] ComponentResult $this has no name") }
    )

    @TaskAction
    fun generateMetadataXml() {
        resultFile.parentFile.mkdirs()

        val actualProjectStructureMetadata = if (kmpIsolatedProjectsSupport) {
            kotlinProjectStructureMetadata.copy(
                sourceSetModuleDependencies = sourceSetDependencies.get().mapValues { (_, resolvedConfiguration) ->
                    val directDependencies = resolvedConfiguration.root.dependencies
                    val result = mutableSetOf<ModuleDependencyIdentifier>()
                    for (dependency in directDependencies) {
                        if (dependency.isConstraint) continue
                        if (dependency !is ResolvedDependencyResult) continue
                        result.add(dependency.moduleDependencyIdentifier(resolvedConfiguration))
                    }

                    result
                }
            )
        } else {
            kotlinProjectStructureMetadata
        }

        val resultString = actualProjectStructureMetadata.toJson()
        resultFile.writeText(resultString)

        val metadataOutputsBySourceSet = sourceSetOutputs.get().associate { it.sourceSetName to it.metadataOutput.get() }
        val metadataOutputsJson = MetadataJsonSerialisationTool.toJson(metadataOutputsBySourceSet)
        sourceSetMetadataOutputsFile.get().asFile.writeText(metadataOutputsJson)
    }

    internal data class SourceSetMetadataOutput(
        @get:Input
        val sourceSetName: String,
        @get:Input
        @get:Optional
        val metadataOutput: Provider<File>,
    )
}

