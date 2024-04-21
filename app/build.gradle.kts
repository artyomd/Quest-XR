plugins {
    id("com.android.application")
}
android {
    compileSdk = 32
    ndkVersion = "26.3.11579264"
    namespace = "app.artyomd.questxr"
    defaultConfig {
        minSdk = 32
        targetSdk = 32
        versionCode = 1
        versionName = "1.0"
        applicationId = "app.artyomd.questxr"
        externalNativeBuild {
            cmake {
                arguments.add("-DANDROID_STL=c++_shared")
                arguments.add("-DANDROID_USE_LEGACY_TOOLCHAIN_FILE=OFF")
            }
            ndk {
                abiFilters.add("arm64-v8a")
            }
        }
    }
    buildTypes {
        getByName("release") {
            isDebuggable = false
            isJniDebuggable = false
        }
        getByName("debug") {
            isDebuggable = true
            isJniDebuggable = true
        }
    }
    externalNativeBuild {
        cmake {
            version = "3.22.1"
            path("CMakeLists.txt")
        }
    }
    sourceSets {
        getByName("main") {
            manifest.srcFile("AndroidManifest.xml")
        }
        getByName("debug") {
            jniLibs {
                srcDir("libs/debug")
            }
        }
        getByName("release") {
            jniLibs.srcDir("libs/release")
        }
    }
    packaging {
        jniLibs {
            keepDebugSymbols.add("**.so")
        }
    }
}
