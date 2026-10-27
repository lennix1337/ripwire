#!/usr/bin/env bash
# namekeyhopcheck.sh — STRING-KEYED (src/backhops.h NameKeyHops, src/pathgaps.h BridgeVia::NameKey; issue #375 slice 7a): a
# function that looks a method up by a name built from a template joins the hedged walks to the definitions that name fits.
#
# THE DEFECT. A command shell (cpython cmd.Cmd.onecmd: `func = getattr( self, 'do_' + cmd )`) or an event table
# (`this[ 'on' + e ]()`) has no call edge from the dispatcher to the handlers. IX-3 already lists the template lookup as a
# <pd> row on --callers / --impact of a handler, but the joining answers ignored it: --path=main,list_dir (main -> onecmd ->
# do_ls -> list_dir) printed no bridge, and --impact=do_ls counted nothing behind the dispatcher.
#
# THE CONTRACT. A template lookup site in f (PatternIndex: getattr / hasattr / X.__dict__.get with an f-string or `+` holding a
# literal side of >= 3 identifier characters; JS/TS o[ 'p' + x ] / o[ `p${x}` ]) hops to every definition its template can
# name (prefix and suffix around a non-empty hole; a self/this/cls receiver keeps the site's class family); width = that count.
# Every hop is HEDGED: reaches= / reachable= / hops= / proven= never move. The --path bridge crosses it (`<s … via="name-key"
# cands=K/>`) within 16 candidates; the --impact ring follows it (reaches_hedged=, `<rh … via="name-key"/>` at d=2).
# A plain variable name, a literal name, a too-wide template and a sibling class's same-prefix method are no hop.
#
# ARMS
#   (K1) bridge: --path=main,list_dir crosses onecmd -> do_ls via="name-key" cands="2"; reachable="0" hops="0".
#   (K2) --impact=do_ls: reaches_hedged= counts onecmd and its proven caller main; reaches= unchanged (0).
#   (K3) d=2 ring row: --impact=do_go (do_go <- run_cmd <- route, both name-key hops; route's receiver is not self, so
#        unnarrowed) lists `<rh called_by="route" via="name-key" calls="run_cmd"/>`.
#   (N1) a sibling class with the same prefix (Other.do_rm) is not in cands (self narrows to the class family): cands="2".
#   (N2) a plain variable name (getattr( self, name )) and a literal name (getattr( self, "do_ls" )) record no hop.
#   (N3) a too-wide template (f"_{m}") records no hop.
#   (W1) width: 17 fitting handlers are counted by --impact but the bridge does not cross them (fan-out 16).
#   (T1) a handler that itself dispatches by the same template is not its own way in (self-exclusion; mutant: drop `from != sink`).
#   (J1) JS/TS: this[ 'handle' + e ]() crosses to handleClick via="name-key" ('on' alone is under the 3-character literal floor).
#   (L1) the name-key clause rides only when a name-key row/crossing is present; every answer parses as XML.
#   (M1) MCP path_between twin crosses the same hop.
#
# Usage: test/namekeyhopcheck.sh [BIN]   (BIN defaults to RIPWIRE_BIN, then build/ripwire)
set -u
ROOT="$( cd "$( dirname "$0" )/.." && pwd )"
BIN="${1:-${RIPWIRE_BIN:-$ROOT/build/ripwire}}"
[ "${BIN#/}" = "$BIN" ] && BIN="$ROOT/$BIN"
echo "namekeyhopcheck: BIN=$BIN"
[ -x "$BIN" ] || { echo "  FAIL  no ripwire binary at $BIN — build first"; exit 2; }
command -v python3 >/dev/null 2>&1 || { echo "  FAIL  python3 required"; exit 2; }
TMP="$( mktemp -d )"; trap 'rm -rf "$TMP"' EXIT

mkdir -p "$TMP/sh" "$TMP/neg" "$TMP/w17" "$TMP/self" "$TMP/js" "$TMP/d2"
cat > "$TMP/sh/shell.py" <<'EOF'
def list_dir(a):
    return a


class Shell:
    def onecmd(self, line):
        cmd = line.split()[0]
        func = getattr(self, 'do_' + cmd)
        return func(line)

    def do_ls(self, a):
        return list_dir(a)

    def do_cd(self, a):
        return a


class Other:
    def do_rm(self, a):
        return list_dir(a)


def main():
    Shell().onecmd("ls x")
EOF
cat > "$TMP/neg/shell.py" <<'EOF'
def list_dir(a):
    return a


class Shell:
    def by_var(self, name):
        return getattr(self, name)()

    def by_literal(self):
        return getattr(self, "do_ls")()

    def too_wide(self, m):
        return getattr(self, f"_{m}")()

    def do_ls(self, a):
        return list_dir(a)


def main():
    s = Shell()
    s.by_var("do_ls")
    s.by_literal()
    s.too_wide("x")
EOF
{
    printf 'def leaf(a):\n    return a\n\n\nclass Wide:\n    def run(self, c):\n        return getattr(self, "do_" + c)(1)\n'
    for i in $( seq 1 17 ); do printf '\n    def do_h%s(self, a):\n        return leaf(a)\n' "$i"; done
    printf '\n\ndef main():\n    Wide().run("h1")\n'
} > "$TMP/w17/wide.py"
cat > "$TMP/self/m.py" <<'EOF'
def sink(a):
    return a


class Rec:
    def do_again(self, c):
        return getattr(self, 'do_' + c)(c)

    def do_end(self, c):
        return sink(c)
EOF
cat > "$TMP/js/ev.js" <<'EOF'
function paint(e) {
  return e;
}

class Button {
  fire(e) {
    return this['handle' + e](e);
  }
  handleClick(e) {
    return paint(e);
  }
}

function main() {
  new Button().fire('Click');
}
module.exports = { main };
EOF
cat > "$TMP/d2/m.py" <<'EOF'
class Shell:
    def run_cmd(self, c):
        return getattr(self, 'do_' + c)(c)

    def do_go(self, c):
        return c


class Front:
    def route(self, k, c):
        return getattr(Shell(), 'run_' + k)(c)


def top():
    return Front().route("cmd", "go")
EOF

python3 - "$BIN" "$TMP" <<'PY'
import json, re, shutil, subprocess, sys
binp, tmp = sys.argv[1], sys.argv[2]
fail = 0
def ok( m ): print( "  PASS  " + m )
def no( m ):
    global fail
    fail = 1
    print( "  FAIL  " + m )
def check( c, m ): ( ok if c else no )( m )
def run( d, *args ):
    """One answer, read only off a run that succeeded and printed a root (checklist 14)."""
    p = subprocess.run( [ binp, d, "--no-cache", *args ], cwd = d, capture_output = True, text = True, timeout = 120 )
    if p.returncode != 0 or not re.search( r'<(path|impact) ', p.stdout ):
        return None
    return p.stdout
def body( t ):
    return re.sub( r'<!--.*?-->', '', t or "", flags = re.S )
def root( t, tag ):
    m = re.search( r'<' + tag + r' [^>]*>', body( t ) )
    return m.group( 0 ) if m else ""
def attr( el, a ):
    m = re.search( r'\b' + a + r'="([^"]*)"', el )
    return m.group( 1 ) if m else None
def bridge( t ):
    m = re.search( r'<bridge .*?</bridge>', body( t ) )
    return m.group( 0 ) if m else ""
def xmlok( t ):
    if shutil.which( "xmllint" ) is None:
        return True
    return subprocess.run( [ "xmllint", "--noout", "-" ], input = t, text = True, capture_output = True ).returncode == 0

sh = f"{tmp}/sh"
t = run( sh, "--path=main,list_dir", "--legend=full" )
if t is None:
    no( "K1 --path=main,list_dir answered" )
else:
    r, b = root( t, "path" ), bridge( t )
    check( attr( r, "reachable" ) == "0" and attr( r, "hops" ) == "0", "K1 reachable=0 hops=0 (a name-key hop is no proven edge)" )
    check( re.search( r'<s [^>]*n="do_ls"[^>]*via="name-key" cands="2"', b ) is not None, "K1 bridge crosses onecmd -> do_ls via=name-key cands=2 (N1: Other.do_rm not a candidate)" )
    check( re.search( r'n="onecmd".*n="do_ls".*n="list_dir"', b ) is not None, "K1 bridge order main..onecmd -> do_ls -> list_dir" )
    check( "via=name-key: a NAME-KEYED lookup" in t, "L1 the name-key clause rides with a name-key crossing" )
    check( xmlok( t ), "L1 the --path answer parses as XML" )
t = run( sh, "--impact=do_ls" )
if t is None:
    no( "K2 --impact=do_ls answered" )
else:
    r = root( t, "impact" )
    check( attr( r, "reaches_hedged" ) == "2", f"K2 reaches_hedged=2 (onecmd + main) got {attr( r, 'reaches_hedged' )}" )
    check( attr( r, "reaches" ) in ( None, "0" ), f"K2 reaches= unchanged ({attr( r, 'reaches' )})" )
    check( xmlok( t ), "L1 the --impact answer parses as XML" )
t = run( f"{tmp}/d2", "--impact=do_go", "--legend=full" )
if t is None:
    no( "K3 --impact=do_go answered" )
else:
    check( re.search( r'<rh [^>]*called_by="route"[^>]*via="name-key"[^>]*calls="run_cmd"', body( t ) ) is not None, "K3 d=2 ring row route -> run_cmd via=name-key" )
    check( attr( root( t, "impact" ), "reaches_hedged" ) == "3", f"K3 reaches_hedged=3 (run_cmd, route, top) got {attr( root( t, 'impact' ), 'reaches_hedged' )}" )
    check( "rh via=name-key: a NAME-KEYED lookup" in t and xmlok( t ), "L1 ring clause rides with a name-key <rh> row; XML parses" )
t = run( f"{tmp}/neg", "--path=main,list_dir", "--legend=full" )
if t is None:
    no( "N2 --path answered" )
else:
    check( 'via="name-key"' not in t and "NAME-KEYED" not in t, "N2/N3 a variable name, a literal name and a too-wide template are no name-key hop (and no clause)" )
t = run( f"{tmp}/neg", "--impact=do_ls" )
check( t is not None and attr( root( t, "impact" ), "reaches_hedged" ) is None, "N2/N3 --impact=do_ls has no reaches_hedged" )
t = run( f"{tmp}/w17", "--path=main,leaf" )
check( t is not None and 'via="name-key"' not in t, "W1 17 candidates: the bridge does not cross (fan-out 16)" )
t = run( f"{tmp}/w17", "--impact=do_h1" )
check( t is not None and attr( root( t, "impact" ), "reaches_hedged" ) == "1", f"W1 --impact counts the wide hop itself (run), composes nothing: {attr( root( t or '', 'impact' ), 'reaches_hedged' )}" )
t = run( f"{tmp}/self", "--impact=do_again" )
check( t is not None and attr( root( t, "impact" ), "reaches_hedged" ) is None, "T1 a dispatcher is not its own way in" )
t = run( f"{tmp}/self", "--impact=sink" )
check( t is not None, "T1 the self-dispatching shape terminates" )
t = run( f"{tmp}/js", "--path=main,paint" )
if t is None:
    no( "J1 --path=main,paint answered" )
else:
    check( re.search( r'<s [^>]*n="handleClick"[^>]*via="name-key"', bridge( t ) ) is not None, "J1 JS this['handle' + e] crosses fire -> handleClick via=name-key" )
t = run( sh, "--path=main,list_dir" )
check( t is not None and "s via=name-key cands=K:" in t, "L1 the default (compact) legend reads s via=name-key (present-only)" )
t = run( f"{tmp}/neg", "--path=main,list_dir" )
check( t is not None and "via=name-key" not in t, "L1 the compact name-key reading is absent with no name-key row" )
# M1: the MCP path_between twin crosses the same hop
req = '{"jsonrpc":"2.0","id":1,"method":"initialize","params":{}}\n{"jsonrpc":"2.0","id":2,"method":"tools/call","params":{"name":"path_between","arguments":{"from":"main","to":"list_dir"}}}\n'
p = subprocess.run( [ binp, "--mcp", sh, "--no-cache" ], cwd = sh, input = req, capture_output = True, text = True, timeout = 120 )
check( 'via=\\"name-key\\"' in p.stdout, "M1 MCP path_between crosses via=name-key" )
sys.exit( fail )
PY
rc=$?   # the harness's own status, read immediately (checklist 14): 0 every arm passed, 1 an arm failed, anything else a crash
fail=0
if [ "$rc" -ne 0 ]; then
    printf '  FAIL  the arm harness exited %s (an arm failed above, or the harness crashed)\n' "$rc"
    fail=1
fi
[ "$fail" = 0 ] && echo "ALL PASS" || echo "FAILURES ABOVE"
exit "$fail"
