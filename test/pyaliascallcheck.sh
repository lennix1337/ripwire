#!/usr/bin/env bash
# pyaliascallcheck.sh — PY-IMPORT-ALIAS-CALL: a bare Python call through a RENAMED member import
# (`from m import f as g` then `g()`) reaches m's `f`, decided by src/graph.h pythonRenamedImportPin
# from the Import record the shared binds channel writes (src/ingest_importcap.h emitCapturedImportBind,
# CONTRIBUTING family 2 slice 2a: importedName "from:f").
#
# Before the fix the call kept only its local spelling: no definition is named `g` in m, so the name
# ladder bound it to an unrelated decoy.py `g` (via="name") and m's `f` had no caller.
#
# Fixture test/pyaliascallfix (decoy.py defines BOTH a decoy `f` and a decoy `g`; m2.py another `f`):
#   POSITIVE (target m.py:1 f, a proven edge: the row carries no via="name" hedge; internally prov="import")
#     c_plain.py     from m import f as g ; g()                         the defect's own shape
#     c_cond.py      if X: from m import f as g ; g()                   one conditional import: the only target it can bind
#     c_class.py     from m import Widget as Gadget ; Gadget()          a class (target m.py:9 Widget)
#   NEAR MISSES (must NOT bind m.py's f)
#     c_param.py     def call_param(g): g()                             a parameter shadows the import
#     c_local.py     g = other ; g()                                    a local assignment shadows it
#     c_rebind.py    g = len (module scope) ; g()                       a later module-scope rebinding
#     c_dynamic.py   globals()["g"] = len                               a dynamic namespace write may rebind any name
#     c_star.py      … from starsrc import * ; g()                      a star import may bind g after the rename
#     c_twomod.py    try m / except: m2, both `f as g`                  two modules: never chosen
#     c_fnimport.py  the import sits in another def                     not bound where the call runs
#     c_inner.py     outer def binds g ; inner def calls g()            the closure reads the enclosing g
#     c_disagree.py  from m import f as g ; from m import h as g        two members under one name
#     c_reexport.py  from pkg import f as g (pkg re-exports _impl.f)    no definition in pkg/__init__.py: no decision (a floor)
#     c_external.py  from json import dumps as d (jsonish.py has a dumps)  outside the tree: never an in-tree dumps
#     c_twice.py     m3.py defines f twice                              which one is live is not read: no decision
#     c_nested.py    from m import method as meth                       m defines `method` only inside class Widget: not a module name
#   UNCHANGED
#     c_same.py      from m import h as h ; h()                         alias equals the name: the ladder, as before
#
# Usage: bash test/pyaliascallcheck.sh [BIN]     (default build/ripwire; exits non-zero on any failure)
set -u
ROOT="$( cd "$( dirname "$0" )/.." && pwd )"
. "$ROOT/test/lib/clean-env.sh"
BIN="${1:-${RIPWIRE_BIN:-$ROOT/build/ripwire}}"
[ "${BIN#/}" = "$BIN" ] && BIN="$ROOT/$BIN"
FIX="$ROOT/test/pyaliascallfix"
TMP="$( mktemp -d )"; trap 'rm -rf "$TMP"' EXIT
fail=0
ok(){ printf '  PASS  %s\n' "$*" || { fail=1; printf '  FAIL  could not write the PASS line for: %s\n' "$*"; }; return 0; }
no(){ printf '  FAIL  %s\n' "$*"; fail=1; }

[ -x "$BIN" ] || { echo "FAIL: no binary at $BIN"; exit 1; }
echo "pyaliascallcheck: $( "$BIN" --version 2>/dev/null )"

callees(){ ( cd "$FIX" || exit 1; "$BIN" "$FIX" --no-cache --callees="$1" --legend=compact ) 2>/dev/null >"$TMP/$1"; }

# ── 0. presence: every fixture file parses (a fixture that fails to parse would pass the near misses inert) ──
( cd "$FIX" || exit 1; "$BIN" "$FIX" --no-cache --skipped --legend=compact ) >"$TMP/skipped" 2>/dev/null
perl -pe 's/<!--.*?-->//g' "$TMP/skipped" >"$TMP/skipped.body"
if ! grep -q 'degraded_parse="0"' "$TMP/skipped.body" || ! grep -q 'indexed="25"' "$TMP/skipped.body" || grep -q '<f \|<h ' "$TMP/skipped.body"; then no "a fixture file was skipped or degraded"; sed 's/^/        | /' "$TMP/skipped" | head -5
else ok "every fixture file indexed with no parse degrade"; fi

# ── 1. positive arms ──
for c in call_plain call_cond; do
    callees "$c"
    row=$( grep -o '<s [^>]*p="m.py:1"[^>]*/>' "$TMP/$c" )
    if [ -n "$row" ] && ! printf '%s' "$row" | grep -q 'via=' && ! grep -q 'p="decoy.py' "$TMP/$c"; then
        ok "$c binds m.py:1 f as a proven edge (no via=\"name\" hedge) and no decoy row"
    else
        no "$c: expected m.py:1 f (proven, no via=) and no decoy row"; sed 's/<!--.*-->//; s/^/        | /' "$TMP/$c"
    fi
done
callees call_class
if grep -q '<s t="cls" n="Widget" p="m.py:9"/>' "$TMP/call_class"; then ok "call_class binds m.py:9 class Widget as a proven edge"
else no "call_class: expected m.py:9 Widget"; sed 's/<!--.*-->//; s/^/        | /' "$TMP/call_class"; fi
( cd "$FIX" || exit 1; "$BIN" "$FIX" --no-cache --callers=m.py:f --legend=compact ) >"$TMP/callers" 2>/dev/null
if grep -q 'n="call_plain"' "$TMP/callers" && grep -q 'n="call_cond"' "$TMP/callers"; then ok "--callers=m.py:f names call_plain and call_cond"
else no "--callers=m.py:f misses an aliased caller"; sed 's/<!--.*-->//; s/^/        | /' "$TMP/callers"; fi

# ── 2. near misses: never m.py's f ──
for c in call_param call_local call_rebind call_dynamic call_star call_twomod call_fnimport call_inner call_disagree call_reexport call_nested call_external call_twice; do
    callees "$c"
    if grep -q 'p="m.py:1"\|p="m2.py:1"\|p="m.py:10"\|p="m3.py\|p="jsonish.py' "$TMP/$c"; then
        no "$c: bound through the renamed import (must stay undecided)"; sed 's/<!--.*-->//; s/^/        | /' "$TMP/$c"
    else ok "$c: no import-pinned edge"; fi
done

# ── 3. unchanged: alias equal to the name keeps the ladder (h in m.py, the edge it had before) ──
callees call_same
if grep -q '<s t="fn" n="h" p="m.py:5"/>' "$TMP/call_same"; then ok "call_same: the same ladder edge to m.py:5 h as before"
else no "call_same changed"; sed 's/<!--.*-->//; s/^/        | /' "$TMP/call_same"; fi

# ── 4. hygiene: determinism and warm == cold ──
( cd "$FIX" || exit 1; "$BIN" "$FIX" --no-cache --callers=f ) >"$TMP/d1" 2>/dev/null
( cd "$FIX" || exit 1; "$BIN" "$FIX" --no-cache --callers=f ) >"$TMP/d2" 2>/dev/null
if cmp -s "$TMP/d1" "$TMP/d2"; then ok "deterministic"; else no "non-deterministic"; fi
( cd "$FIX" || exit 1; "$BIN" "$FIX" --cache="$TMP/c.bin" --callers=f ) >"$TMP/cold" 2>/dev/null
( cd "$FIX" || exit 1; "$BIN" "$FIX" --cache="$TMP/c.bin" --callers=f ) >"$TMP/warm" 2>/dev/null
if cmp -s "$TMP/cold" "$TMP/warm" && cmp -s "$TMP/cold" "$TMP/d1"; then ok "warm == cold == --no-cache"; else no "cache changes the answer"; fi

[ "$fail" -eq 0 ] && echo "ALL PASS" || { echo "SOME CHECKS FAILED"; exit 1; }
