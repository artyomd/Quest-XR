# Quest XR

This is a sample project that is using only OpenXR and Vulkan on the android platform and targets Oculus Quest 2 device. This project demonstrates how to create a native c++ only project with Gradle and Cmake and utilize OpenXR runtime. The project was taken from the official [OpenXR SDK samples](https://github.com/KhronosGroup/OpenXR-SDK-Source).


### Building the project

Only the following dependencies are required:

- [Android SDK](https://developer.android.com/studio)
  
- [Vulkan SDK](https://vulkan.lunarg.com/sdk/home)
  
To build the project just run the following in the project directory:

```bash
 ./gradlew assembleDebug #for debug build
 ./gradlew assembleRelease # for release build
```

After that, apk can be found in `app/build/outputs/apk/` directory.

### Preview (Screenshot from Quest2)

![](https://user-images.githubusercontent.com/22776744/148455860-78d585cc-252c-481c-9fb3-a45999326977.jpg)
