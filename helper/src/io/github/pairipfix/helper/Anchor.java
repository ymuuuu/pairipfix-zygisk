package io.github.pairipfix.helper;

import java.lang.reflect.Method;

public class Anchor {
    public Method backup;
    public static native void onCreate(Object application);

    public Object callback(Object[] args) throws Throwable {
        try {
            onCreate(args.length > 1 ? args[1] : null);
        } catch (Throwable ignored) {}

        Object recv = args[0];
        Object[] rest = new Object[args.length - 1];
        System.arraycopy(args, 1, rest, 0, rest.length);
        return backup.invoke(recv, rest);
    }
}
