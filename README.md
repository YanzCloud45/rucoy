# Rucoy Tile Bot - Zygisk

Phone-only Zygisk module for the currently verified Rucoy Online build.

## Current configuration

- Package: `com.mmo.android`
- Native library: `libgojni.so`
- Instrumentation RVA: `0x6daabc`
- Target world position: `(187, 445)`
- Tolerance: `0.18`
- Android tap: `(886, 361)` on display `0`
- ABI: `arm64-v8a`

The native callback reads ARM64 `S0/S1` from Dobby's register context. It keeps an ENTER/EXIT state per observed monster pointer. On ENTER, it signals a root Zygisk companion over a socket. The companion executes `/system/bin/input -d 0 tap 886 361`.

## Build with GitHub Actions

1. Extract this source ZIP.
2. Create a new GitHub repository and upload all extracted files, including `.github/workflows/build.yml`.
3. Open **Actions** -> **Build Rucoy Tile Bot** -> **Run workflow**.
4. Download the `Rucoy-TileBot-Zygisk` artifact.
5. Inside the artifact is `Rucoy-TileBot-Zygisk.zip`, installable as a root module.
6. Reboot the phone.

The workflow fetches the published Zygisk API header and checks out Dobby at commit `809f8ca`, then builds only `arm64-v8a` with Android NDK r26d.

## Logs

```sh
su -c 'logcat -s RucoyTileBot:I *:S'
```

Expected messages include:

```text
RucoyTileBot: target process selected; companion fd=...
RucoyTileBot: libgojni.so base=... hook=...
RucoyTileBot: hook active target=(187.000,445.000) eps=0.180 tap=(886,361)
RucoyTileBot: ENTER ptr=... world=(187.000,445.000)
RucoyTileBot: root tap display=0 x=886 y=361
```

## Change coordinates / RVA

Edit `native/src/config.hpp` and run the workflow again.

## Important

The RVA is build-specific. If Rucoy updates, `0x6daabc` may no longer be correct and the module should be disabled until the new hook location is verified.

This project does not patch or disable app protection libraries. If the game rejects native instrumentation on a particular build/device, the module intentionally contains no protection-bypass logic.
