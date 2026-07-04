# pairipfix-zygisk — Design & Plan

> Zygisk port of the `pairipfix` LSPosed module. Same Java-hook technique, but
> stealthier (Zygisk vs Xposed), plus a generic native anti-detection shim that
> covers the anti-Frida/anti-debug blind spot neither the LSPosed module nor the
> Frida script currently handles.

Status: **design approved, awaiting spec review** · Date: 2026-07-04

---

## 1. Goal & non-goals

**Goal.** A single Zygisk native module that:
1. Reproduces the 4 LSPosed hooks (Signature / LicenseClient / LicenseActivity /
   LicenseResponse) via Java/ART hooking.
2. Adds one cheap Frida-script extra: `getInstallerPackageName` spoof.
3. Adds a generic native syscall/libc anti-detection shim (ptrace/proc/string
   scrub) — the real enhancement Zygisk unlocks.
4. Stays **stealthy**: no companion process, silent by default, hard-gated to
   pairip apps, minimal fingerprint.

**Non-goals (YAGNI — keep it lean).**
- No "trace every function" recon tracer (that idea belonged to the Frida script).
- No reversing of `libpairipcore.so` (generic shim only; per-APK offset hooks are
  explicitly out of scope — documented as a future slot, not built).
- No port of the Frida script's blunt Java nets (`System.exit`/`halt`/`killProcess`
  block, `startActivity` redirect block, `Activity.finish` block). Once the
  license/signature checks pass at source, pairip never triggers those paths, so
  the nets are redundant surface.
- No host-side unit test harness for injection (verified on-device instead).

---

## 2. Architecture

Zygisk `.so` is loaded into every forked app process. It gates hard and no-ops
everywhere except pairip apps.

```
Zygisk module (native C++)
├── onLoad
├── preAppSpecialize   ── read package/nice-name (decide candidacy cheaply)
├── postAppSpecialize
│     ├── Engine B (native shim) installs immediately (before pairip .so runs)
│     └── Engine A anchor hook installed (deferred real hooks — see timing)
│
├── Engine A: LSPlant (Java/ART hooks)  ── 4 LSPosed hooks + installer spoof
│      backend: Dobby (ART symbol resolve + inline patch)
│
└── Engine B: Dobby (native libc/syscall hooks)  ── anti-detection shim
```

**One native hooking dependency: Dobby.** LSPlant uses Dobby as its inline-hook
backend, so the same lib serves both engines. Deps: **LSPlant** (approved) +
**Dobby** (transitive, also used directly by Engine B).

---

## 3. Hook timing (critical detail)

At `postAppSpecialize`, pairip's Java classes are not loaded yet — `FindClass`
would fail. So Engine A does **not** hook pairip classes directly at specialize
time. Instead:

1. At `postAppSpecialize`, LSPlant-hook one boot-class anchor that IS resolvable
   then: `android.app.Instrumentation.callApplicationOnCreate(Application)`.
2. In that callback (runs once, early in app startup, app classloader now live):
   - `ClassLoader cl = app.getClassLoader();`
   - Try-load `com.pairip.VMRunner` via `cl`.
   - **Absent** → not a pairip app (or unrecognized protection): unhook anchor,
     no-op, ideally `dlclose` self. Same guard as `mainHook.handleLoadPackage`.
   - **Present** → install the real Engine A hooks against `cl`, then unhook the
     anchor to minimize residual surface.

This mirrors exactly how the LSPosed module obtains `lpparam.classLoader`.

Engine B installs at `postAppSpecialize` (before pairip native checks run); it is
harmless in non-pairip processes but SHOULD still be gated to candidate packages
to avoid touching unrelated apps (see §6 gating).

---

## 4. Engine A — Java hooks (LSPlant port of the LSPosed module)

Ported 1:1 from `pairipfix_lsposed`. All hooks best-effort with class-exists
guards (missing class ≠ crash), matching `BaseHook` semantics.

| # | Target | Signature | Action | Source hook |
|---|--------|-----------|--------|-------------|
| 1 | `com.pairip.SignatureCheck.verifyIntegrity` | `(Context)` | do-nothing | SignatureBypass |
| 2 | `com.pairip.SignatureCheck.verifySignatureMatches` | `(String)` | return `true` | SignatureBypass |
| 3 | `com.pairip.licensecheck.LicenseClient.initializeLicenseCheck` | `()` | do-nothing | LicenseClientBypass |
| 4 | `LicenseClient.performLocalInstallerCheck` | `()` | return `true` | LicenseClientBypass |
| 5 | static field `LicenseClient.licenseCheckState` | — | set = `LicenseCheckState.FULL_CHECK_OK` | LicenseClientBypass |
| 6 | `com.pairip.licensecheck3.LicenseClientV3.processResponse` | `(int,Bundle)` | do-nothing *(if class exists)* | LicenseClientBypass |
| 7 | `com.pairip.licensecheck.LicenseActivity.closeApp` | `()` | do-nothing | LicenseActivityBypass |
| 8 | `LicenseActivity.exitApp` | `()` | do-nothing | LicenseActivityBypass |
| 9 | `LicenseActivity.showErrorDialog` | `()` | do-nothing | LicenseActivityBypass |
| 10 | `LicenseActivity.showPaywallAndCloseApp` | `()` | do-nothing | LicenseActivityBypass |
| 11 | `LicenseActivity.logAndShowErrorDialog` | `(String)` | do-nothing | LicenseActivityBypass |
| 12 | `LicenseActivity.logAndShowErrorDialog` | `(String,Exception)` | do-nothing | LicenseActivityBypass |
| 13 | `com.pairip.licensecheck.LicenseResponseHelper.validateResponse` | `(Bundle,String)` | do-nothing | LicenseResponseBypass |
| 14 | `LicenseResponseHelper.getRepeatedCheckMetadata` | `(Bundle)` | return `null` | LicenseResponseBypass |
| 15 | `LicenseResponseHelper.verifySignature` | `(String,String,String,PublicKey)` | do-nothing | LicenseResponseBypass |
| 16 | `com.pairip.licensecheck.ResponseValidator.validateResponse` | `(Bundle,String)` | do-nothing *(if exists)* | LicenseResponseBypass |
| 17 | `ResponseValidator.verifySignature` | `(String,String,String,PublicKey)` | do-nothing *(if exists)* | LicenseResponseBypass |
| 18 | `com.pairip.licensecheck3.ResponseValidator.validateResponse` | `(Bundle,String)` | do-nothing *(if exists)* | LicenseResponseBypass |
| 19 | `android.app.ApplicationPackageManager.getInstallerPackageName` | `(String)` | return `"com.android.vending"` for own pkg, else original | Frida extra |

**LSPlant mechanics.** For do-nothing/return-constant hooks, LSPlant hooks the
target with a native callback; the callback either returns immediately (void) or
returns a boxed constant, and does **not** call the backup (original). For the
static-field set (#5), no hook needed — resolve field via JNI in the anchor
callback and `SetStaticObjectField`. `getInstallerPackageName` (#19) inspects the
arg and conditionally calls the backup.

---

## 5. Engine B — native anti-detection shim (Dobby)

Per-process inline hooks on libc/syscall symbols. Tight allowlist so the app
itself is unaffected. Purpose: keep Zygisk's stealth edge by hiding the exact
artifacts pairip's native detection looks for.

| Symbol | Behavior |
|--------|----------|
| `ptrace` | `PTRACE_TRACEME`/attach probes → return success/deny without real trace |
| `openat` / `open` | on `/proc/self/maps`, `/proc/self/status`, `/proc/self/task`, `/proc/self/mounts`, `/proc/net/tcp` → serve scrubbed copy (fd to filtered temp) |
| `read` | on scrubbed fds, drop lines containing tokens (below) |
| `strstr` / `strcmp` (targeted) | neutralize matches on detection tokens |
| `kill(pid, 0)` / `fork` | liveness/self-debug probes → spoof expected result |

**Scrub token list:** `frida`, `gum`, `frida-agent`, `frida-gadget`, `xposed`,
`lsposed`, `magisk`, `zygisk`, `riru`, `substrate`, `27042` (frida default port).

Scope guard: install Engine B only in candidate/pairip processes (§6), never
system-wide.

---

## 6. Gating & stealth (the whole point)

- **Candidate detection.** `preAppSpecialize`/`postAppSpecialize` expose the
  process/package name. Cheap first filter there; authoritative filter is the
  `com.pairip.VMRunner` class-load check in the anchor callback (§3). Non-pairip →
  unhook + `dlclose` self.
- **No companion process.** No root-side work needed; omitting the companion
  removes a well-known Zygisk fingerprint.
- **Silent by default.** All logging behind compile-time `PAIRIP_LOG` macro.
  Release build emits zero logcat (nothing to `grep` for a tag). Debug build logs
  to a neutral tag for verification only.
- **Minimal residual surface.** Unhook the anchor after real hooks install; only
  the intended targets stay patched.
- **No persistence side-effects.** No files in app storage, no network, no props.
- **Maps hygiene (optional hardening, flagged not committed).** Consider
  anonymizing/generic-naming the module `.so` mapping so it doesn't stand out in
  `/proc/self/maps`; Engine B already scrubs reads of that file for the app.
- **Future slot (documented, NOT built).** Per-APK offset-based native hooks on
  `libpairipcore.so` if some app defeats the generic shim.

---

## 7. Project layout

Rename folder `pairipfix-frida` → **`pairipfix-zygisk`**.

```
pairipfix-zygisk/
├── plan.md                         # this file
├── README.md                       # ported/updated from LSPosed README
├── build.py                        # from 5ec1cff/zygisk-module-template
├── project-config.json
├── native/
│   ├── CMakeLists.txt
│   ├── jni/
│   │   ├── main.cpp                # Zygisk entry (onLoad, {pre,post}AppSpecialize)
│   │   ├── gate.{h,cpp}            # candidate detection + VMRunner guard
│   │   ├── java_hooks.{h,cpp}      # Engine A: LSPlant hook installs (§4)
│   │   ├── native_shim.{h,cpp}     # Engine B: Dobby libc/syscall hooks (§5)
│   │   ├── log.h                   # PAIRIP_LOG gate
│   │   └── zygisk.hpp              # Zygisk API header
│   └── external/
│       ├── lsplant/                # submodule
│       └── dobby/                  # submodule (LSPlant backend + Engine B)
├── template/
│   └── module.prop, customize.sh, ...
└── reference/                      # archived originals (not built)
    ├── pairip_bypass.js
    └── pairipfix_lsposed/          # original LSPosed sources for parity checks
```

Build output: flashable Magisk/KernelSU zip with `module.prop` +
`zygisk/arm64-v8a.so`, `zygisk/armeabi-v7a.so`.

---

## 8. Implementation phases

1. **Scaffold.** Clone template into renamed folder, wire `build.py`/CMake,
   confirm an empty module builds + flashes + loads (logcat sanity).
2. **Deps.** Add LSPlant + Dobby submodules, initialize LSPlant
   (`InitInfo`/ART symbol resolution), confirm link.
3. **Gate.** Implement candidate detection + `VMRunner` guard + `dlclose`
   no-op path. Verify module stays silent/inert in non-pairip apps.
4. **Anchor + Engine A.** Install `Instrumentation.callApplicationOnCreate`
   anchor; in callback resolve app classloader and install hooks #1–#19; unhook
   anchor. Verify each hook fires (debug log build).
5. **Engine B.** Implement native shim hooks (§5) with allowlist; verify no app
   breakage.
6. **Stealth pass.** Strip logging in release, confirm no companion, minimize
   residual mappings, self-check against common detectors.
7. **Docs.** Update README (install, scope, limitations, "detected? try
   BetterKnownInstalled" note carried over).

---

## 9. Testing / verification (on-device, manual)

No host unit tests for injection. Verification checklist:

- [ ] Empty module builds, flashes, loads (phase 1).
- [ ] Non-pairip app: module no-ops, silent, `dlclose` path taken.
- [ ] Known pairip app: launches past the "Get this app from Play" screen, no
      paywall, no forced exit.
- [ ] Debug build: logcat shows each of hooks #1–#19 installed + fired where
      exercised; Engine B shows ptrace/proc scrubs.
- [ ] Release build: **zero** logcat output from module.
- [ ] App remains functional (protected VM methods still run — we never block
      `VMRunner`).
- [ ] Basic anti-detect self-test: app that reads `/proc/self/maps` for `frida`/
      `magisk` sees scrubbed output.

**Guard skills during implementation:** run `clean-code-guard` on native C++
after each engine lands; `docs-guard` on README before finalizing.

---

## 10. Risks & mitigations

| Risk | Mitigation |
|------|-----------|
| LSPlant ART version drift | Use upstream LSPlant (tracks AOSP); pin a known-good release |
| Anchor method absent/renamed on some ROMs | Fallback anchor candidates (`ActivityThread.handleBindApplication`); guard best-effort |
| Engine B breaks the app (over-broad scrub) | Tight path/token allowlist; per-process gating; feature-flag each shim hook |
| Zygisk module itself detected | No companion, silent, maps hygiene, gate hard |
| pairip VM crash if a needed method is stubbed | Only stub license/UI/validator methods — never `VMRunner`/VM execution |
| Generic shim insufficient for a specific app | Documented future slot for per-APK offset hooks (not built now) |

---

## 11. Decisions (resolved)

- **Folder rename:** done — `pairipfix-frida` → `pairipfix-zygisk`.
- **Originals:** archived under `reference/` (not deleted).
- **Min Android / NDK / Zygisk API:** pin to the `5ec1cff/zygisk-module-template`
  defaults; no custom targets.
- **GitHub:** repo `ymuuuu/pairipfix-zygisk` created (private), `main` pushed.
- **Attribution:** README credits original `pairipfix` author **ahmedmani**.
```
