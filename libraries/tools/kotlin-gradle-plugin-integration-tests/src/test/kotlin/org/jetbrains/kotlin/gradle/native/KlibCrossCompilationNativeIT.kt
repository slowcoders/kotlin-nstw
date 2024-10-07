/*
 * Copyright 2010-2024 JetBrains s.r.o. and Kotlin Programming Language contributors.
 * Use of this source code is governed by the Apache 2.0 license that can be found in the license/LICENSE.txt file.
 */

package org.jetbrains.kotlin.gradle.native

import org.gradle.util.GradleVersion
import org.jetbrains.kotlin.gradle.testbase.*
import org.jetbrains.kotlin.test.KotlinTestUtils
import org.jetbrains.kotlin.test.TestMetadata
import org.junit.jupiter.api.Disabled
import org.junit.jupiter.api.DisplayName
import org.junit.jupiter.api.condition.OS
import org.junit.jupiter.api.io.TempDir
import java.nio.file.Path

@DisplayName("Tests for checking klib cross-compilation in KGP")
@NativeGradlePluginTests
class KlibCrossCompilationNativeIT : KGPBaseTest() {

    @GradleTest
    @TestMetadata("klibCrossCompilationDefaultSettings")
    @OsCondition(supportedOn = [OS.LINUX, OS.WINDOWS], enabledOnCI = [OS.LINUX, OS.WINDOWS])
    fun compileIosTargetOnNonDarwinHostWithDefaultSettings(gradleVersion: GradleVersion) {
        nativeProject("klibCrossCompilationDefaultSettings", gradleVersion) {
            build(":compileKotlinIosArm64") {
                KotlinTestUtils.assertEqualsToFile(projectPath.resolve("diagnostics.txt"), extractProjectsAndTheirDiagnostics())
                assertTasksSkipped(":compileKotlinIosArm64")
            }
        }
    }

    @GradleTest
    @TestMetadata("klibCrossCompilationWithGradlePropertyEnabled")
    @OsCondition(supportedOn = [OS.LINUX, OS.WINDOWS], enabledOnCI = [OS.LINUX, OS.WINDOWS])
    fun compileIosTargetOnNonDarwinHostWithGradlePropertyEnabled(gradleVersion: GradleVersion, @TempDir konanDataDir: Path) {
        nativeProject(
            "klibCrossCompilationWithGradlePropertyEnabled",
            gradleVersion,
            buildOptions = defaultBuildOptions.copy(
                // This line is required for the custom konan home location, to check that it is downloaded,
                // even when target is not supported(@see KT-72068). Even without this line the test will not fail,
                // but please don't remove while `kotlin.native.enableKlibsCrossCompilation` flag exists.
                konanDataDir = konanDataDir
            )
        ) {
            build(":compileKotlinIosArm64") {
                KotlinTestUtils.assertEqualsToFile(
                    projectPath.resolve("diagnostics-compileKotlinIosArm64.txt"), extractProjectsAndTheirDiagnostics()
                )
                assertTasksExecuted(":compileKotlinIosArm64")
            }

            build(":linkIosArm64") {
                KotlinTestUtils.assertEqualsToFile(
                    projectPath.resolve("diagnostics-linkIosArm64.txt"), extractProjectsAndTheirDiagnostics()
                )
                // Do not assert :linkIosArm64, because it's a plain umbrella-like `org.gradle.DefaultTask` instance,
                // and it doesn't get disabled even on linuxes (see [KotlinNativeConfigureBinariesSideEffect])
                assertTasksSkipped(":linkDebugTestIosArm64")
                assertTasksSkipped(":linkReleaseExecutableIosArm64")
                assertTasksSkipped(":linkDebugExecutableIosArm64")
            }
        }
    }
}
