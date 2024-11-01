/*
 * Copyright 2010-2024 JetBrains s.r.o. and Kotlin Programming Language contributors.
 * Use of this source code is governed by the Apache 2.0 license that can be found in the license/LICENSE.txt file.
 */

package org.jetbrains.kotlin.gradle.tasks.abi

import org.gradle.api.file.ConfigurableFileCollection
import org.gradle.api.file.RegularFileProperty
import org.gradle.api.provider.Property
import org.gradle.api.provider.SetProperty
import org.gradle.api.tasks.*
import org.jetbrains.kotlin.abi.tools.api.AbiFilters
import org.jetbrains.kotlin.abi.tools.api.JvmAbiSuit
import org.jetbrains.kotlin.abi.tools.api.JvmAbiToolsInterface

internal abstract class KotlinJvmAbiDumpTask : AbiToolsTask(), KotlinAbiDumpTask {
    companion object {
        const val KOTLIN_JVM_TASK_NAME = "dumpAbi"
        const val KOTLIN_JVM_SUIT_NAME = "jvm"
    }

    @get:OutputFile
    abstract override val dumpFile: RegularFileProperty

    @get:InputFiles
    @get:Optional
    @get:PathSensitive(PathSensitivity.RELATIVE)
    abstract val classfilesDirs: ConfigurableFileCollection

    @get:Input
    abstract val legacyFormat: Property<Boolean>

    @get:Input
    @get:Optional
    abstract val includedClasses: SetProperty<String>

    @get:Input
    @get:Optional
    abstract val excludedClasses: SetProperty<String>

    @get:Input
    @get:Optional
    abstract val includedAnnotatedWith: SetProperty<String>

    @get:Input
    @get:Optional
    abstract val excludedAnnotatedWith: SetProperty<String>

    override fun runTools(tools: JvmAbiToolsInterface) {
        val classfiles = classfilesDirs.asFileTree
            .asSequence()
            .filter {
                !it.isDirectory && it.name.endsWith(".class") && !it.name.startsWith("META-INF/")
            }

        val jvmAbiSuit = JvmAbiSuit(KOTLIN_JVM_SUIT_NAME, classfiles)

        val filters = AbiFilters(
            includedClasses.getOrElse(emptySet()),
            excludedClasses.getOrElse(emptySet()),
            includedAnnotatedWith.getOrElse(emptySet()),
            excludedAnnotatedWith.getOrElse(emptySet())
        )

        if (legacyFormat.get()) {
            tools.dumpToLegacyFile(listOf(jvmAbiSuit), filters, dumpFile.get().asFile)
        } else {
            tools.dumpToV2File(listOf(jvmAbiSuit), filters, dumpFile.get().asFile)
        }
    }
}
