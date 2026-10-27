#!/usr/bin/env bash
# testonlydeadcheck.sh — D1 TESTONLY-DEAD: --dead-code lists library functions whose EVERY indexed caller is test code
# as <to n= t= p= l=> rows (test-only=N on the root, absent at 0). Never in count=, never a <d> row.
# Arms: A1 Python private-module def called only by a test is listed; A2-A6 near misses stay off (a library caller,
# an ALIASED library import, no internal evidence, a method, a decorated def); A7 a C static called only by a test is
# listed; A8 a zero-caller Python private def is not a count= row; A9 a tree with none is byte-identical in shape
# (no test-only=, no clause); A10 rows only on the first page; A11 well-formed XML.
#   RIPWIRE_BIN=build/ripwire bash test/testonlydeadcheck.sh     # red on a pre-D1 binary (A1, A7)
set -u
ROOT="$( cd "$( dirname "$0" )/.." && pwd )"
BIN="${1:-${RIPWIRE_BIN:-$ROOT/build/ripwire}}"
[ "${BIN#/}" = "$BIN" ] && BIN="$ROOT/$BIN"
[ -x "$BIN" ] || { echo "no ripwire binary at $BIN — build first"; exit 2; }
TMP="$( mktemp -d )"; trap 'rm -rf "$TMP"' EXIT
fail=0
ok(){ printf '  PASS  %s\n' "$*" || { fail=1; printf '  FAIL  could not write the PASS line for: %s\n' "$*"; }; return 0; }
no(){ printf '  FAIL  %s\n' "$*"; fail=1; }
echo "testonlydeadcheck: BIN=$BIN"

F="$TMP/fx"; mkdir -p "$F/pkg" "$F/tests" "$F/src"
cat > "$F/pkg/_tools.py" <<'P'
def only_tested(x):
    return x + 1

def lib_used(x):
    return x * 2

def alias_used(x):
    return x - 1

def zero_callers(x):
    return x

@cache_it
def decorated_tested(x):
    return x

class Widget:
    def method_tested(self):
        return 1
P
cat > "$F/pkg/public.py" <<'P'
def public_tested(x):
    return x
P
cat > "$F/pkg/user.py" <<'P'
from pkg._tools import alias_used as renamed
from pkg._tools import lib_used

def run(v):
    return lib_used(v) + renamed(v)
P
cat > "$F/tests/test_tools.py" <<'P'
from pkg._tools import only_tested, lib_used, alias_used, decorated_tested, Widget
from pkg.public import public_tested

def test_all():
    assert only_tested(1) == 2
    assert lib_used(1) == 2
    assert alias_used(1) == 0
    assert decorated_tested(1) == 1
    assert Widget().method_tested() == 1
    assert public_tested(1) == 1
P
cat > "$F/src/core.c" <<'C'
static int c_only_tested(int x)
{
    return x + 1;
}
int core_entry(int x)
{
    return x;
}
C
cat > "$F/tests/test_core.c" <<'C'
int run_core_test(void)
{
    return c_only_tested(1);
}
C
run(){ ( cd "$F" || exit 1; "$BIN" "$F" --no-cache "$@" 2>/dev/null ); }
OUT="$( run --dead-code )"
has(){ printf '%s' "$OUT" | grep -q "<to n=\"$1\""; }
if has only_tested; then ok "A1 private-module def called only by a test is a <to> row"; else no "A1 only_tested not listed"; fi
if printf '%s' "$OUT" | grep -q 'test-only="2"'; then ok "A1 root test-only=2"; else no "A1 test-only= is not 2: $( printf '%s' "$OUT" | grep -o 'test-only="[0-9]*"' )"; fi
if has lib_used; then no "A2 lib_used (a library caller) listed"; else ok "A2 a library caller keeps it off"; fi
if has alias_used; then no "A3 alias_used (an aliased library import) listed"; else ok "A3 an aliased library import keeps it off"; fi
if has public_tested; then no "A4 public_tested (no internal evidence) listed"; else ok "A4 a public module/name is not listed"; fi
if has method_tested; then no "A5 a method listed"; else ok "A5 methods are not listed"; fi
if has decorated_tested; then no "A6 a decorated def listed"; else ok "A6 a decorated def is not listed"; fi
if has c_only_tested; then ok "A7 a C static called only by a test is a <to> row"; else no "A7 c_only_tested not listed"; fi
if printf '%s' "$OUT" | grep -q '<d n="zero_callers"'; then no "A8 a zero-caller Python private def became a count= row"; else ok "A8 zero-caller Python private def is not a <d> row"; fi
if printf '%s' "$OUT" | grep -q '<dead-code [^>]*count="0"'; then ok "A8 count=0 (test-only rows are not in count=)"; else no "A8 count= is not 0"; fi
NONE="$( run --dead-code=pkg/public.py )"
if printf '%s' "$NONE" | grep -q 'test-only'; then no "A9 test-only text on an answer with no <to> row"; else ok "A9 no test-only=/clause when none (present-only)"; fi
if printf '%s' "$OUT" | grep -q 'test-only=N'; then ok "A9 the clause rides the answer that carries test-only="; else no "A9 clause missing"; fi
P2="$( run --dead-code --limit=1 --offset=1 )"
if printf '%s' "$P2" | grep -q '<to n="'; then no "A10 <to> rows repeated past the first page"; else ok "A10 <to> rows ride the first page only"; fi
if command -v xmllint >/dev/null 2>&1; then
    if printf '%s' "$OUT" | xmllint --noout - 2>/dev/null; then ok "A11 well-formed"; else no "A11 not well-formed"; fi
    if run --dead-code --legend=full | xmllint --noout - 2>/dev/null; then ok "A11 full legend well-formed"; else no "A11 full legend not well-formed"; fi
else
    echo "  SKIP  A11 xmllint absent"
fi
[ "$fail" = 0 ] && echo "testonlydeadcheck: PASS" || echo "testonlydeadcheck: FAIL"
exit "$fail"
