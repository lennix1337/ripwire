#!/usr/bin/env bash
# jsdefnamingcheck.sh — JS/TS definitions carry the names the source gives them (lane js-def-naming):
#   (1) a JS/TS method row prints the class whose body holds it (memberowner.h, by graph.h containedOwnersOf — the rule the
#       written-spelling tier binds `Class.method` by), so two same-file namesakes read Environment.render / Template.render;
#       and a printed p= (`file:LINE`, a definition's own signature line) is a selector, tried only after every name tier;
#   (2) a module-scope object literal records its binding name (model.h LocalBindKind::JsObjectLiteral), so `Widget.run`
#       beside both `const Widget = { run() {} }` and `class Widget { run() {} }` is unknown, never the class alone;
#   (3) a bare `module.exports = <anonymous fn>` is a function named by the file's stem (`is-stream`).
#
# RED-FIRST (recorded 2026-10-27 against the base binary built_from=bbbb5c5c1): every arm marked RED failed there; every
# KEEP arm already passed. Each target calls its own helper, which is the discriminator the arms grep for.
#
# Usage:  test/jsdefnamingcheck.sh [BIN]   (BIN falls back to RIPWIRE_BIN, then build/ripwire). Exits non-zero on any failure.

set -u
ROOT="$( cd "$( dirname "$0" )/.." && pwd )"
BIN="${1:-${RIPWIRE_BIN:-$ROOT/build/ripwire}}"
[ "${BIN#/}" = "$BIN" ] && BIN="$ROOT/$BIN"
FIX="$ROOT/test/jsdefnamingfix"
fail=0
ok(){ printf '  PASS  %s\n' "$*" || { fail=1; printf '  FAIL  could not write the PASS line for: %s\n' "$*"; }; return 0; }
no(){ printf '  FAIL  %s\n' "$*"; fail=1; }

[ -x "$BIN" ] || { echo "no ripwire binary at $BIN — build first (cmake --build build -j)"; exit 2; }
echo "jsdefnamingcheck: BIN=$BIN  FIX=$FIX"

run(){ local sub="$1"; shift; ( cd "$FIX/$sub" && "$BIN" "$FIX/$sub" --no-cache "$@" 2>&1 ); }
# has SUB 'LABEL' NEEDLE ARGS...: the answer exits 0 and contains NEEDLE (a fixed string)
has(){
    local sub="$1" label="$2" needle="$3"; shift 3
    local out rc; out="$( run "$sub" "$@" )"; rc=$?
    if [ "$rc" -eq 0 ] && printf '%s' "$out" | grep -qF -- "$needle"; then ok "$label"; else no "$label (rc=$rc, want $needle)"; printf '%s\n' "$out" | tail -c 400; echo; fi
}
# lacks SUB 'LABEL' NEEDLE ARGS...: the answer does not contain NEEDLE
lacks(){
    local sub="$1" label="$2" needle="$3"; shift 3
    local out; out="$( run "$sub" "$@" )"
    if printf '%s' "$out" | grep -qF -- "$needle"; then no "$label (found $needle)"; printf '%s\n' "$out" | tail -c 400; echo; else ok "$label"; fi
}
# bind SUB SPEC WANT [NOT...]: --callees=SPEC exits 0, names helper WANT and none of NOT
bind(){
    local sub="$1" spec="$2" want="$3"; shift 3
    local out rc bad=""; out="$( run "$sub" --callees="$spec" )"; rc=$?
    for n in "$@"; do printf '%s' "$out" | grep -q "n=\"$n\"" && bad="$bad $n"; done
    if [ "$rc" -eq 0 ] && printf '%s' "$out" | grep -q "n=\"$want\"" && [ -z "$bad" ]; then ok "bind   $sub $spec -> $want"
    else no "bind   $sub $spec (rc=$rc, want $want, wrong:$bad)"; printf '%s\n' "$out" | tail -c 400; echo; fi
}
# refuse SUB SPEC: --callees=SPEC exits 1 with found="0"
refuse(){
    local sub="$1" spec="$2" out rc; out="$( run "$sub" --callees="$spec" )"; rc=$?
    if [ "$rc" -eq 1 ] && printf '%s' "$out" | grep -q 'found="0"'; then ok "refuse $sub $spec"
    else no "refuse $sub $spec bound something (rc=$rc)"; printf '%s\n' "$out" | tail -c 400; echo; fi
}

echo "-- (1) a JS method row names the class whose body holds it"
has   js 'RED  --callers rows: Template.render'               'n="Template.render" p="src/environment.js:8"'    --callers=jdnTplHelper
has   js 'RED  --callers rows: Environment.render'            'n="Environment.render" p="src/environment.js:2"' --callers=jdnEnvHelper
has   js 'RED  --path hop rows print the owner'                '<s t="method" n="Template.render"'               --path=Template.render,jdnTplHelper
has   js 'RED  the TS/JS owner clause rides with such a row'   'On a TS/JS row Owner is the class'               --legend=full --callers=jdnTplHelper
lacks js 'KEEP no owner row, no TS/JS owner clause'            'On a TS/JS row Owner'                            --legend=full --callers=jdnNamedHelper
lacks js 'KEEP a free function row stays bare'                 'n=".onlyNamed"'                                  --callers=jdnNamedHelper
lacks js 'KEEP a member of a literal a method returns stays bare' 'n="Nest.inner"'                               --callers=jdnNestHelper
has   js 'KEEP ... and still prints, bare'                     'n="inner"'                                       --callers=jdnNestHelper
bind  js 'Template.render' jdnTplHelper jdnEnvHelper           # KEEP: the printed name resolves back (written-spelling tier)

echo "-- (1) a printed p= (file:LINE) selects that definition, after every name tier"
bind   js 'src/environment.js:8' jdnTplHelper jdnEnvHelper     # RED: Template.render's own line
bind   js 'src/environment.js:2' jdnEnvHelper jdnTplHelper     # RED: Environment.render's
refuse js 'src/environment.js:3'                               # KEEP: a body line names no definition
refuse js 'src/environment.js:99'                              # KEEP: past the end
has    js 'KEEP a numeric name beats the line (cfg.json:3 = both keys "3")' 'defs="2"' --callers=lib/cfg.json:3
has    js 'RED  a line no name spells binds by line (cfg.json:4 = key x)'   'defs="1"' --callers=lib/cfg.json:4

echo "-- (2) an object literal named like a class makes the spelling unknown"
refuse ts 'Widget.run'                                         # RED: base bound the class's run beside the literal's
bind   ts 'src/widget-class.ts:Widget.run' jdnClassHelper jdnLitHelper   # KEEP: the file narrows to the class
bind   ts 'Kit.run' jdnKitHelper jdnLitHelper                  # KEEP: a literal with another name (kitMock) is not Kit

echo "-- (3) a bare module.exports function is named by its file's stem"
bind   js 'is-stream' jdnStreamHelper                          # RED: base had no symbol (the calls sat in <file-scope>)
bind   js 'onlyNamed' jdnNamedHelper                           # KEEP: a named function expression keeps its own name
refuse js 'only'                                               # KEEP: ... and is not also the stem
bind   js 'propFn' jdnPropHelper                               # KEEP: module.exports.NAME = fn is NAME
refuse js 'prop'
refuse js 'decoy'                                              # KEEP: moduleLike.exports / a local `exports2` export nothing

[ "$fail" -eq 0 ] && echo "jsdefnamingcheck: PASS" || echo "jsdefnamingcheck: FAIL"
exit "$fail"
