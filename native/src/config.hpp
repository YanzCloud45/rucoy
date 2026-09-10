#pragma once

#include <cstdint>

namespace cfg {

inline constexpr char kTargetPackage[] = "com.mmo.android";
inline constexpr char kTargetLibrary[] = "libgojni.so";

// Verified on the user's current Rucoy build.
// Monsters.Update.func1 instrumentation point.
inline constexpr uintptr_t kHookRva = 0x6daabc;

inline constexpr float kTargetX = 187.0f;
inline constexpr float kTargetY = 445.0f;
inline constexpr float kEpsilon = 0.18f;

// Android screen coordinates, 1600x720 landscape.
inline constexpr int kTapX = 886;
inline constexpr int kTapY = 361;
inline constexpr int kDisplayId = 0;

// Wait for libgojni.so to be mapped before installing the instrumentation.
inline constexpr int kLibraryWaitMs = 100;
inline constexpr int kLibraryWaitTries = 1200; // ~120 seconds

} // namespace cfg
