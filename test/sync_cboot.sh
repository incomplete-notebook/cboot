#!/bin/bash
# Sync .c source files to .cboot code blocks
# Usage: ./sync_cboot.sh [module1] [module2] ...
# If no modules specified, syncs all modules

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

sync_module() {
    local mod="$1"
    local cfile="$mod/$mod.c"
    local cbfile="$mod/.cboot"

    if [ ! -f "$cfile" ] || [ ! -f "$cbfile" ]; then
        echo "skip: $mod (missing .c or .cboot)"
        return 1
    fi

    # Find the line numbers of "code <<EOF" and the first "EOF" after it
    local code_start=$(grep -n "^code <<EOF" "$cbfile" | head -1 | cut -d: -f1)
    if [ -z "$code_start" ]; then
        echo "skip: $mod (no code block found)"
        return 1
    fi

    local eof_line=$(awk "NR>$code_start && /^EOF\$/{print NR; exit}" "$cbfile")
    if [ -z "$eof_line" ]; then
        echo "skip: $mod (no EOF found)"
        return 1
    fi

    # Build new .cboot:
    # 1. Lines 1..code_start (inclusive) from .cboot
    # 2. .c file content (skip first 2 generated lines)
    # 3. Lines from EOF to end from .cboot
    {
        sed -n "1,${code_start}p" "$cbfile"
        tail -n +3 "$cfile"
        sed -n "${eof_line},\$p" "$cbfile"
    } > "$cbfile.tmp"

    mv "$cbfile.tmp" "$cbfile"
    echo "synced: $mod ($cfile -> $cbfile)"
}

if [ $# -eq 0 ]; then
    for dir in */; do
        mod="${dir%/}"
        if [ -f "$mod/$mod.c" ] && [ -f "$mod/.cboot" ]; then
            sync_module "$mod"
        fi
    done
else
    for mod in "$@"; do
        sync_module "$mod"
    done
fi
