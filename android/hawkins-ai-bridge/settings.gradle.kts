pluginManagement {
    repositories { google(); mavenCentral(); gradlePluginPortal() }
}
dependencyResolutionManagement {
    repositoriesMode.set(RepositoriesMode.FAIL_ON_PROJECT_REPOS)
    repositories {
        google()
        mavenCentral()
        maven(url = "https://jitpack.io")
    }
}
rootProject.name = "HawkinsAiBridge"
include(":app", ":llama-lib")
project(":llama-lib").projectDir =
    file("../llama.cpp/examples/llama.android/lib")
