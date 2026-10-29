#!/usr/bin/env bash
# forliteralcheck.sh — THE LITERAL ANCHOR of a --for answer whose task names a literal (lane MF1-LIT; src/forliteral.h).
#
# THE CONTRACT THIS PINS (arms tagged [P] are RED on the pre-change binary 251ac97c; [G] arms are twins and near-miss negatives
# that hold on both binaries and must keep holding — PROCESS rule 4, no gate erosion):
#
#   TRIGGER  CLI --for (XML, default regime, one root) whose task carries a "double-quoted message" (two words or more) or a
#            CONST_SHAPED word (APPTOOL_RENDER_MODE, PYTHONASYNCIODEBUG).
#   WHAT     a legend comment + <lit s= shape= hits= src= shown= comment= test= doc= next="--grep=S" bytes=> just before </ctx>:
#            <ls p= l= k=string|code|? in= ink=> source sites (sites in a function first), <b> owner bodies, <x> excerpts.
#   NOT      a comment, a docstring, a test path or a doc file is never an <ls> row (counted only); no literal, no hit,
#            --json and --token-budget carry none (byte-identical to RIPWIRE_NO_LITERAL_ANCHOR=1).
#
# Arms:
#   L1 [P] --for=APPTOOL_RENDER_MODE: shape="const"; config.py k="string" in="read_mode", runner.py in="debug_mode" and the
#          a_table.py rows (src="3", several sites listed, never one picked); the read_mode body is printed whole
#   L1b [P] the first row is the site inside a function (config.py), not a_table.py, which the scan's path order puts first
#   L1c [P] the a_table.py string site (outside any function) carries lines="3-5" and the entry after it
#   L2 [P] the counts: comment="2" (a # comment and a docstring), test="1", doc="1"; none of those files' lines is an <ls> row
#   L3 [P] a quoted message ("dependency cycle detected" error): shape="quoted", the check_cycle row; the ranked answer already
#          prints check_cycle's body, so the row says body="above" and the body appears exactly once
#   L4 [P] a body over the cap (big_dispatch): an <x> excerpt holding the site line, of= the body's lines, next="--expand=…big_dispatch"
#   L5 [G] no literal: a lower-case word, a ONE-word quoted value ("full", and "detected", which a source string holds), a
#          backticked span, a 4-byte CONST (ABC_D): no <lit
#   L6 [G] a CONST_SHAPED literal with no hit: no <lit (never a "the literal is unused" claim)
#   L6b [P] a literal met only in a test path and a README (APPTOOL_TEST_ONLY): no <lit (counts alone anchor nothing)
#   L7 [G] prefix identity: the answer minus the legend comment and <lit>…</lit> equals RIPWIRE_NO_LITERAL_ANCHOR=1 byte for byte
#   L8 [G] --json and --token-budget=3000: no <lit, byte-identical to RIPWIRE_NO_LITERAL_ANCHOR=1
#   L9 [G] bytes= is the element's own size (open tag to </lit>)
#   X1 [G] determinism  X2 [G] xmllint well-formed  X3 [G] no "--" inside the lit legend comment
#
# Usage:  bash test/forliteralcheck.sh [BIN]   |   RIPWIRE_BIN=asan/ripwire bash test/forliteralcheck.sh
# BIN is $1, else RIPWIRE_BIN, else build/ripwire; the first output line names the binary used. Exits non-zero on any failure.

set -u
ROOT="$( cd "$( dirname "$0" )/.." && pwd )"
. "$ROOT/test/lib/clean-env.sh"
unset RIPWIRE_NO_LITERAL_ANCHOR
BIN="${1:-${RIPWIRE_BIN:-$ROOT/build/ripwire}}"
[ "${BIN#/}" = "$BIN" ] && BIN="$ROOT/$BIN"
echo "forliteralcheck: BIN=$BIN"
fail=0
ok(){ printf '  PASS  %s\n' "$*" || { fail=1; printf '  FAIL  could not write the PASS line for: %s\n' "$*"; }; return 0; }
no(){ printf '  FAIL  %s\n' "$*"; fail=1; }
[ -x "$BIN" ] || { no "binary not executable: $BIN"; exit 1; }
TMP="$( mktemp -d )"; trap 'rm -rf "$TMP"' EXIT
export HOME="$TMP/home" TMPDIR="$TMP/"; mkdir -p "$HOME"
FIX="$ROOT/test/forliteralfix"
run(){ ( cd "$FIX" || exit 1; "$BIN" "$FIX" --no-cache "$@" </dev/null 2>/dev/null ); }
off(){ ( cd "$FIX" || exit 1; RIPWIRE_NO_LITERAL_ANCHOR=1 "$BIN" "$FIX" --no-cache "$@" </dev/null 2>/dev/null ); }
lit(){ python3 -I -c 'import sys;s=sys.stdin.read();i=s.find("<lit ");print(s[i:s.find("</lit>",i)+6] if i>=0 else "")'; }
has(){ grep -qF -- "$2" <<<"$1"; }

# ── L1 ──
A="$( run --for=APPTOOL_RENDER_MODE )"; rc=$?
L="$( lit <<<"$A" )"
if [ "$rc" -eq 0 ] && [ -n "$L" ] && has "$L" 's="APPTOOL_RENDER_MODE" shape="const"' && has "$L" 'src="3"' \
   && has "$L" '<ls p="app/config.py" l="5" k="string" in="read_mode" ink="fn"' && has "$L" 'in="debug_mode"' \
   && has "$L" '<b t="fn" l="4" p="app/config.py" n="read_mode">'; then
    ok "L1 const literal: every source site listed (src=3), read_mode body printed"
else no "L1 const literal block wrong (rc=$rc): ${L:0:600}"; fi
# ── L1b / L1c ──
first="$( grep -o '<ls p="[^"]*" l="[0-9]*"' <<<"$L" | head -1 )"
if [ "$first" = '<ls p="app/config.py" l="5"' ]; then ok "L1b a site inside a function is the first row (a_table.py sorts earlier by path)"
else no "L1b first row is not the function site: $first"; fi
if has "$L" '<ls p="app/a_table.py" l="3" k="string" in="MODES" ink="var" lines="3-5">' && has "$L" '"OTHER": "flag",'; then
    ok "L1c a string site outside any function carries the lines after it (lines=3-5)"
else no "L1c table row context wrong: ${L:0:600}"; fi
# ── L2 ──
if has "$L" 'comment="2"' && has "$L" 'test="1"' && has "$L" 'doc="1"' && ! has "$L" 'p="tests/' && ! has "$L" 'p="README.md"' \
   && ! has "$L" 'in="helper"' && ! has "$L" 'l="7" k='; then
    ok "L2 comment/docstring/test/doc sites counted (2/1/1), never an <ls> row"
else no "L2 counts or rows wrong: ${L:0:600}"; fi
# ── L3 ──
QA="$( run '--for="dependency cycle detected" error' )"; Q="$( lit <<<"$QA" )"
nb="$( grep -o 'n="check_cycle"><!\[CDATA\[' <<<"$QA" | wc -l | tr -d ' ' )"
if has "$Q" 's="dependency cycle detected" shape="quoted"' && has "$Q" 'in="check_cycle" ink="fn" body="above"' && [ "$nb" = "1" ]; then
    ok "L3 quoted message: check_cycle row; its body, already in the answer, is marked body=above and printed once"
else no "L3 quoted literal block wrong: ${Q:0:600}"; fi
# ── L4 ──
X="$( run --for=APPTOOL_BIG_FLAG | lit )"
if has "$X" '<x p="app/big.py" n="big_dispatch"' && has "$X" 'of="1-' && has "$X" 'next="--expand=app/big.py:1:big_dispatch"' \
   && has "$X" 'environ.get("APPTOOL_BIG_FLAG")' && ! has "$X" '<b t="fn" l="1" p="app/big.py"'; then
    ok "L4 a body over the cap is an excerpt around the site with next= the whole body"
else no "L4 excerpt wrong: ${X:0:600}"; fi
# ── L5 / L6 ──
neg=0
for t in 'apptool_render_mode' '"full" mode' 'the `APPTOOL` switch' 'ABC_D toggle' 'NOPE_NOT_HERE_ANYWHERE' 'APPTOOL_TEST_ONLY' '"detected" banner'; do
    o="$( run "--for=$t" )"
    if has "$o" '<lit '; then neg=1; no "L5/L6 a block rode on: $t"; fi
done
if [ "$neg" -eq 0 ]; then
    ok "L5/L6/L6b no block: lower-case word, one-word quotes, backticked word, short CONST, CONST with no hit, test/doc-only CONST"
fi
# ── L7 ──
for t in APPTOOL_RENDER_MODE '"dependency cycle detected" error' APPTOOL_BIG_FLAG; do
    a="$( run "--for=$t" | python3 -I -c 'import sys,re;s=sys.stdin.read();print(re.sub(r"<!-- ripwire for lit:.*?-->(<lit .*?</lit>)","",s,flags=re.S),end="")' )"
    b="$( off "--for=$t" )"
    if [ -n "$b" ] && [ "$a" = "$b" ]; then ok "L7 prefix identity vs RIPWIRE_NO_LITERAL_ANCHOR=1: $t"; else no "L7 answer outside the block changed: $t"; fi
done
# ── L8 ──
for f in --json --token-budget=3000; do
    a="$( run --for=APPTOOL_RENDER_MODE "$f" )"; b="$( off --for=APPTOOL_RENDER_MODE "$f" )"
    if [ -n "$a" ] && [ "$a" = "$b" ] && ! has "$a" '<lit '; then ok "L8 $f: no block, byte-identical"; else no "L8 $f carries a block or differs"; fi
done
# ── L9 ──
if python3 -I -c 'import sys,re;s=sys.argv[1];m=re.search(r" bytes=\"(\d+)\"",s);sys.exit(0 if m and int(m.group(1))==len(s.encode()) else 1)' "$L"; then
    ok "L9 bytes= is the element's own size"
else no "L9 bytes= disagrees with the element"; fi
# ── X1-X3 ──
if [ "$A" = "$( run --for=APPTOOL_RENDER_MODE )" ]; then ok "X1 deterministic"; else no "X1 two runs differ"; fi
if command -v xmllint >/dev/null 2>&1; then
    if printf '%s' "$A" | xmllint --noout - 2>/dev/null; then ok "X2 xmllint well-formed"; else no "X2 not well-formed"; fi
else ok "X2 SKIP: xmllint not installed"; fi
C="$( python3 -I -c 'import sys,re;m=re.search(r"<!-- ripwire for lit:(.*?)-->",sys.stdin.read(),re.S);print(m.group(1) if m else "MISSING")' <<<"$A" )"
if [ "$C" != "MISSING" ] && ! has "$C" '--'; then ok "X3 the lit legend comment holds no --"; else no "X3 legend comment missing or holds --"; fi

[ "$fail" -eq 0 ] && echo "forliteralcheck: OK" || echo "forliteralcheck: FAILED"
exit "$fail"
