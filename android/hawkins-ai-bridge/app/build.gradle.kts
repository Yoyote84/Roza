plugins {
    alias(libs.plugins.android.application)
    alias(libs.plugins.jetbrains.kotlin.android)
}
android {
    namespace = "fr.roza.hawkinsai"
    compileSdk = 36
    ndkVersion = "29.0.13113456"
    defaultConfig {
        applicationId = "fr.roza.hawkinsai"
        minSdk = 33
        targetSdk = 36
        versionCode = 1
        versionName = "1.0.0"
        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"
        ndk { abiFilters += listOf("arm64-v8a") }
    }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }
    kotlin {
        jvmToolchain(17)
        compileOptions {
            targetCompatibility = JavaVersion.VERSION_17
        }
    }
    testOptions { unitTests.isReturnDefaultValues = true }
}
dependencies {
    implementation(project(":llama-lib"))
    implementation(libs.androidx.activity)
    implementation(libs.androidx.core.ktx)
    implementation(libs.kotlinx.coroutines.android)
    implementation("com.github.mik3y:usb-serial-for-android:3.12.0")
    testImplementation(libs.junit)
}
