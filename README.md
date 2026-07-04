# pairipfix-zygisk

A **Zygisk** port of the [`pairipfix`](https://github.com/ahmedmani) LSPosed
module — same license-check bypass technique, but running as a stealthier Zygisk
native module instead of Xposed, plus a generic native anti-detection shim.

> **Status: work in progress.** The native module (Zygisk entry, LSPlant Java
> hooks, helper dex, native shim) is implemented but **not yet built or tested on a
> device**. Building requires an Android NDK + SDK build-tools (for `d8`); the
> embedded `helper.dex` must be generated first (step 2 below) or the build fails
> by design.

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
- A **generic native syscall/libc anti-detection shim** (via Dobby) that scrubs
  `frida` / `xposed` / `magisk` / `zygisk` traces and denies `ptrace` — closing the
  anti-Frida/anti-debug blind spot the LSPosed module and Frida script both leave open.
- Stealth-first: no companion process, silent by default, hard-gated to pairip apps.

## What is pairip?

Google's `pairipcore` (`libpairipcore.so`) is a runtime protection that, among
other things, validates the app was installed from Google Play, checks the app
signature against repackaging, and detects hooking/debugging tools. This module
neutralizes the license/install-origin/signature Java checks so a sideloaded APK
runs without the "Get this app from Play" screen.

## Build & install

1. Put `android.jar` and `d8.jar` into `build-tools/` (see `build-tools/README.txt`).
2. Run `./helper/build-dex.sh` to generate `native/generated/helper.dex` and `native/jni/helper_dex.h`.
3. Build the module zip:
   ```bash
   export ANDROID_NDK_HOME=/path/to/android-ndk
   python build.py zip
   ```
4. Flash `release/pairipfix-zygisk-*.zip` in Magisk / KernelSU Manager and reboot.

No per-app configuration is needed: the module auto-detects pairip apps via the
`com.pairip.VMRunner` class.

## Detection note

If a specific app still instant-crashes on launch, it may be detecting the module.
As with the original, try
[BetterKnownInstalled](https://github.com/Pixel-Props/BetterKnownInstalled) as an
alternative.

## Legal

For research, interoperability, and analysis of software you own or are authorized
to test. You are responsible for complying with applicable law and the terms of
any software you use this with.
