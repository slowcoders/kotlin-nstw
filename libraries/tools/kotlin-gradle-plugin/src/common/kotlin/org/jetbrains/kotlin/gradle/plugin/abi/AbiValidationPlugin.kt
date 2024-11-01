/*
 * Copyright 2010-2024 JetBrains s.r.o. and Kotlin Programming Language contributors.
 * Use of this source code is governed by the Apache 2.0 license that can be found in the license/LICENSE.txt file.
 */

@file:Suppress("DEPRECATION")

package org.jetbrains.kotlin.gradle.plugin.abi

import org.gradle.api.NamedDomainObjectContainer
import org.gradle.api.Project
import org.gradle.api.artifacts.Configuration
import org.gradle.api.tasks.SourceSet
import org.jetbrains.kotlin.gradle.dsl.KotlinJvmCompilerOptions
import org.jetbrains.kotlin.gradle.dsl.KotlinJvmOptions
import org.jetbrains.kotlin.gradle.dsl.KotlinJvmProjectExtension
import org.jetbrains.kotlin.gradle.plugin.getKotlinPluginVersion
import org.jetbrains.kotlin.gradle.plugin.mpp.KotlinWithJavaCompilation
import org.jetbrains.kotlin.gradle.tasks.abi.KotlinAbiUpdateTask
import org.jetbrains.kotlin.gradle.tasks.abi.KotlinJvmAbiCheckTask
import org.jetbrains.kotlin.gradle.tasks.abi.KotlinJvmAbiDumpTask

private const val DEFAULT_REFERENCE_DIR_PATH = "abi"
private const val DEFAULT_REFERENCE_DUMP_NAME = "jvm.abi"
private const val ACTUAL_DUMP_PATH = "abi/jvm.abi"

internal fun Project.applyJvmAbiValidation(
    extension: KotlinJvmProjectExtension,
    compilations: NamedDomainObjectContainer<KotlinWithJavaCompilation<KotlinJvmOptions, KotlinJvmCompilerOptions>>,
) {
    val abiClasspath = prepareClasspath()

    val abiValidation = extension.abiValidation
    abiValidation.referenceDumpDir.convention(layout.projectDirectory.dir(DEFAULT_REFERENCE_DIR_PATH))
    abiValidation.referenceDumpFileName.convention(DEFAULT_REFERENCE_DUMP_NAME)
    abiValidation.useLegacyFormat.convention(false)

    val referenceDumpFileName = abiValidation.referenceDumpFileName
    val referenceDump = abiValidation.referenceDumpDir.map { dir -> dir.file(referenceDumpFileName.get()) }

    val dumpTaskProvider = tasks.register(KotlinJvmAbiDumpTask.KOTLIN_JVM_TASK_NAME, KotlinJvmAbiDumpTask::class.java) {
        it.dumpFile.convention(layout.buildDirectory.file(ACTUAL_DUMP_PATH))
        it.toolsClasspath.from(abiClasspath)
        it.legacyFormat.convention(abiValidation.useLegacyFormat)

        it.includedClasses.convention(abiValidation.filters.included.classes)
        it.includedAnnotatedWith.convention(abiValidation.filters.included.annotatedWith)
        it.excludedClasses.convention(abiValidation.filters.excluded.classes)
        it.excludedAnnotatedWith.convention(abiValidation.filters.excluded.annotatedWith)
    }

    tasks.register(KotlinJvmAbiCheckTask.KOTLIN_JVM_TASK_NAME, KotlinJvmAbiCheckTask::class.java) {
        it.actualDump.convention(dumpTaskProvider.map { t -> t.dumpFile.get() })
        it.referenceDump.convention(referenceDump)
        it.toolsClasspath.from(abiClasspath)
        it.updateTaskName.set(KotlinAbiUpdateTask.KOTLIN_JVM_TASK_NAME)
    }

    tasks.register(KotlinAbiUpdateTask.KOTLIN_JVM_TASK_NAME, KotlinAbiUpdateTask::class.java) {
        it.actualDump.convention(dumpTaskProvider.map { t -> t.dumpFile.get() })
        it.referenceDump.convention(referenceDump)
    }

    compilations.all { compilation ->
        // TODO additional source sets!
        if (compilation.name == SourceSet.MAIN_SOURCE_SET_NAME) {
            dumpTaskProvider.configure { dumpTask ->
                dumpTask.classfilesDirs.from(compilation.output.classesDirs)
            }
        }
    }
}

private fun Project.prepareClasspath(): Configuration {
    val version = getKotlinPluginVersion()
    val dependency = dependencies.create("org.jetbrains.kotlin:abi-tools:$version")
    return configurations.detachedConfiguration(dependency)
}
