/*
 * Copyright 2010-2022 JetBrains s.r.o. and Kotlin Programming Language contributors.
 * Use of this source code is governed by the Apache 2.0 license that can be found in the license/LICENSE.txt file.
 */

package org.jetbrains.kotlin.backend.konan

import org.jetbrains.kotlin.backend.konan.serialization.ClassFieldsSerializer
import org.jetbrains.kotlin.backend.konan.serialization.EagerInitializedPropertySerializer
import org.jetbrains.kotlin.backend.konan.serialization.InlineFunctionBodyReferenceSerializer
import org.jetbrains.kotlin.konan.file.File
import kotlin.random.Random

internal class CacheStorage(private val generationState: NativeGenerationState) {
    private val outputFiles = generationState.outputFiles

    companion object {
        fun renameOutput(outputFiles: OutputFiles) {
            if (outputFiles.mainFile.exists) return
            if (!outputFiles.tempCacheDirectory!!.renameTo(outputFiles.mainFile))
                outputFiles.tempCacheDirectory.deleteRecursively()
        }
    }

    fun saveAdditionalCacheInfo() {
        outputFiles.prepareTempDirectories()
        saveKlibContentsHash()
        saveCacheBitcodeDependencies()
        saveInlineFunctionBodies()
        saveClassFields()
        saveEagerInitializedProperties()
    }

    private fun saveKlibContentsHash() {
        outputFiles.hashFile!!.writeBytes(generationState.klibHash.toByteArray())
    }

    private fun saveCacheBitcodeDependencies() {
        outputFiles.bitcodeDependenciesFile!!.writeLines(
                DependenciesSerializer.serialize(generationState.dependenciesTracker.immediateBitcodeDependencies))
    }

    private fun saveInlineFunctionBodies() {
        outputFiles.inlineFunctionBodiesFile!!.writeBytes(
                InlineFunctionBodyReferenceSerializer.serialize(generationState.inlineFunctionBodies))
    }

    private fun saveClassFields() {
        outputFiles.classFieldsFile!!.writeBytes(
                ClassFieldsSerializer.serialize(generationState.classFields))
    }

    private fun saveEagerInitializedProperties() {
        outputFiles.eagerInitializedPropertiesFile!!.writeBytes(
                EagerInitializedPropertySerializer.serialize(generationState.eagerInitializedFiles))
    }
}
