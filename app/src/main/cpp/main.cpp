// Copyright (c) 2017-2020 The Khronos Group Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include "pch.h"
#include "common.h"
#include "options.h"
#include "platformdata.h"
#include "platformplugin.h"
#include "graphicsplugin.h"
#include "openxr_program.h"
#include <android_native_app_glue.h>

namespace {
    void ShowHelp() {
        Log::Write(Log::Level::Info, "adb shell setprop debug.xr.graphicsPlugin OpenGLES|Vulkan");
    }

    bool UpdateOptionsFromSystemProperties(Options &options) {
        options.GraphicsPlugin = "Vulkan";
        return true;
    }
}  // namespace

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
            Log::Write(Log::Level::Info, "    APP_CMD_START");
            Log::Write(Log::Level::Info, "onStart()");
            break;
        }
        case APP_CMD_RESUME: {
            Log::Write(Log::Level::Info, "onResume()");
            Log::Write(Log::Level::Info, "    APP_CMD_RESUME");
            appState->Resumed = true;
            break;
        }
        case APP_CMD_PAUSE: {
            Log::Write(Log::Level::Info, "onPause()");
            Log::Write(Log::Level::Info, "    APP_CMD_PAUSE");
            appState->Resumed = false;
            break;
        }
        case APP_CMD_STOP: {
            Log::Write(Log::Level::Info, "onStop()");
            Log::Write(Log::Level::Info, "    APP_CMD_STOP");
            break;
        }
        case APP_CMD_DESTROY: {
            Log::Write(Log::Level::Info, "onDestroy()");
            Log::Write(Log::Level::Info, "    APP_CMD_DESTROY");
            appState->NativeWindow = NULL;
            break;
        }
        case APP_CMD_INIT_WINDOW: {
            Log::Write(Log::Level::Info, "surfaceCreated()");
            Log::Write(Log::Level::Info, "    APP_CMD_INIT_WINDOW");
            appState->NativeWindow = app->window;
            break;
        }
        case APP_CMD_TERM_WINDOW: {
            Log::Write(Log::Level::Info, "surfaceDestroyed()");
            Log::Write(Log::Level::Info, "    APP_CMD_TERM_WINDOW");
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
    try {
        JNIEnv *Env;
        app->activity->vm->AttachCurrentThread(&Env, nullptr);

        AndroidAppState appState = {};

        app->userData = &appState;
        app->onAppCmd = app_handle_cmd;

        std::shared_ptr<Options> options = std::make_shared<Options>();
        if (!UpdateOptionsFromSystemProperties(*options)) {
            return;
        }

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
        Log::Write(Log::Level::Error, ex.what());
    } catch (...) {
        Log::Write(Log::Level::Error, "Unknown Error");
    }
}
