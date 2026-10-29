#!/usr/bin/env bash
# listedincheck.sh — LISTED-IN on a --for answer (lane manifest-noncode; src/listedin.h; CONTRIBUTING family 10 slice 10c).
#
# THE CONTRACT THIS PINS (arms tagged [P] are RED on the pre-change binary 251ac97c; [G] arms are twins and near-miss negatives
# that hold on both binaries and must keep holding — PROCESS rule 4, no gate erosion):
#
#   TRIGGER  CLI --for (XML, default regime) whose task has a creation word and whose ranked head shows a file that a build,
#            package or registry manifest at the root or above it lists by a bounded path token.
#   WHAT     <listed p= of= by=path|dir [also=]><ln l=>LINE</ln>…</listed> after the tail, each line byte-exact (XML-escaped)
#            after the less-indented line it sits under.
#   NOT      a task without a creation word, a glob listing, a comment line, --json, RIPWIRE_NO_LISTED_IN=1.
#
# Arms:
#   (D1 holds on 251ac97c — no <listed> at all — and is RED on f8399b64, which matched directory tokens in make files)
#   A1 [P] automake: "new widget" → <listed p="Makefile.am" of="widget-alpha.c" by="path"> with ln l=4 (the SOURCES opener) and l=5
#   A2 [G] a comment line naming a file (`# widget-old.c is gone`) is not a listing: no ln l="3", widget-old.c in no also=
#   A3 [P] package manifests: package.json by="dir" l=8,9 (the ./cors exports key + its build output), jsr.json by="path" l=2,4
#   A4 [G] segment-bounded: src/middleware/log/index.ts is never named by `middleware/logger` lines (absent from of=/also=)
#   A5 [G] a creation word inside an identifier is not one: "addWidget alpha run" and "widget_new_alpha run" carry no <listed>
#   A6 [P] an all-caps creation word is a word, not a camel boundary: "NEW widget" carries <listed p="Makefile.am"
#   S1 [P] a registry needs a sibling: deno.json names ONE shown file (its "exports") and carries no <listed>; package.json names two (also=)
#   D1 [G] a Makefile naming the shown middleware DIRECTORIES as command arguments carries no <listed> (make lists sources by file path)
#   P1 [P] a directory token names that directory, not a deeper one: "./dist/middleware/cors/internal/helpers.js" (l=11) is not printed
#   V1 [G] vendored: a shown file under bower_components/ with its own package.json listing it carries no <listed> for that manifest
#   N1 [G] no creation word ("widget alpha"): no <listed>, no clause
#   N2 [G] a glob listing ($(wildcard *.c)) is a floor: no <listed>
#   N3 [G] prefix identity: the answer minus <listed> and its clause, est_tokens masked, equals RIPWIRE_NO_LISTED_IN=1
#   N4 [G] --json carries no listed
#   L1 [P] the clause rides (compact and full) and defines every attribute <listed>/<ln> carry, as whole names in the clause alone
#   X1 [G] determinism  X2 [G] xmllint well-formed (when xmllint exists)  X3 [G] no "--" inside the legend comment
#
# Usage:  bash test/listedincheck.sh [BIN]   |   RIPWIRE_BIN=asan/ripwire bash test/listedincheck.sh
# BIN is $1, else RIPWIRE_BIN, else build/ripwire; the first output line names the binary used. Exits non-zero on any failure.

set -u
ROOT="$( cd "$( dirname "$0" )/.." && pwd )"
. "$ROOT/test/lib/clean-env.sh"
unset RIPWIRE_NO_LISTED_IN
BIN="${1:-${RIPWIRE_BIN:-$ROOT/build/ripwire}}"
[ "${BIN#/}" = "$BIN" ] && BIN="$ROOT/$BIN"
echo "listedincheck: BIN=$BIN"
fail=0
ok(){ printf '  PASS  %s\n' "$*" || { fail=1; printf '  FAIL  could not write the PASS line for: %s\n' "$*"; }; return 0; }
no(){ printf '  FAIL  %s\n' "$*"; fail=1; }

[ -x "$BIN" ] || { echo "no ripwire binary at $BIN — build first"; exit 2; }
command -v python3 >/dev/null 2>&1 || { echo "python3 required"; exit 2; }
FX="$ROOT/test/listedinfix"
[ -f "$FX/am/Makefile.am" ] && [ -f "$FX/pkg/package.json" ] && [ -f "$FX/pkg/jsr.json" ] && [ -f "$FX/glob/Makefile" ] || { echo "fixture test/listedinfix missing"; exit 2; }
T="$( mktemp -d )"; trap 'rm -rf "$T"' EXIT
H="$T/home"; mkdir -p "$H"

run(){   # dir out task [extra args…]
    local d="$1" o="$2" t="$3"; shift 3
    ( cd "$FX/$d" || exit 1; HOME="$H" "$BIN" "$FX/$d" --no-cache "--for=$t" "$@" </dev/null >"$o" 2>"$o.err" )
    local rc=$?
    [ "$rc" -eq 0 ] && [ -s "$o" ] || { no "run $d '$t' $*: rc=$rc or empty output ($(head -c 200 "$o.err"))"; return 1; }
    return 0
}
has(){ grep -qF -- "$2" "$1"; }

# ---- A1 / A2 / A5
if run am "$T/a1" "new widget"; then
    if has "$T/a1" '<listed p="Makefile.am" of="widget-alpha.c" by="path"' && has "$T/a1" '<ln l="4">dist_demo_SOURCES = \</ln>' && has "$T/a1" '<ln l="5">&#9;widget-alpha.c \</ln>'; then
        ok "A1 Makefile.am lists widget-alpha.c (by=path) with its SOURCES opener l=4 and l=5"
    else no "A1 expected <listed p=\"Makefile.am\" of=\"widget-alpha.c\" by=\"path\"> with ln l=4/l=5: $(grep -o '<listed .*</listed>' "$T/a1" | head -c 400)"; fi
    if ! has "$T/a1" '<ln l="3">' && ! grep -o '<listed [^>]*>' "$T/a1" | grep -qF 'widget-old.c'; then ok "A2 a comment line naming widget-old.c is not a listing"
    else no "A2 the comment line was taken as a listing"; fi
fi
for t5 in "addWidget alpha run" "widget_new_alpha run"; do   # [G] identifiers; A6 below is the all-caps twin that DOES fire
    if run am "$T/a5" "$t5"; then
        if ! has "$T/a5" '<listed'; then ok "A5 identifier subtoken is not a creation word ($t5)"; else no "A5 '$t5' fired <listed>"; fi
    fi
done

if run am "$T/a6" "NEW widget"; then
    if has "$T/a6" '<listed p="Makefile.am"'; then ok "A6 an all-caps creation word (NEW widget) fires"; else no "A6 'NEW widget' carried no <listed>"; fi
fi

# ---- A3 / A4
if run pkg "$T/a3" "new middleware"; then
    if has "$T/a3" '<listed p="package.json" of="src/middleware/cors/index.ts" by="dir" also="src/middleware/logger/index.ts"' && has "$T/a3" '<ln l="8">    &quot;./cors&quot;: {</ln>' \
       && has "$T/a3" '<ln l="9">      &quot;import&quot;: &quot;./dist/middleware/cors/index.js&quot;</ln>'; then ok "A3 package.json exports stanza by=dir l=8,9 (logger in also=)"
    else no "A3 package.json stanza missing: $(grep -o '<listed .*</listed>' "$T/a3" | head -c 500)"; fi
    if has "$T/a3" '<listed p="jsr.json" of="src/middleware/cors/index.ts" by="path"' && has "$T/a3" '<ln l="2">  &quot;exports&quot;: {</ln>' \
       && has "$T/a3" '<ln l="4">    &quot;./cors&quot;: &quot;./src/middleware/cors/index.ts&quot;</ln>'; then ok "A3 jsr.json exports line by=path l=2,4"
    else no "A3 jsr.json line missing"; fi
    if ! grep -o '<listed [^>]*>' "$T/a3" | grep -qF 'middleware/log/index.ts'; then ok "A4 middleware/log is not named by middleware/logger lines"
    else no "A4 segment bound broken: middleware/log/index.ts listed"; fi
fi

if ! has "$T/a3" '<listed p="deno.json"' && grep -o '<listed p="package.json"[^>]*>' "$T/a3" | grep -qF 'also="src/middleware/logger/index.ts'; then ok "S1 one-file manifest (deno.json) not printed; package.json carries its sibling in also="
else no "S1 sibling rule: $(grep -o '<listed [^>]*>' "$T/a3")"; fi
if has "$T/a3" '<listed p="package.json"' && ! has "$T/a3" '<ln l="11">'; then ok "P1 a deeper directory under the shown one is not its listing"; else no "P1 deeper-directory line printed or package.json missing"; fi
if ! has "$T/a3" '<listed p="Makefile"'; then ok "D1 a Makefile's directory arguments are not a listing"; else no "D1 Makefile directory arguments were listed"; fi
if grep -o '<listed [^>]*>' "$T/a3" | grep -qF 'bower_components'; then no "V1 a vendored copy's own manifest was listed"; else ok "V1 vendored bower_components/ manifest not listed"; fi

# ---- N1 / N2 / N4
if run am "$T/n1" "widget alpha"; then
    if ! has "$T/n1" '<listed' && ! has "$T/n1" '; listed p='; then ok "N1 no creation word: no <listed>, no clause"; else no "N1 <listed> on a non-creation task"; fi
fi
if run glob "$T/n2" "new widget"; then
    if ! has "$T/n2" '<listed'; then ok "N2 a glob listing is a floor: no <listed>"; else no "N2 a glob listing matched"; fi
fi
if run am "$T/n4" "new widget" --json; then
    if ! has "$T/n4" 'listed'; then ok "N4 --json carries no listed"; else no "N4 --json carried listed"; fi
fi

# ---- N3 prefix identity
if ( cd "$FX/am" && HOME="$H" RIPWIRE_NO_LISTED_IN=1 "$BIN" "$FX/am" --no-cache "--for=new widget" </dev/null >"$T/n3" 2>/dev/null ); then
    if python3 - "$T/a1" "$T/n3" <<'EOF'
import re, sys
a = open(sys.argv[1], encoding="utf-8").read(); b = open(sys.argv[2], encoding="utf-8").read()
a = re.sub(r'<listed .*?</listed>', '', a); a = re.sub(r'; listed p= of= by=path\|dir:.*?if cut', '', a)
m = lambda s: re.sub(r'est_tokens="\d+"', 'est_tokens=""', s)
sys.exit(0 if m(a) == m(b) else 1)
EOF
    then ok "N3 answer minus <listed> and clause == RIPWIRE_NO_LISTED_IN=1"; else no "N3 the rest of the answer moved"; fi
else no "N3 RIPWIRE_NO_LISTED_IN=1 run failed"; fi

# ---- L1 legend coverage, compact and full
if run pkg "$T/l1" "new middleware" --legend=full; then
    for f in "$T/a3" "$T/l1"; do
        if python3 - "$f" <<'EOF'
import re, sys
s = open(sys.argv[1], encoding="utf-8").read()
m = re.search(r'; listed p= of= by=.*?(?:\(a floor\)|if cut)', s)
if not m: print("    no listed clause"); sys.exit(1)
clause = m.group(0)
attrs = set()
for el in re.findall(r'<(?:listed|ln) [^>]*>', s):
    attrs.update(re.findall(r' ([a-z_]+)=', el))
miss = [a for a in sorted(attrs) if not re.search(r'(?<![a-z_])' + a + r'=', clause)]
if miss: print("    undefined:", miss); sys.exit(1)
EOF
        then ok "L1 the listed clause defines every attribute ($( basename "$f" ))"; else no "L1 clause misses attributes ($( basename "$f" ))"; fi
    done
fi

# ---- X1 / X2 / X3
if run am "$T/x1" "new widget" && cmp -s "$T/a1" "$T/x1"; then ok "X1 deterministic"; else no "X1 two runs differ"; fi
if command -v xmllint >/dev/null 2>&1; then
    if xmllint --noout "$T/a3" 2>/dev/null && xmllint --noout "$T/l1" 2>/dev/null; then ok "X2 well-formed (compact + full)"; else no "X2 xmllint failed"; fi
else echo "  SKIP  X2 xmllint not installed"; fi
if python3 -c "
import re,sys
for f in sys.argv[1:]:
    s=open(f,encoding='utf-8').read()
    for c in re.findall(r'<!--(.*?)-->', s, re.S):
        if '--' in c: sys.exit(1)
" "$T/a3" "$T/l1"; then ok "X3 no -- inside legend comments"; else no "X3 -- inside a legend comment"; fi

[ "$fail" -eq 0 ] && echo "listedincheck: PASS" || echo "listedincheck: FAIL"
exit "$fail"
