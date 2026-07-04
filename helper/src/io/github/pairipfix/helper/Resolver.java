package io.github.pairipfix.helper;

import java.lang.reflect.Field;
import java.lang.reflect.Method;

public class Resolver {
    public static boolean classExists(ClassLoader cl, String name) {
        try { Class.forName(name, false, cl); return true; }
        catch (Throwable t) { return false; }
    }

    public static Method method(ClassLoader cl, String cls, String name, Class<?>[] params) {
        try {
            Class<?> c = Class.forName(cls, false, cl);
            Method m = c.getDeclaredMethod(name, params);
            m.setAccessible(true);
            return m;
        } catch (Throwable t) { return null; }
    }

    public static Class<?> type(ClassLoader cl, String name) {
        switch (name) {
            case "int": return int.class;
            case "boolean": return boolean.class;
            case "long": return long.class;
            case "float": return float.class;
            case "double": return double.class;
            case "short": return short.class;
            case "byte": return byte.class;
            case "char": return char.class;
            default:
                try { return Class.forName(name, false, cl); }
                catch (Throwable t) { return null; }
        }
    }

    public static boolean setStaticStateOk(ClassLoader cl) {
        try {
            Class<?> client = Class.forName("com.pairip.licensecheck.LicenseClient", false, cl);
            Class<?> stateEnum = Class.forName(
                "com.pairip.licensecheck.LicenseClient$LicenseCheckState", false, cl);
            Object ok = null;
            for (Object v : stateEnum.getEnumConstants()) {
                if (v.toString().equals("FULL_CHECK_OK")) { ok = v; break; }
            }
            if (ok == null) return false;
            Field f = client.getDeclaredField("licenseCheckState");
            f.setAccessible(true);
            f.set(null, ok);
            return true;
        } catch (Throwable t) { return false; }
    }
}
