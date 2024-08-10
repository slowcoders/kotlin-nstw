import org.jetbrains.kotlin.gradle.tasks.KotlinCompile
import org.gradle.api.tasks.compile.JavaCompile
import org.jetbrains.kotlin.gradle.dsl.JvmTarget

plugins {
    kotlin("jvm")
    id("jps-compatible")
}

project.configureJvmToolchain(JdkMajorVersion.JDK_1_8)

dependencies {
    compileOnly(project(":core:util.runtime"))
    compileOnly(project(":core:descriptors"))
    compileOnly(project(":core:descriptors.jvm"))

    testApi(projectTests(":compiler:tests-common"))
    testApi(projectTests(":generators:test-generator"))

    testApi(intellijCore())
    testApi(platform(libs.junit.bom))
    testImplementation(libs.junit4)
}

sourceSets {
    "main" { projectDefault() }
    "test" { projectDefault() }
}

tasks.named<KotlinCompile>("compileTestKotlin").configure {
    kotlinJavaToolchain.toolchain.use(getToolchainLauncherFor(JdkMajorVersion.JDK_11_0))
    compilerOptions.jvmTarget = JvmTarget.JVM_11
}
tasks.named<JavaCompile>("compileTestJava").configure {
    configureTaskToolchain(JdkMajorVersion.JDK_11_0)
    targetCompatibility = "11"
}

val generateTests by generator("org.jetbrains.kotlin.generators.tests.GenerateRuntimeDescriptorTestsKt")

projectTest(parallel = true) {
    dependsOn(":dist")
    workingDir = rootDir
}

testsJar()