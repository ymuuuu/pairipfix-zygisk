# pairipfix-zygisk

A **Zygisk** port of the [`pairipfix`](https://github.com/ahmedmani) LSPosed
module — same license-check bypass technique, but running as a stealthier Zygisk
native module instead of Xposed, plus a generic native anti-detection shim.

> **Status: work in progress.** Design and implementation plan live in
> [`plan.md`](./plan.md). Nothing is built yet.

## Credits

Based on the original **pairipfix** LSPosed module by
**[ahmedmani](https://github.com/ahmedmani)**. This project re-implements that
module's Java-hook technique on Zygisk and extends it. All credit for the original
bypass approach and the pairip license-check research goes to the original author.

The archived originals (the LSPosed module sources and the reference Frida script)
are kept under [`reference/`](./reference/) for parity checks.

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

To be documented once implemented (Zygisk module zip, flashable via Magisk /
KernelSU). See [`plan.md`](./plan.md) §7–§8.

## Detection note

If a specific app still instant-crashes on launch, it may be detecting the module.
As with the original, try
[BetterKnownInstalled](https://github.com/Pixel-Props/BetterKnownInstalled) as an
alternative.

## Legal

For research, interoperability, and analysis of software you own or are authorized
to test. You are responsible for complying with applicable law and the terms of
any software you use this with.
