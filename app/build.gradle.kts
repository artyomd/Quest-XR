plugins {
    id("com.android.application")
}
repositories {
    google()
    mavenCentral()
}
android {
    compileSdk = 29
    ndkVersion = "23.1.7779620"
    defaultConfig {
        minSdk = 29
        targetSdk = 29
        versionCode = 1
        versionName = "1.0"
        applicationId = "app.artyomd.questxr"
        externalNativeBuild {
            cmake {
                arguments.add("-DANDROID_STL=c++_static")
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
            version = "3.18.1"
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
    packagingOptions {
        jniLibs {
            keepDebugSymbols.add("**.so")
        }
    }
}
