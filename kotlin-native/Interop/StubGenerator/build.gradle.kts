/*
 * Copyright 2010-2022 JetBrains s.r.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

buildscript {
    apply(from = "$rootDir/kotlin-native/gradle/kotlinGradlePlugin.gradle")
}

plugins {
    kotlin("jvm")
    application
}

application {
    mainClass.set("org.jetbrains.kotlin.native.interop.gen.jvm.MainKt")
}

dependencies {
    implementation(project(":kotlin-native:Interop:Indexer"))
    implementation(project(":kotlin-native:utilities:basic-utils"))
    api(project(path = ":kotlin-native:endorsedLibraries:kotlinx.cli", configuration = "jvmRuntimeElements"))

    api(project(":kotlin-stdlib"))
    api(project(path = ":kotlin-compiler", configuration = "runtimeElements"))
    api(project(":kotlinx-metadata-klib"))
    api(project(":native:kotlin-native-utils"))
    implementation(project(":compiler:util"))
    implementation(project(":compiler:ir.serialization.common"))

    testImplementation(kotlin("test-junit"))
    testImplementation(project(":kotlin-test:kotlin-test-junit"))
}

tasks {
    withType<org.jetbrains.kotlin.gradle.tasks.KotlinCompile> {
        kotlinOptions {
            freeCompilerArgs = listOf(
                    "-opt-in=kotlin.ExperimentalUnsignedTypes",
                    "-Xskip-metadata-version-check"
            )
            allWarningsAsErrors = true
        }
    }
}

sourceSets {
    "main" {
        kotlin {
            srcDir(project.kotlinNativeVersionSrc())
            srcDir("../../shared/src/library/kotlin")
            srcDir("../../shared/src/main/kotlin")
        }
    }
}