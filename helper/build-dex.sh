#!/usr/bin/env bash
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
OUT="$HERE/../native/generated"
mkdir -p "$OUT"
CLASSES="$HERE/classes"; rm -rf "$CLASSES"; mkdir -p "$CLASSES"

# Point these at your Android SDK build-tools or set env D8_JAR / ANDROID_HOME.
: "${D8_JAR:=${HERE}/../../build-tools/d8.jar}"
: "${ANDROID_JAR:=${HERE}/../../build-tools/android.jar}"

javac -source 8 -target 8 -cp "$ANDROID_JAR" -d "$CLASSES" \
  "$HERE"/src/io/github/pairipfix/helper/*.java

java -cp "$D8_JAR" com.android.tools.r8.D8 \
  --min-api 26 --output "$OUT" $(find "$CLASSES" -name '*.class')

mv "$OUT/classes.dex" "$OUT/helper.dex"

python3 - "$OUT/helper.dex" "$OUT/../jni/helper_dex.h" <<'PY'
import sys
data = open(sys.argv[1], 'rb').read()
with open(sys.argv[2], 'w') as f:
    f.write("#pragma once\n#include <cstddef>\n")
    f.write("static const unsigned char kHelperDex[] = {")
    f.write(",".join(str(b) for b in data))
    f.write("};\n")
    f.write(f"static const size_t kHelperDexLen = {len(data)};\n")
PY

echo "helper.dex: $(wc -c < \"$OUT/helper.dex\") bytes"
