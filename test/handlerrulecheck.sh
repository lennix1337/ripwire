#!/usr/bin/env bash
# handlerrulecheck.sh — ON-NAME-HANDLERS-V2 (src/handlerrule.h HandlerRules, read by backhops.h NameKeyHops and pathgaps.h
# pathHandlerRule): a posted message CLASS names its handler methods, by the rule the repo's own Message base writes.
#
# THE DEFECT. A message framework (textual: `self.post_message( Button.Pressed( self ) )` → the pump → `cls.__dict__.get( method_name )`
# with method_name = message.handler_name, set by `Message.__init_subclass__` to f"on_{name}") has no edge from the post site to
# on_button_pressed, and the lookup itself names every on_* def. --path=post_message,on_* ended at on_event, --impact of a
# handler counted nothing, and near_sink called a dispatched handler why="external".
#
# THE CONTRACT. The rule is READ from the source (a `cls.A = f"P{v}"` line in a Python __init_subclass__ that calls a `[a-z][A-Z]`
# + `.lower()` converter on the qualname's last two parts or the class name; a `{namespace}_` branch reads a namespace= class
# keyword). A call to a method of a class that reads `.A` (outside tests) whose FIRST argument constructs a subclass is a post
# site: a name-key hop (hedged, cands= the methods of that name) to the handlers. --path from a poster whose sites name a to=
# handler prints <handler_rule> (rule p=, the readers and the method each passes the name to, the posted classes widest first,
# the handler defs listed only when at most 16, else next=). reachable= / hops= / reaches= never move.
#
# ARMS
#   (A1) --path=post_message,on_* : <handler_rule> with p= at the rule line, base=Message attr=handler_name, the reader
#        Pump._on_message passes= Pump._get_dispatch_methods, <hm n="Button.Pressed" h="on_button_pressed" cands="2">, and the
#        two handler defs as <h> rows (handlers=2 <= 16, no next=); reachable="0".
#   (A2) --impact=widgets.py:on_button_pressed: reaches_hedged=1 reaches_hedged_via=name-key (Button.press), reaches="0".
#   (A3) a nested class's name is owner-qualified, a namespace= keyword replaces the owner: on_worker_changed is reached hedged.
#   (A4) a top-level class posted by name (Tick()) hops to on_tick: the bridge --path=tick,leaf crosses via="name-key" cands="1".
#   (N1) a variable argument (msg = Tick(); post_message( msg )) is no post site: --path=poke,leaf has no name-key crossing.
#   (N2) a repo whose __init_subclass__ builds the name another way (no converter) has no rule: no <handler_rule>, no name-key.
#   (N3) a call that is not a poster's (log( Tick() )) is no post site.
#   (W1) 17 handler defs: handlers="17", no <h> rows, next= lists one name's defs.
#   (L1) the <handler_rule> clause rides only with the block (full + compact); every answer parses as XML.
#   (M1) MCP path_between carries the same <handler_rule>.
#
# Usage: test/handlerrulecheck.sh [BIN]   (BIN defaults to RIPWIRE_BIN, then build/ripwire)
set -u
ROOT="$( cd "$( dirname "$0" )/.." && pwd )"
BIN="${1:-${RIPWIRE_BIN:-$ROOT/build/ripwire}}"
[ "${BIN#/}" = "$BIN" ] && BIN="$ROOT/$BIN"
echo "handlerrulecheck: BIN=$BIN"
[ -x "$BIN" ] || { echo "  FAIL  no ripwire binary at $BIN — build first"; exit 2; }
command -v python3 >/dev/null 2>&1 || { echo "  FAIL  python3 required"; exit 2; }
TMP="$( mktemp -d )"; trap 'rm -rf "$TMP"' EXIT

mkdir -p "$TMP/fw" "$TMP/norule" "$TMP/wide"
cat > "$TMP/fw/case.py" <<'EOF'
import re


def camel_to_snake(name, _re=re.compile("[a-z][A-Z]")):
    def repl(m):
        lower, upper = m.group()
        return f"{lower}_{upper.lower()}"

    return _re.sub(repl, name).lower()
EOF
cat > "$TMP/fw/message.py" <<'EOF'
from case import camel_to_snake


class Message:
    namespace = ""

    def __init_subclass__(cls, namespace=None):
        super().__init_subclass__()
        if namespace is not None:
            name = f"{namespace}_{camel_to_snake(cls.__name__)}"
        else:
            parts = cls.__qualname__.rsplit(".", 2)[-2:]
            name = "_".join(camel_to_snake(p) for p in parts)
        cls.handler_name = f"on_{name}"
EOF
cat > "$TMP/fw/pump.py" <<'EOF'
class Pump:
    def post_message(self, message):
        self._queue.append(message)
        return True

    def _on_message(self, message):
        handler_name = message.handler_name
        for method in self._get_dispatch_methods(handler_name, message):
            method(message)

    def _get_dispatch_methods(self, method_name, message):
        for cls in self.__class__.__mro__:
            method = cls.__dict__.get(method_name)
            if method is not None:
                yield method.__get__(self, cls)
EOF
cat > "$TMP/fw/widgets.py" <<'EOF'
from message import Message
from pump import Pump


def leaf(x):
    return x


class Tick(Message):
    pass


class Button(Pump):
    class Pressed(Message):
        pass

    def press(self):
        self.post_message(self.Pressed())


class Worker(Pump):
    class Changed(Message, namespace="worker"):
        pass

    def change(self):
        self.post_message(Worker.Changed())


class Form(Pump):
    def on_button_pressed(self, m):
        return leaf(m)

    def on_worker_changed(self, m):
        return m

    def on_tick(self, m):
        return leaf(m)


def log(m):
    return m


class Clock(Pump):
    def tick(self):
        self.post_message(Tick())

    def poke(self):
        msg = Tick()
        self.post_message(msg)

    def note(self):
        log(Tick())
EOF
cat > "$TMP/fw/other.py" <<'EOF'
class Panel:
    def on_button_pressed(self, m):
        return m
EOF
cp "$TMP/fw/case.py" "$TMP/fw/pump.py" "$TMP/fw/widgets.py" "$TMP/norule/"
cat > "$TMP/norule/message.py" <<'EOF'
class Message:
    def __init_subclass__(cls):
        super().__init_subclass__()
        name = cls.__name__.lower()
        cls.handler_name = f"on_{name}"
EOF
cp "$TMP/fw/case.py" "$TMP/fw/message.py" "$TMP/fw/pump.py" "$TMP/wide/"
{
    printf 'from message import Message\nfrom pump import Pump\n\n\nclass Wide(Message):\n    pass\n\n\nclass Src(Pump):\n    def send(self):\n        self.post_message(Wide())\n'
    for i in $( seq 1 17 ); do printf '\n\nclass H%s:\n    def on_wide(self, m):\n        return m\n' "$i"; done
} > "$TMP/wide/w.py"

python3 - "$BIN" "$TMP" <<'PY'
import json, re, subprocess, sys
binp, tmp = sys.argv[1], sys.argv[2]
fail = 0
def ok( m ): print( "  PASS  " + m )
def no( m ):
    global fail
    fail = 1
    print( "  FAIL  " + m )
def check( c, m ): ( ok if c else no )( m )
def run( d, *args ):
    p = subprocess.run( [ binp, d, "--no-cache", *args ], cwd = d, capture_output = True, text = True, timeout = 120 )
    return p.stdout if p.returncode == 0 else None
def xml_ok( t ):
    if t is None:
        return False
    q = subprocess.run( [ "xmllint", "--noout", "-" ], input = t, capture_output = True, text = True )
    return q.returncode == 0
def root( t, tag ):
    m = re.search( "<" + tag + r"\b[^>]*>", t or "" )
    return m.group( 0 ) if m else ""
def attr( el, a ):
    m = re.search( r'\s' + a + r'="([^"]*)"', el or "" )
    return m.group( 1 ) if m else None
fw, nr, wd = f"{tmp}/fw", f"{tmp}/norule", f"{tmp}/wide"

t = run( fw, "--path=post_message,on_*" )
hr = root( t, "handler_rule" )
check( xml_ok( t ), "A1 answer parses as XML" )
check( attr( root( t, "path" ), "reachable" ) == "0", "A1 reachable stays 0" )
check( attr( hr, "p" ) == "message.py:14" and attr( hr, "base" ) == "Message" and attr( hr, "attr" ) == "handler_name", f"A1 the rule is read from message.py:14 ({hr[:200]})" )
check( '<hrd n="Pump._on_message" p="pump.py:6" passes="Pump._get_dispatch_methods" passes_p="pump.py:11"/>' in ( t or "" ), "A1 the reader passes the name to the MRO lookup" )
check( re.search( r'<hm n="Button.Pressed" h="on_button_pressed" in="Button.press" site="widgets.py:18" posts="1" cands="2"/>', t or "" ) is not None, "A1 Button.Pressed -> on_button_pressed, cands=2" )
check( '<h n="Form.on_button_pressed"' in ( t or "" ) and '<h n="Panel.on_button_pressed"' in ( t or "" ) and attr( hr, "next" ) is None, "A1 both handler defs listed, no next=" )
check( 'name="on_{snake(Owner)_}{snake(Class)} | on_{namespace}_{snake(Class)}"' in hr, "A1 name= spells the rule as the source states it" )

t = run( fw, "--impact=widgets.py:on_button_pressed" )
r = root( t, "impact" )
check( attr( r, "reaches" ) == "0" and attr( r, "reaches_hedged" ) == "1" and attr( r, "reaches_hedged_via" ) == "name-key", f"A2 impact hedged via name-key, reaches=0 ({r[:220]})" )
t = run( fw, "--impact=on_worker_changed" )
check( attr( root( t, "impact" ), "reaches_hedged" ) == "1", "A3 namespace= keyword: on_worker_changed reached hedged" )
t = run( fw, "--path=tick,leaf" )
br = re.search( r"<bridge.*?</bridge>", t or "" )
check( br is not None and '<s t="fn" n="on_tick" p="widgets.py:36" via="name-key" cands="1"/>' in br.group( 0 ), f"A4 bridge tick -> on_tick via=name-key cands=1 ({br.group( 0 )[:240] if br else t and t[-300:]})" )
check( attr( root( t, "path" ), "reachable" ) == "0" and attr( root( t, "path" ), "hops" ) == "0", "A4 reachable/hops stay 0" )
t = run( fw, "--path=poke,leaf" )
check( t is not None and 'in="poke"' not in t and "<bridge" not in t, "N1 a variable argument is no post site (no bridge, no ns in=poke)" )
t = run( fw, "--path=note,leaf" )
check( t is not None and 'in="note"' not in t and "<bridge" not in t, "N3 a non-poster call log(Tick()) is no post site" )
t = run( nr, "--path=post_message,on_*" )
check( xml_ok( t ) and "<handler_rule" not in t and "handler_rule via=name-key" not in t, "N2 no converter: no <handler_rule>, no clause" )
t = run( nr, "--impact=widgets.py:on_button_pressed" )
check( t is not None and attr( root( t, "impact" ), "reaches_hedged_via" ) is None, "N2 no rule: no name-key hop" )
t = run( wd, "--path=post_message,on_*" )
hr = root( t, "handler_rule" )
check( attr( hr, "handlers" ) == "17" and "<h n=\"" not in ( t or "" ) and attr( hr, "next" ) == "--regex=&apos;def on_wide\\(&apos;", f"W1 17 defs: count + next=, no cut list ({hr[:260]})" )

t = run( fw, "--path=post_message,on_*" )
check( "handler_rule via=name-key base=" in t, "L1 compact clause rides with the block" )
t = run( fw, "--path=post_message,on_*", "--legend=full" )
check( xml_ok( t ) and "<handler_rule via=name-key base= attr=" in t, "L1 full clause rides with the block" )
t = run( fw, "--path=Button.press,Form.on_tick" )
check( t is not None and "handler_rule" not in t, "L1 no block, no clause" )

req = '{"jsonrpc":"2.0","id":1,"method":"initialize","params":{}}\n' + json.dumps( { "jsonrpc": "2.0", "id": 2, "method": "tools/call", "params": { "name": "path_between", "arguments": { "from": "post_message", "to": "on_*" } } } ) + "\n"
p = subprocess.run( [ binp, "--mcp", fw, "--no-cache" ], cwd = fw, input = req, capture_output = True, text = True, timeout = 120 )
check( '<handler_rule via=\\"name-key\\" base=\\"Message\\"' in p.stdout and 'h=\\"on_button_pressed\\"' in p.stdout, "M1 MCP path_between carries <handler_rule>" )
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
