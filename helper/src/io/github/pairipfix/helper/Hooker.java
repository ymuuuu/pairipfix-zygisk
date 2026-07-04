package io.github.pairipfix.helper;

import java.lang.reflect.Method;

public class Hooker {
    public static final int DO_NOTHING = 0;
    public static final int RETURN_CONST = 1;
    public static final int INSTALLER_SPOOF = 2;

    public int mode;
    public Object constant;
    public Method backup;
    public String selfPackage;

    public Object callback(Object[] args) throws Throwable {
        switch (mode) {
            case DO_NOTHING:
                return null;
            case RETURN_CONST:
                return constant;
            case INSTALLER_SPOOF: {
                String pkg = args.length > 1 ? (String) args[1] : null;
                if (selfPackage != null && selfPackage.equals(pkg)) {
                    return "com.android.vending";
                }
                Object recv = args[0];
                Object[] rest = new Object[args.length - 1];
                System.arraycopy(args, 1, rest, 0, rest.length);
                return backup.invoke(recv, rest);
            }
            default:
                return null;
        }
    }
}
