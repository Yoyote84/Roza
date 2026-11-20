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
        versionCode = 4
        versionName = "1.0.3"
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
    // llama.cpp loads CPU backends as dynamic plugins from nativeLibraryDir.
    // Compressing JNI libraries forces Android to extract them to that directory
    // at install time, where ggml_backend_load_all_from_path() can enumerate them.
    packaging {
        jniLibs {
            useLegacyPackaging = true
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
    testImplementation("org.json:json:20240303")
}
