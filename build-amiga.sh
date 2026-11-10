#!/bin/sh
# Builds Golly 5.0 for AmigaOS 3: 68040, the FPU when there is one
# (fpuauto). The GFOpenWX Toolkit's SDK first (once for each toolkit
# commit), fpuauto's library, then Golly with GLSoft for its pattern view.
#   STOVE=.../stoves/os32-gcc16-open6 OUT=out ./build-amiga.sh
# Golly, the toolkit and fpuauto are fetched at the versions golly.lock
# gives; nothing third-party is kept here. OUT gets Golly, bgolly and
# Golly-<version>.lha (the program with Help, Patterns, Rules and Scripts).
# Copyright (c) 2026 AmiCradle Team
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
. "$HERE/golly.lock"
STOVE=${STOVE:-"$HOME/AmiCradle/stoves/os32-gcc16-open6"}
# the work tree lives outside the checkout (Cradle cleans its checkout
# before every job), so a run that stops at the time limit goes on next time
WORK="${WORK:-${XDG_CACHE_HOME:-$HOME/.cache}/gfamigagolly/build}"
OUT="${OUT:-$WORK/out}"; JOBS=${JOBS:-12}
TC="$STOVE/prefix"
mkdir -p "$WORK/deps" "$WORK/dl" "$OUT"
t0=$(date +%s)
echo "stove: $("$TC/bin/m68k-amigaos-gcc" --version | head -1)"

fetch() {   # name repo commit [paths...]: one commit, its tree (or those paths)
    d="$WORK/deps/$1"; repo=$2; c=$3; shift 3
    if [ "$(cat "$d/.commit" 2>/dev/null)" != "$c" ]; then
        rm -rf "${d:?}"; mkdir -p "$d"
        git -C "$d" init -q
        git -C "$d" fetch -q --depth 1 "$repo" "$c"
        if [ $# -gt 0 ]; then git -C "$d" checkout -q FETCH_HEAD -- "$@"; else git -C "$d" checkout -q FETCH_HEAD; fi
        echo "$c" > "$d/.commit"
    fi
}

# ---- the toolkit's SDK, when its commit changed
fetch gfopenwxtoolkit "$WXT_REPO" "$WXT_COMMIT"
WXT="$WORK/deps/gfopenwxtoolkit"; WXB="$WORK/gfopenwx"
if [ ! -x "$WXB/out/sdk/bin/wx-config" ] || [ "$(cat "$WXB/.rev" 2>/dev/null)" != "$WXT_COMMIT" ]; then
    echo "toolkit $WXT_COMMIT: building its SDK"
    STOVE="$STOVE" WORK="$WXB" OUT="$WXB/out" JOBS=$JOBS sh "$WXT/build-amiga.sh" > "$WORK/wx.log" 2>&1 \
        || { tail -40 "$WORK/wx.log"; exit 1; }
    echo "$WXT_COMMIT" > "$WXB/.rev"
    echo "toolkit: $(( $(date +%s) - t0 )) s"
fi
[ -x "$WXB/out/sdk/bin/wx-config" ] || { tail -40 "$WORK/wx.log"; echo "no toolkit SDK"; exit 1; }
. "$WXT/wx.lock"
WX_TREE="$WXB/wxWidgets-$WX_VERSION"
[ -f "$WX_TREE/src/zlib/zlib.h" ] || { echo "no zlib.h in $WX_TREE/src/zlib"; ls "$WXB"; exit 1; }

# ---- fpuauto: libfpuauto.a from its fpuauto.S, as its build-amiga.sh makes it
fetch fpuauto "$FPUAUTO_REPO" "$FPUAUTO_COMMIT" common/fpuauto
FA="$WORK/deps/fpuauto/common/fpuauto"; FAL="$WORK/fpuauto"
if [ "$(cat "$FAL/.commit" 2>/dev/null)" != "$FPUAUTO_COMMIT" ]; then
    rm -rf "$FAL"; mkdir -p "$FAL/obj"
    "$TC/bin/m68k-amigaos-gcc" -m68040 -DFPUAUTO_COMMON -c "$FA/fpuauto.S" -o "$FAL/obj/fa_common.o"
    for n in $(sed 's/^--wrap=//' "$FA/fpuauto.wraps"); do
        "$TC/bin/m68k-amigaos-gcc" -m68040 -DR_$n -c "$FA/fpuauto.S" -o "$FAL/obj/fa_$n.o"
    done
    "$TC/bin/m68k-amigaos-ar" rcs "$FAL/libfpuauto.a" "$FAL"/obj/fa_*.o
    echo "$FPUAUTO_COMMIT" > "$FAL/.commit"
fi

# ---- Golly's source, with our patches (again when they change)
TB="$WORK/dl/golly-$GOLLY_VERSION-src.tar.gz"
if [ ! -f "$TB" ] || ! echo "$GOLLY_SHA256  $TB" | sha256sum -c - >/dev/null 2>&1; then
    curl -fsSL -o "$TB.part" "$GOLLY_URL"
    echo "$GOLLY_SHA256  $TB.part" | sha256sum -c - >/dev/null || { echo "sha256 mismatch: $GOLLY_URL"; exit 1; }
    mv "$TB.part" "$TB"
fi
SRC="$WORK/golly-$GOLLY_VERSION"
PSUM=$(cat "$HERE"/patches/*.patch | sha256sum | cut -c1-16)
if [ "$(cat "$SRC/.patched" 2>/dev/null)" != "$PSUM" ]; then
    rm -rf "$SRC" "$WORK/obj/wx" "$WORK/obj/base"; mkdir -p "$SRC"
    tar xzf "$TB" -C "$SRC" --strip-components=1
    for p in "$HERE"/patches/*.patch; do
        patch -s -d "$SRC" -p1 < "$p" || { echo "patch failed: $p"; exit 1; }
    done
    echo "$PSUM" > "$SRC/.patched"
fi

# ---- Golly
echo "golly: building (from $(( $(date +%s) - t0 )) s)"
SDK="$WXB/out/sdk"
# objects built on another toolkit commit go: its headers (features, class
# layouts, vtables) are not the ones the make rules list, and a stale object
# beside new ones breaks objects at run time (GLSoft's canvas did)
if [ "$(cat "$WORK/obj/.toolkit" 2>/dev/null)" != "$WXT_COMMIT" ]; then
    rm -rf "$WORK/obj"; mkdir -p "$WORK/obj"
    echo "$WXT_COMMIT" > "$WORK/obj/.toolkit"
fi
nice -n 19 make -k -j"$JOBS" -f "$HERE/scripts/Makefile.amiga" SRC="$SRC" OBJ="$WORK/obj" OUT="$OUT" TC="$TC" \
    WXCONFIG="$SDK/bin/wx-config" WX_TREE="$WX_TREE" WX_ZLIB="$SDK/lib/libwxzlib-${WX_VERSION%.*}.a" \
    GLSOFT="$HERE/glsoft" PORT="$HERE" SPECS="$HERE/scripts/m68k-soft-m68040.specs" FPUSPECS="$FA/fpuauto.specs" FPULIB="$FAL" \
    > "$WORK/golly.log" 2>&1 || { grep -E 'error|Error|undefined' "$WORK/golly.log" | cut -c1-300 | head -40; tail -15 "$WORK/golly.log" | cut -c1-300; }
[ -f "$OUT/Golly" ] || exit 1
# bgolly (the command-line runner) is a bonus: the package goes without it
ls -l "$OUT/Golly" "$OUT/bgolly" 2>&1 || :
OD="$TC/bin/m68k-amigaos-objdump"
echo "fpuauto: $("$OD" -d "$OUT/Golly" | grep -c 'jsr.*<___wrap_' || true) wrapped calls in Golly"

# ---- the package: Golly's drawer
P="$WORK/pkg/Golly"
rm -rf "$WORK/pkg"; mkdir -p "$P"
cp "$OUT/Golly" "$P/"; [ -f "$OUT/bgolly" ] && cp "$OUT/bgolly" "$P/"
cp -R "$SRC/Help" "$SRC/Patterns" "$SRC/Rules" "$SRC/Scripts" "$P/"
cp "$SRC/docs/License.html" "$SRC/docs/ReadMe.html" "$P/" 2>/dev/null || :
mkdir -p "$P/Rexx"; cp "$HERE"/Rexx/*.rexx "$P/Rexx/"   # the ARexx example (README: ARexx)
mkdir -p "$P/tests"; cp "$HERE"/tests/arexx.rexx "$P/tests/"   # the port's test (README: ARexx)
if command -v lha >/dev/null 2>&1; then
    (cd "$WORK/pkg" && lha -aq "$OUT/Golly-$GOLLY_VERSION.lha" Golly >/dev/null) && ls -l "$OUT/Golly-$GOLLY_VERSION.lha"
elif command -v jlha >/dev/null 2>&1; then
    (cd "$WORK/pkg" && jlha -aq "$OUT/Golly-$GOLLY_VERSION.lha" Golly >/dev/null) && ls -l "$OUT/Golly-$GOLLY_VERSION.lha"
else
    (cd "$WORK/pkg" && tar cf "$OUT/Golly-$GOLLY_VERSION.tar" Golly) && echo "no lha here: $OUT/Golly-$GOLLY_VERSION.tar"
fi
echo "total: $(( $(date +%s) - t0 )) s"
