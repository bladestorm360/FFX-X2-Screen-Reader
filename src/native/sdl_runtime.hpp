#pragma once
#define SDL_MAIN_HANDLED
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_gamepad.h>
#include <SDL3/SDL_audio.h>
#include <SDL3/SDL_hints.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_version.h>
#define FFX_SDL_FUNCTIONS(X) \
    X(SDL_SetMainReady) X(SDL_IsMainThread) X(SDL_WasInit) X(SDL_GetVersion) X(SDL_GetError) X(SDL_SetHint) X(SDL_GetTicksNS) \
    X(SDL_InitSubSystem) X(SDL_PumpEvents) X(SDL_PeepEvents) X(SDL_UpdateGamepads) \
    X(SDL_GetGamepads) X(SDL_free) X(SDL_OpenGamepad) X(SDL_CloseGamepad) \
    X(SDL_GamepadConnected) X(SDL_GetGamepadID) X(SDL_GetGamepadName) X(SDL_GetGamepadButton) X(SDL_GetGamepadAxis) X(SDL_RumbleGamepad) \
    X(SDL_OpenAudioDeviceStream) X(SDL_ResumeAudioStreamDevice) X(SDL_DestroyAudioStream) \
    X(SDL_PutAudioStreamData) X(SDL_ClearAudioStream)
namespace ffx::sdl {
#define FFX_SDL_DECLARE(name) extern decltype(&::name) name;
FFX_SDL_FUNCTIONS(FFX_SDL_DECLARE)
#undef FFX_SDL_DECLARE
bool load();
}
