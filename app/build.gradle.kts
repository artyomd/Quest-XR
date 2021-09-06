plugins {
    id("com.android.application")
}
repositories {
    google()
    mavenCentral()
}
android {
    compileSdk = 30
    buildToolsVersion = "30.0.3"
    ndkVersion = "23.0.7599858"
    defaultConfig {
        minSdk = 29
        targetSdk = 30
        versionCode = 1
        versionName = "1.0"
        applicationId = "app.artyomd.questdemo"
        externalNativeBuild {
            cmake {
                arguments.add("-DANDROID_STL=c++_static")
            }
            ndk {
                abiFilters.apply {
                    clear()
                    addAll(setOf("arm64-v8a"))
                }
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
            jniLibs.srcDir("libs/debug")
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
