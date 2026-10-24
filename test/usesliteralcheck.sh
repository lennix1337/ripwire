#!/usr/bin/env bash
# usesliteralcheck.sh — regression gate for JS/TS literal receiver built-in member call filtering in --uses.
#
#   test/usesliteralcheck.sh                        # uses build/ripwire on test/usesliteralfix
#   RIPWIRE_BIN=asan/ripwire test/usesliteralcheck.sh
#
# Fixture test/usesliteralfix exercises:
#   - Direct imported calls: replace, map, test
#   - Literal-receiver calls to built-in members: "hello".replace, `template`.replace, [1, 2, 3].map, /re/.test
#   - Casted non-literal receiver call: (x as string).replace
#   - Prototype method polyfill: String.prototype.trimEnd and literal call "abc".trimEnd()
#   - Name with NO in-repo definition: "hello".repeat(3) (must answer with exit 0 and external="1")
#
# Exits non-zero on any failure.

set -u
ROOT="$( cd "$( dirname "$0" )/.." && pwd )"
BIN="${1:-${RIPWIRE_BIN:-$ROOT/build/ripwire}}"
[ "${BIN#/}" = "$BIN" ] && BIN="$ROOT/$BIN"          # allow a repo-relative RIPWIRE_BIN
CORPUS="$ROOT/test/usesliteralfix"
TMP="$( mktemp -d )"; trap 'rm -rf "$TMP"' EXIT
fail=0
ok(){ printf '  PASS  %s\n' "$*" || { fail=1; printf '  FAIL  could not write the PASS line for: %s\n' "$*"; }; return 0; }
no(){ printf '  FAIL  %s\n' "$*"; fail=1; }

[ -x "$BIN" ]   || { echo "no ripwire binary at $BIN — build first"; exit 2; }
[ -d "$CORPUS" ]|| { echo "no test/usesliteralfix dir — fixture missing"; exit 2; }

echo "usesliteralcheck: BIN=$BIN  CORPUS=$CORPUS"

# helper: extract call sites as "role basename:line"
uses_sites(){ "$BIN" "$CORPUS" --uses="$1" --no-cache 2>/dev/null | grep -o 'role="[a-z]*" p="[^"]*"' \
    | sed -E 's/role="([a-z]*)" p="([^"]*\/)?([^"/]*)"/\1 \3/'; }
has_site(){ uses_sites "$1" | grep -qxF "$2"; }

# ── 1) replace: literal calls filtered out, direct + casted receiver kept, declined_literal_calls="2" ──
REPLACE_OUT="$( "$BIN" "$CORPUS" --uses=replace --no-cache 2>/dev/null )"
if echo "$REPLACE_OUT" | grep -q 'count="2"'; then
    ok "replace count=2"
else
    no "replace count != 2: $REPLACE_OUT"
fi
if echo "$REPLACE_OUT" | grep -q 'declined_literal_calls="2"'; then
    ok "replace declined_literal_calls=2"
else
    no "replace missing declined_literal_calls=2"
fi
REPLACE_LEGEND="${REPLACE_OUT%%-->*}"
if printf '%s' "$REPLACE_LEGEND" | grep -q 'declined_literal_calls='; then
    ok "replace compact legend defines declined_literal_calls="
else
    no "replace compact legend leaves declined_literal_calls= undefined"
fi
if has_site replace "call app.ts:4"; then
    ok "replace direct call kept @app.ts:4"
else
    no "replace direct call missing @app.ts:4"
fi
if has_site replace "call app.ts:16"; then
    ok "replace casted receiver call kept @app.ts:16"
else
    no "replace casted receiver missing @app.ts:16"
fi
if ! has_site replace "call app.ts:8"; then
    ok "replace literal string call dropped @app.ts:8"
else
    no "replace literal string call leaked @app.ts:8"
fi
if ! has_site replace "call app.ts:9"; then
    ok "replace template literal call dropped @app.ts:9"
else
    no "replace template literal call leaked @app.ts:9"
fi

# ── 2) map: literal array call filtered out, direct call kept, declined_literal_calls="1" ─────────────
MAP_OUT="$( "$BIN" "$CORPUS" --uses=map --no-cache 2>/dev/null )"
if echo "$MAP_OUT" | grep -q 'count="1"'; then
    ok "map count=1"
else
    no "map count != 1: $MAP_OUT"
fi
if echo "$MAP_OUT" | grep -q 'declined_literal_calls="1"'; then
    ok "map declined_literal_calls=1"
else
    no "map missing declined_literal_calls=1"
fi
if has_site map "call app.ts:5"; then
    ok "map direct call kept @app.ts:5"
else
    no "map direct call missing @app.ts:5"
fi
if ! has_site map "call app.ts:10"; then
    ok "map literal array call dropped @app.ts:10"
else
    no "map literal array call leaked @app.ts:10"
fi

# ── 3) test: literal regex call filtered out, direct call kept, declined_literal_calls="1" ───────────
TEST_OUT="$( "$BIN" "$CORPUS" --uses=test --no-cache 2>/dev/null )"
if echo "$TEST_OUT" | grep -q 'count="1"'; then
    ok "test count=1"
else
    no "test count != 1: $TEST_OUT"
fi
if echo "$TEST_OUT" | grep -q 'declined_literal_calls="1"'; then
    ok "test declined_literal_calls=1"
else
    no "test missing declined_literal_calls=1"
fi
if has_site test "call app.ts:6"; then
    ok "test direct call kept @app.ts:6"
else
    no "test direct call missing @app.ts:6"
fi
if ! has_site test "call app.ts:11"; then
    ok "test literal regex call dropped @app.ts:11"
else
    no "test literal regex call leaked @app.ts:11"
fi

# ── 4) trimEnd: prototype polyfill keeps literal call row ─────────────────────────────────────────────
TRIM_OUT="$( "$BIN" "$CORPUS" --uses=trimEnd --no-cache 2>/dev/null )"
if echo "$TRIM_OUT" | grep -q 'defs="1"'; then
    ok "trimEnd defs=1"
else
    no "trimEnd defs != 1: $TRIM_OUT"
fi
if echo "$TRIM_OUT" | grep -q 'count="1"'; then
    ok "trimEnd count=1"
else
    no "trimEnd count != 1: $TRIM_OUT"
fi
if has_site trimEnd "call polyfill.js:5"; then
    ok "trimEnd polyfill call kept @polyfill.js:5"
else
    no "trimEnd polyfill call missing @polyfill.js:5"
fi

# ── 5) repeat: no in-repo definition answers with exit 0 and external="1" ────────────────────────────
REPEAT_CODE=0
REPEAT_OUT="$( "$BIN" "$CORPUS" --uses=repeat --no-cache 2>"$TMP/err" )" || REPEAT_CODE=$?
if [ "$REPEAT_CODE" -eq 0 ]; then
    ok "repeat exits 0 with no in-repo definition"
else
    no "repeat exited $REPEAT_CODE"
fi
if echo "$REPEAT_OUT" | grep -q 'external="1"'; then
    ok "repeat reports external=1"
else
    no "repeat missing external=1: $REPEAT_OUT"
fi
if echo "$REPEAT_OUT" | grep -q 'count="1"'; then
    ok "repeat reports count=1"
else
    no "repeat missing count=1: $REPEAT_OUT"
fi
if has_site repeat "call app.ts:19"; then
    ok "repeat call kept @app.ts:19"
else
    no "repeat call missing @app.ts:19"
fi

# ── 6) determinism ────────────────────────────────────────────────────────────────────────────────────
"$BIN" "$CORPUS" --uses=replace --no-cache >"$TMP/a" 2>/dev/null
"$BIN" "$CORPUS" --uses=replace --no-cache >"$TMP/b" 2>/dev/null
if diff -q "$TMP/a" "$TMP/b" >/dev/null; then
    ok "determinism (byte-identical run-to-run)"
else
    no "determinism failure"
fi

exit "$fail"
