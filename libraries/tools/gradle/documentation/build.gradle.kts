plugins {
    id("gradle-plugins-documentation")
}

pluginsApiDocumentation {
    documentationOutput = layout.buildDirectory.dir("documentation/kotlinlang")
    documentationOldVersions = layout.buildDirectory.dir("documentation/kotlinlangOld")
    templatesArchiveUrl = "https://github.com/JetBrains/kotlin-web-site/archive/refs/heads/master.zip"
    templatesArchiveSubDirectoryPattern = "kotlin-web-site-master/dokka-templates/**"
    templatesArchivePrefixToRemove = "kotlin-web-site-master/dokka-templates/"

    moduleDescription.set(layout.projectDirectory.file("module-description.md"))
    addGradlePluginProject(project(":kotlin-gradle-plugin-api"))
    addGradlePluginProject(project(":compose-compiler-gradle-plugin"))
}

configurations.all {
    resolutionStrategy.eachDependency {
        if (requested.group == "org.apache.commons" && requested.name == "commons-lang3") {
            useVersion(libs.versions.commons.lang.get())
            because("CVE-2025-48924")
        }
    }
}