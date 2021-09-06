buildscript {
	repositories {
		google()
		mavenCentral()
	}
	dependencies {
		classpath("com.android.tools.build:gradle:7.0.2")
	}
}

tasks.register("clean", Delete::class) {
	delete(rootProject.buildDir)
}
