# Rucoy Tile Bot - Zygisk

Phone-only native Zygisk module for the currently tested Rucoy build.

## Current configuration

- Package: `com.mmo.android`
- Library: `libgojni.so`
- Hook RVA: `0x6daabc`
- World target: `(187, 445)`
- Epsilon: `0.18`
- Android tap: `(886, 361)` on display `0`
- ABI: `arm64-v8a`

Configuration lives in `native/src/config.hpp`.

## Build with GitHub Actions

1. Upload the contents of this folder to a GitHub repository. Keep `.github/workflows/build.yml`.
2. Open **Actions** and run **Build Rucoy Tile Bot**.
3. Download the artifact named `Rucoy-TileBot-Zygisk`.
4. Inside the artifact is `Rucoy-TileBot-Zygisk.zip`, ready for a compatible Zygisk-capable root manager.
5. Install, reboot, then launch Rucoy.

The workflow explicitly uses NDK r26d through `setup-ndk`'s `ndk-path` output. It does not compile the currently broken upstream Dobby source tree. Instead it consumes the immutable Android Dobby 1.2 static Prefab artifact from Maven Central and verifies that the archive is AArch64 and exports `DobbyInstrument` before building this module.

## Logs

After reboot and launching the game:

```sh
su -c 'logcat -s RucoyTileBot:I "*:S"'
```

Expected messages include:

```text
RucoyTileBot: target process selected
RucoyTileBot: libgojni.so base=... hook=...
RucoyTileBot: hook active target=(187.000,445.000) eps=0.180 tap=(886,361)
RucoyTileBot: ENTER ptr=... world=(187.xxx,445.xxx)
RucoyTileBot: root tap display=0 x=886 y=361
```

## Important

The RVA is build-specific. If Rucoy updates and `libgojni.so` changes, `0x6daabc` may need to be rediscovered before installing a rebuilt module.

This project does not patch or disable the game's protection library. It only loads in the target process, observes the known native position point, and requests a root input tap through the Zygisk companion.
