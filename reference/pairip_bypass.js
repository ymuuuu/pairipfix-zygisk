// pairip_bypass.js
// Usage: frida -U -f com.biltrewards.bilt -l pairip_bypass.js --no-pause

function log(msg) {
    console.log("[*] " + msg);
}

function warn(msg) {
    console.log("[!] " + msg);
}

function printStack() {
    console.log(Java.use("android.util.Log").getStackTraceString(Java.use("java.lang.Exception").$new()));
}

Java.perform(function () {
    // ==========================================
    // 1. BLOCK ALL TERMINATION PATHS
    // ==========================================
    var System = Java.use("java.lang.System");
    System.exit.overload("int").implementation = function (code) {
        warn("System.exit(" + code + ") BLOCKED by Pairip bypass!");
        printStack();
        // Do nothing = app stays alive
        return;
    };

    var Runtime = Java.use("java.lang.Runtime");
    Runtime.halt.overload("int").implementation = function (code) {
        warn("Runtime.halt(" + code + ") BLOCKED!");
        return;
    };

    var Process = Java.use("android.os.Process");
    Process.killProcess.overload("int").implementation = function (pid) {
        warn("Process.killProcess(" + pid + ") BLOCKED!");
        return;
    };

    // ==========================================
    // 2. BLOCK PAIRIP PAYWALL & EXIT LOGIC
    // ==========================================
    try {
        var LicenseClient = Java.use("com.pairip.licensecheck.LicenseClient");

        // Block the paywall activity launch
        if (LicenseClient.startPaywallActivity) {
            LicenseClient.startPaywallActivity.implementation = function () {
                warn("Pairip startPaywallActivity() BLOCKED");
                return;
            };
        }

        // Block the background exit runnable
        if (LicenseClient.createCloseAppIntentOrExitIfAppInBackground) {
            LicenseClient.createCloseAppIntentOrExitIfAppInBackground.implementation = function () {
                warn("Pairip createCloseAppIntentOrExitIfAppInBackground() BLOCKED");
                return;
            };
        }

        // Intercept processResponse and kill it early
        if (LicenseClient.processResponse) {
            LicenseClient.processResponse.implementation = function (response) {
                warn("Pairip processResponse() intercepted — faking valid license");
                // Don't process the failure response at all
                return;
            };
        }

        log("Hooked Pairip LicenseClient");
    } catch (e) {
        warn("LicenseClient hook failed: " + e.message);
    }

    // ==========================================
    // 3. BLOCK PLAY STORE REDIRECTS (safety net)
    // ==========================================
    var Activity = Java.use("android.app.Activity");
    var Context = Java.use("android.content.Context");

    Activity.startActivity.overload("android.content.Intent").implementation = function (intent) {
        var data = intent.getData();
        var component = intent.getComponent();
        var action = intent.getAction();

        if (data && (
            data.toString().indexOf("market://") !== -1 ||
            data.toString().indexOf("play.google.com") !== -1 ||
            data.toString().indexOf("pairip") !== -1
        )) {
            warn("BLOCKED Play Store / Pairip redirect: " + data);
            return;
        }

        // Also block if the stack trace contains Pairip
        var stack = Java.use("android.util.Log").getStackTraceString(Java.use("java.lang.Exception").$new());
        if (stack.indexOf("pairip") !== -1 || stack.indexOf("LicenseClient") !== -1) {
            warn("BLOCKED startActivity() originating from Pairip");
            return;
        }

        return this.startActivity(intent);
    };

    Context.startActivity.overload("android.content.Intent").implementation = function (intent) {
        var data = intent.getData();
        if (data && (data.toString().indexOf("market://") !== -1 || data.toString().indexOf("play.google.com") !== -1)) {
            warn("BLOCKED Context.startActivity() to Play Store");
            return;
        }
        return this.startActivity(intent);
    };

    // ==========================================
    // 4. BLOCK ACTIVITY FINISH (if Pairip tries to close the task)
    // ==========================================
    Activity.finish.overload().implementation = function () {
        var stack = Java.use("android.util.Log").getStackTraceString(Java.use("java.lang.Exception").$new());
        if (stack.indexOf("pairip") !== -1) {
            warn("BLOCKED Activity.finish() from Pairip");
            return;
        }
        return this.finish();
    };

    Activity.finishAffinity.overload().implementation = function () {
        var stack = Java.use("android.util.Log").getStackTraceString(Java.use("java.lang.Exception").$new());
        if (stack.indexOf("pairip") !== -1) {
            warn("BLOCKED Activity.finishAffinity() from Pairip");
            return;
        }
        return this.finishAffinity();
    };

    // ==========================================
    // 5. INSTALLER SPOOF (keep it from your old script)
    // ==========================================
    var ApplicationPackageManager = Java.use("android.app.ApplicationPackageManager");
    if (ApplicationPackageManager.getInstallerPackageName) {
        ApplicationPackageManager.getInstallerPackageName.implementation = function (packageName) {
            if (packageName === "com.biltrewards.bilt") {
                return "com.android.vending";
            }
            return this.getInstallerPackageName(packageName);
        };
    }

    log("Pairip bypass hooks installed. App should stay alive now.");
});