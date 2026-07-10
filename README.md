# pairipfix-zygisk

A **Zygisk** port of the [`pairipfix`](https://github.com/ahmedmani) LSPosed
module — same license-check bypass technique, but running as a stealthier Zygisk
native module instead of Xposed.

> **Status: working — first release (`v0.1.0-dev`).**
> Happy to read some issues :"D

## Tested ON

> Pixel 6A bluejay | Android 16 | KernelSU-Next | SUSFSv2.1

## Credits

Based on the original **pairipfix** LSPosed module by
**[ahmedmani](https://github.com/ahmedmani)**. This project re-implements that
module's Java-hook technique on Zygisk and extends it. All credit for the original
bypass approach and the pairip license-check research goes to the original author.

## Why Zygisk instead of LSPosed?

The LSPosed module works, but Xposed is detected fairly easily on many devices.
Zygisk injection is harder to fingerprint. This port keeps that stealth advantage:

- Same 4 Java hooks (Signature / LicenseClient / LicenseActivity / LicenseResponse),
  applied via **LSPlant** (the same ART hooking library LSPosed uses under the hood).
- One extra: `getInstallerPackageName` spoof (from the reference Frida script).
- Stealth-first: no companion process, silent by default, hard-gated to pairip apps.

## What is pairip?

Google's `pairipcore` (`libpairipcore.so`) is a runtime protection that, among
other things, validates the app was installed from Google Play, checks the app
signature against repackaging, and detects hooking/debugging tools. This module
neutralizes the license/install-origin/signature Java checks so a sideloaded APK
runs without the "Get this app from Play" screen.

## Installation

1. Flash `pairipfix-zygisk-*.zip` in Magisk / KernelSU Manager and reboot.

## Build & install

1. Put `android.jar` and `d8.jar` into `build-tools/` (see `build-tools/README.txt`).
2. Run `./helper/build-dex.sh` to generate `native/generated/helper.dex` and `native/jni/helper_dex.h`.
3. Build the module zip:
   ```bash
   # Point at your Android SDK; the NDK is resolved from <ANDROID_HOME>/ndk/<ndkVer>
   export ANDROID_HOME=/path/to/android-sdk
   python build.py zip                 # NDK version comes from project-config.json
   # or override the NDK version explicitly:
   python build.py --ndk 29.0.14206865 zip
   ```
4. Flash `release/pairipfix-zygisk-*.zip` in Magisk / KernelSU Manager and reboot.

No per-app configuration is needed: the module auto-detects pairip apps via the
`com.pairip.VMRunner` class.

## Legal

For research, interoperability, and analysis of software you own or are authorized
to test. You are responsible for complying with applicable law and the terms of
any software you use this with.
