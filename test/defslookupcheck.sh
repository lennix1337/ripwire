#!/usr/bin/env bash
# defslookupcheck.sh — `--defs=NAME[,NAME…]` (src/defslookup.h rw::lookupDefs) and --path's inline <alts> (lane D2 DEFS-LOOKUP).
#
# RED-FIRST (recorded 2026-10-28 against the train-40 base 1318069a): every arm below fails there — the base has no --defs
# flag (rc=1 "unknown flag") and its --path answer carries no <alts>; the (P2) unique-pick negative passes on both.
# Arms: namesakes across files with a test copy and a JSON key (A), a C prototype folded with its definition (B), every
# printed sel= re-resolved by another verb to one definition (C), file:name / Owner.name spellings (D), zero found (E), over
# the 16-row cap with --offset paging (F), several names in one call (G), the JSON twin spelling every row as the XML does (J),
# --path alternatives present only when the pick is not provably unique (P).
#
# Usage:  test/defslookupcheck.sh [BIN]   (BIN falls back to RIPWIRE_BIN, then build/ripwire). Exits non-zero on any failure.

set -u
ROOT="$( cd "$( dirname "$0" )/.." && pwd )"
BIN="${1:-${RIPWIRE_BIN:-$ROOT/build/ripwire}}"
[ "${BIN#/}" = "$BIN" ] && BIN="$ROOT/$BIN"
FIX="$ROOT/test/defslookupfix"
fail=0
ok(){ printf '  PASS  %s\n' "$*"; }
no(){ printf '  FAIL  %s\n' "$*"; fail=1; }
[ -x "$BIN" ] || { echo "no ripwire binary at $BIN — build first (cmake --build build -j)"; exit 2; }
echo "defslookupcheck: BIN=$BIN  FIX=$FIX"
TMP="$( mktemp -d )"; trap 'rm -rf "$TMP"' EXIT
run(){ ( cd "$FIX" && "$BIN" "$FIX" --no-cache "$@" ); }
# the answer without its legend comments (the compact legend itself spells `<name q= defs=>` and `<d n= …>`)
body(){ perl -0pe 's/<!--.*?-->//gs' "$1"; }

echo "-- (A) namesakes: two source methods, one test copy, one JSON key"
run --defs=run --legend=full >"$TMP/a.xml" 2>"$TMP/a.err"; rc=$?
name="$( body "$TMP/a.xml" | grep -o '<name [^>]*>' )"
[ "$rc" -eq 0 ] && printf '%s' "$name" | grep -q 'defs="3"' && ok "defs=3 (two classes + the test copy)" || no "defs= not 3 (rc=$rc): $name"
printf '%s' "$name" | grep -q 'data_defs="1"' && ok "the JSON key is data_defs=1" || no "no data_defs=1: $name"
grep -q 'lang="json"' "$TMP/a.xml" && no "a JSON key is listed as a definition" || ok "no row is a JSON key"
classes="$( grep -o 'class="[a-z]*"' "$TMP/a.xml" | tr '\n' ' ' )"
[ "$classes" = 'class="source" class="source" class="test" ' ] && ok "source rows before the test row" || no "row order by class wrong: $classes"
xmllint --noout "$TMP/a.xml" 2>/dev/null && ok "--legend=full answer is well-formed XML" || no "--legend=full answer is not well-formed"
grep -q 'sel_proven=' "$TMP/a.xml" && no "a sel= was not proven: $( grep -o '<d [^>]*>' "$TMP/a.xml" )" || ok "every sel= proven"

echo "-- (B) a C prototype and its definition are one group"
run --defs=work >"$TMP/b.xml" 2>/dev/null
grep -q '<name q="work" defs="1">' "$TMP/b.xml" && grep -q 'p="src/work.c:2"' "$TMP/b.xml" && ok "work: one group, the bodied definition" || no "work not one group: $( grep -o '<name.*' "$TMP/b.xml" | head -c 300 )"

echo "-- (C) every printed sel= re-resolves to exactly one definition group in another verb (no multi-group <def> table)"
n=0; bad=0
for s in $( grep -o 'sel="[^"]*"' "$TMP/a.xml" "$TMP/b.xml" | sed 's/.*sel="//; s/"$//' ); do
    n=$(( n + 1 ))
    out="$( run --callers="$s" 2>/dev/null )"
    { printf '%s' "$out" | grep -q '<callers ' && ! printf '%s' "$out" | grep -q '<def ix='; } || { bad=$(( bad + 1 )); echo "    sel=$s -> $( printf '%s' "$out" | grep -o '<callers [^>]*>' | head -c 200 )"; }
done
[ "$n" -ge 4 ] && [ "$bad" -eq 0 ] && ok "$n sel= spellings each bind one definition group" || no "$bad of $n sel= spellings did not bind one definition group"

echo "-- (D) spellings the shared resolver accepts"
run --defs=src/b.ts:run >"$TMP/d1.xml" 2>/dev/null
grep -q 'defs="1"' "$TMP/d1.xml" && grep -q 'sel="src/b.ts:run"' "$TMP/d1.xml" && ok "file:name -> one row, sel= as given" || no "file:name: $( grep -o '<name.*' "$TMP/d1.xml" | head -c 300 )"
run --defs=A.run >"$TMP/d2.xml" 2>/dev/null
grep -q 'defs="1"' "$TMP/d2.xml" && grep -q 'n="A.run"' "$TMP/d2.xml" && ok "Owner.name -> one row" || no "Owner.name: $( grep -o '<name.*' "$TMP/d2.xml" | head -c 300 )"

echo "-- (E) zero found: the shared refusal"
run --defs=zzqdefsnone >"$TMP/e.xml" 2>"$TMP/e.err"; rc=$?
[ "$rc" -eq 1 ] && grep -q 'found="0"' "$TMP/e.xml" && grep -q 'symbol not found' "$TMP/e.err" && ok "rc=1, found=0, stderr refusal" || no "zero found: rc=$rc $( head -c 300 "$TMP/e.xml" )"

echo "-- (F) over the cap: 16 rows, defs= the true count, --offset pages"
run --defs=many >"$TMP/f1.xml" 2>/dev/null
rows="$( body "$TMP/f1.xml" | grep -o '<d ' | wc -l | tr -d ' ' )"
grep -q 'defs="18" shown="16" offset="0" next="--defs=many --offset=16"' "$TMP/f1.xml" && [ "$rows" = 16 ] && ok "page 1: 16 rows of 18, next= names page 2" || no "page 1: rows=$rows $( grep -o '<name [^>]*>' "$TMP/f1.xml" )"
run --defs=many --offset=16 >"$TMP/f2.xml" 2>/dev/null
rows2="$( body "$TMP/f2.xml" | grep -o '<d ' | wc -l | tr -d ' ' )"
[ "$rows2" = 2 ] && ! body "$TMP/f2.xml" | grep -q 'next=' && ok "page 2: the last 2 rows, no next=" || no "page 2: rows=$rows2"
cat "$TMP/f1.xml" "$TMP/f2.xml" | grep -o 'p="gen/m[0-9]*.ts:1"' | sort -u | wc -l | grep -q '^ *18$' && ok "the two pages cover all 18, no repeats" || no "pages do not cover 18 distinct definitions"

run --defs=many --limit=5 --offset=5 >"$TMP/f3.xml" 2>/dev/null
grep -q 'defs="18" shown="5" offset="5" next="--defs=many --offset=10 --limit=5"' "$TMP/f3.xml" && [ "$( body "$TMP/f3.xml" | grep -o '<d ' | wc -l | tr -d ' ' )" = 5 ] \
  && ok "--limit=5 --offset=5: rows 5..9, next= keeps the limit" || no "--limit page: $( grep -o '<name [^>]*>' "$TMP/f3.xml" | tail -1 )"
run --defs=many,run --offset=3 >/dev/null 2>"$TMP/f4.err"; rc=$?
[ "$rc" -eq 1 ] && grep -q -- '--offset' "$TMP/f4.err" && ok "--offset beside several names refuses (never ignored)" || no "--offset with several names: rc=$rc"

echo "-- (G) several names, one call"
run --defs=run,work,zzqdefsnone >"$TMP/g.xml" 2>/dev/null; rc=$?
[ "$rc" -eq 0 ] && [ "$( body "$TMP/g.xml" | grep -o '<name q=' | wc -l | tr -d ' ' )" = 3 ] && grep -q '<name q="zzqdefsnone" defs="0" next="--defs=zzqdefsnone">' "$TMP/g.xml" \
  && ok "3 <name> rows; the missing one defs=0 with its refusal as next=" || no "multi-name: rc=$rc $( grep -o '<name [^>]*>' "$TMP/g.xml" | tr '\n' ' ' )"

echo "-- (J) the JSON twin spells every row as the XML does (checklist 41)"
run --defs=run --json >"$TMP/j.json" 2>/dev/null
python3 - "$TMP/j.json" "$TMP/a.xml" <<'PY' && ok "JSON rows equal the XML rows (n, p, sel, class, in)" || no "JSON twin differs from XML"
import json, re, sys
j = json.load( open( sys.argv[1] ) )["names"][0]["rows"]
x = re.findall( r'<d n="([^"]*)" t="[^"]*" p="([^"]*)" sel="([^"]*)" lang="[^"]*" class="([^"]*)" in="([0-9]+)"', open( sys.argv[2] ).read() )
sys.exit( 0 if [ ( r["n"], r["p"], r["sel"], r["class"], str( r["in"] ) ) for r in j ] == x and len( x ) == 3 else 1 )
PY

echo "-- (P) --path lists the other definitions only when the pick is not provably unique"
run --path=run,helperA >"$TMP/p1.xml" 2>/dev/null
alts="$( grep -o '<alts .*</alts>' "$TMP/p1.xml" )"
printf '%s' "$alts" | grep -q '^<alts side="from" of="run">' && [ "$( printf '%s' "$alts" | grep -o '<d ' | wc -l | tr -d ' ' )" = 2 ] \
  && printf '%s' "$alts" | grep -q 'sel="src/b.ts:run"' && ! printf '%s' "$alts" | grep -q 'p="src/a.ts:2"' \
  && ok "P1: from=run binds A.run; <alts> lists the 2 others (B.run, the test copy), not the bound one" || no "P1: $alts"
run --path=A.run,helperA >"$TMP/p2.xml" 2>/dev/null
grep -q 'reachable="1"' "$TMP/p2.xml" && ! grep -q '<alts' "$TMP/p2.xml" && ok "P2: a unique pick prints no <alts> (0 bytes)" || no "P2: $( head -c 400 "$TMP/p2.xml" )"
run --path=many,helperA --legend=full >"$TMP/p3.xml" 2>/dev/null
grep -q '<alts side="from" of="many" alts_shown="16" defs="18" next="--defs=many">' "$TMP/p3.xml" && xmllint --noout "$TMP/p3.xml" 2>/dev/null \
  && ok "P3: 17 others past the cap: alts_shown=16 defs=18 next=--defs=many, well-formed" || no "P3: $( grep -o '<alts [^>]*>' "$TMP/p3.xml" )"

[ "$fail" -eq 0 ] && echo "defslookupcheck: OK" || echo "defslookupcheck: FAILURES above"
exit "$fail"
