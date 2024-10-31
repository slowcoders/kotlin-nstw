/*
 * Copyright 2010-2021 JetBrains s.r.o. and Kotlin Programming Language contributors.
 * Use of this source code is governed by the Apache 2.0 license that can be found in the license/LICENSE.txt file.
 */

package org.jetbrains.kotlin.gradle.testbase

import com.intellij.util.containers.isEmpty
import org.gradle.tooling.internal.consumer.ConnectorServices
import org.junit.jupiter.api.AfterEach
import java.nio.file.Files
import java.nio.file.Path
import java.util.concurrent.TimeUnit

@DaemonsGradlePluginTests
abstract class KGPDaemonsBaseTest : KGPBaseTest() {

    @AfterEach
    internal open fun tearDown() {
        // Stops Gradle and initiates Kotlin daemon termination, so new run will pick up new jvm arguments
        ConnectorServices.reset()
        awaitKotlinDaemonTermination(kotlinDaemonRunFilesDir)
    }

    private fun awaitKotlinDaemonTermination(directory: Path) {
        require(Files.isDirectory(directory)) { "$directory is not a directory." }
        val maxWaitSeconds = 10L
        val periodicCheckMs = 100L

        val timeoutMillis = TimeUnit.SECONDS.toMillis(maxWaitSeconds)
        val startTime = System.currentTimeMillis()

        while (true) {
            val runFiles = Files.list(directory).use { it.toList() }
            if (runFiles.isEmpty()) {
                return
            }
            if (System.currentTimeMillis() - startTime >= timeoutMillis) {
                error("Kotlin daemon has not been terminated in $maxWaitSeconds seconds. Remaining run files: \n${runFiles.joinToString("\n")}")
            }
            TimeUnit.MILLISECONDS.sleep(periodicCheckMs)
        }
    }
}