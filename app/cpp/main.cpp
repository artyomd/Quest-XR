// Copyright (c) 2017-2021, The Khronos Group Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include <android_native_app_glue.h>
#include "openxr-include.h"
#include "common.h"
#include "options.h"
#include "platformdata.h"
#include "platformplugin.h"
#include "graphicsplugin.h"
#include "openxr_program.h"
#include <shaders.hpp>

struct AndroidAppState {
  ANativeWindow *NativeWindow = nullptr;
  bool Resumed = false;
};

/**
 * Process the next main command.
 */
static void app_handle_cmd(struct android_app *app, int32_t cmd) {
  AndroidAppState *appState = (AndroidAppState *) app->userData;

  switch (cmd) {
    // There is no APP_CMD_CREATE. The ANativeActivity creates the
    // application thread from onCreate(). The application thread
    // then calls android_main().
    case APP_CMD_START: {
      utils::logger::Log(utils::logger::Level::INFO, "    APP_CMD_START");
      utils::logger::Log(utils::logger::Level::INFO, "onStart()");
      break;
    }
    case APP_CMD_RESUME: {
      utils::logger::Log(utils::logger::Level::INFO, "onResume()");
      utils::logger::Log(utils::logger::Level::INFO, "    APP_CMD_RESUME");
      appState->Resumed = true;
      break;
    }
    case APP_CMD_PAUSE: {
      utils::logger::Log(utils::logger::Level::INFO, "onPause()");
      utils::logger::Log(utils::logger::Level::INFO, "    APP_CMD_PAUSE");
      appState->Resumed = false;
      break;
    }
    case APP_CMD_STOP: {
      utils::logger::Log(utils::logger::Level::INFO, "onStop()");
      utils::logger::Log(utils::logger::Level::INFO, "    APP_CMD_STOP");
      break;
    }
    case APP_CMD_DESTROY: {
      utils::logger::Log(utils::logger::Level::INFO, "onDestroy()");
      utils::logger::Log(utils::logger::Level::INFO, "    APP_CMD_DESTROY");
      appState->NativeWindow = NULL;
      break;
    }
    case APP_CMD_INIT_WINDOW: {
      utils::logger::Log(utils::logger::Level::INFO, "surfaceCreated()");
      utils::logger::Log(utils::logger::Level::INFO, "    APP_CMD_INIT_WINDOW");
      appState->NativeWindow = app->window;
      break;
    }
    case APP_CMD_TERM_WINDOW: {
      utils::logger::Log(utils::logger::Level::INFO, "surfaceDestroyed()");
      utils::logger::Log(utils::logger::Level::INFO, "    APP_CMD_TERM_WINDOW");
      appState->NativeWindow = NULL;
      break;
    }
  }
}

/**
 * This is the main entry point of a native application that is using
 * android_native_app_glue.  It runs in its own thread, with its own
 * event loop for receiving input events and doing other things.
 */
void android_main(struct android_app *app) {
  LoadShaders();
  try {
    JNIEnv *Env;
    app->activity->vm->AttachCurrentThread(&Env, nullptr);

    AndroidAppState appState = {};

    app->userData = &appState;
    app->onAppCmd = app_handle_cmd;

    std::shared_ptr<Options> options = std::make_shared<Options>();
    options->GraphicsPlugin = "Vulkan";

    std::shared_ptr<PlatformData> data = std::make_shared<PlatformData>();
    data->applicationVM = app->activity->vm;
    data->applicationActivity = app->activity->clazz;

    bool requestRestart = false;
    bool exitRenderLoop = false;

    // Create platform-specific implementation.
    std::shared_ptr<IPlatformPlugin> platformPlugin = CreatePlatformPlugin(options, data);
    // Create graphics API implementation.
    std::shared_ptr<IGraphicsPlugin> graphicsPlugin = CreateGraphicsPlugin(options,
                                                                           platformPlugin);

    // Initialize the OpenXR program.
    std::shared_ptr<IOpenXrProgram> program = CreateOpenXrProgram(options, platformPlugin,
                                                                  graphicsPlugin);

    // Initialize the loader for this platform
    PFN_xrInitializeLoaderKHR initializeLoader = nullptr;
    if (XR_SUCCEEDED(
        xrGetInstanceProcAddr(XR_NULL_HANDLE, "xrInitializeLoaderKHR",
                              (PFN_xrVoidFunction *) (&initializeLoader)))) {
      XrLoaderInitInfoAndroidKHR loaderInitInfoAndroid;
      memset(&loaderInitInfoAndroid, 0, sizeof(loaderInitInfoAndroid));
      loaderInitInfoAndroid.type = XR_TYPE_LOADER_INIT_INFO_ANDROID_KHR;
      loaderInitInfoAndroid.next = NULL;
      loaderInitInfoAndroid.applicationVM = app->activity->vm;
      loaderInitInfoAndroid.applicationContext = app->activity->clazz;
      initializeLoader((const XrLoaderInitInfoBaseHeaderKHR *) &loaderInitInfoAndroid);
    }

    program->CreateInstance();
    program->InitializeSystem();
    program->InitializeSession();
    program->CreateSwapchains();

    while (app->destroyRequested == 0) {
      // Read all pending events.
      for (;;) {
        int events;
        struct android_poll_source *source;
        // If the timeout is zero, returns immediately without blocking.
        // If the timeout is negative, waits indefinitely until an event appears.
        const int timeoutMilliseconds =
            (!appState.Resumed && !program->IsSessionRunning() &&
                app->destroyRequested == 0) ? -1 : 0;
        if (ALooper_pollAll(timeoutMilliseconds, nullptr, &events, (void **) &source) < 0) {
          break;
        }

        // Process this event.
        if (source != nullptr) {
          source->process(app, source);
        }
      }

      program->PollEvents(&exitRenderLoop, &requestRestart);
      if (!program->IsSessionRunning()) {
        continue;
      }

      program->PollActions();
      program->RenderFrame();
    }

    app->activity->vm->DetachCurrentThread();
  } catch (const std::exception &ex) {
    utils::logger::Log(utils::logger::Level::FATAL, ex.what());
  } catch (...) {
    utils::logger::Log(utils::logger::Level::FATAL, "Unknown Error");
  }
}
