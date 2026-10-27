#!/usr/bin/env bash
# forroutecheck.sh — FOR-ROUTE-DECLINED (src/forgraphverb.h, lexical.h parseGraphSubject): a --for task spelled as a graph
# question (`path from A to B`, `callers of X`, `common callers of A and B`) appends that verb's default answer inside
# <graph_verb endpoints="subject" verb=… *_span=…> just before </ctx>; the endpoints come from the task's words, so the
# block says so. A description ("serialising a dict", "spawning ripgrep") never fires, even when its plain words are
# symbol names; an endpoint no resolver finds appends nothing; a non-graph task appends nothing.
#   (p1) path form: the --path answer for the two named symbols, reachable="1", once, right before </ctx>, well-formed
#   (p2) the qualifier tail is cut (to_span= stops before "for"), and `main` is the one plain word that is an endpoint
#   (c1) callers form: the --callers answer (both callers listed); (c2) the qualifier tail "with ..." is cut
#   (v1)/(v2) the block's verb answer is byte-identical to the bare --path= / --callers= call (default caps, compact legend)
#   (m1) common-callers form: the --connect answer for both names
#   (t1) an unresolved last-first candidate falls through to the next one, and tried="2" says so
#   (n1) "path from X to serialising a dict" does NOT fire, though `dict` is a symbol here
#   (n2) "path from X to spawning ripgrep" does NOT fire, though `spawning` and `ripgrep` are symbols here
#   (n3) an endpoint no resolver finds: no block, and nothing on stdout says not found
#   (n4) a task that is not a graph question: no block
#   (j1) --json carries no block (one document per dialect)
#   (x1) MCP `for`: the same <graph_verb …> opening tag and the same path answer element
set -u
ROOT="$( cd "$( dirname "$0" )/.." && pwd )"
. "$ROOT/test/lib/clean-env.sh"
BIN="${1:-${RIPWIRE_BIN:-$ROOT/build/ripwire}}"
[ "${BIN#/}" = "$BIN" ] && BIN="$ROOT/$BIN"
echo "forroutecheck: BIN=$BIN"
fail=0
ok(){ printf '  PASS  %s\n' "$*" || { fail=1; printf '  FAIL  could not write the PASS line for: %s\n' "$*"; }; return 0; }
no(){ printf '  FAIL  %s\n' "$*"; fail=1; }
[ -x "$BIN" ] || { echo "no ripwire binary at $BIN — build first"; exit 2; }
command -v python3 >/dev/null 2>&1 || { echo "python3 required"; exit 2; }
TMP="$( mktemp -d )"; trap 'rm -rf "$TMP"' EXIT
export HOME="$TMP/home"; mkdir -p "$HOME"
FX="$TMP/fx"; mkdir -p "$FX"
cat > "$FX/flow.c" <<'C'
void leaf_sink( void ) {}
void mid_hop( void ) { leaf_sink(); }
void startEntry( void ) { mid_hop(); }
void other_caller( void ) { leaf_sink(); }
void dict( void ) {}
void spawning( void ) {}
void ripgrep( void ) {}
int main( void ) { startEntry(); other_caller(); dict(); spawning(); ripgrep(); return 0; }
C
run(){ ( cd "$FX" || exit 1; "$BIN" "$FX" --no-cache "$@" 2>/dev/null ); }
# block facts: count, verb=, spans, tried=, whether it closes right before </ctx> as the document's end, and the verb element
facts(){ python3 - "$1" <<'PY'
import re, sys
s = open( sys.argv[1], encoding="utf8", errors="replace" ).read()
n = s.count( '<graph_verb ' )
m = re.search( r'<graph_verb ([^>]*)>', s )
a = m.group( 1 ) if m else ''
g = lambda k: ( re.search( r'\b' + k + r'="([^"]*)"', a ) or [None, '-'] )[1]
tail = s.endswith( '</graph_verb></ctx>' )
leg = re.search( r'<!-- ripwire for graph_verb:(.*?)-->', s, re.S )
legdash = '--' in leg.group( 1 ) if leg else False
print( f"n={n} endpoints={g('endpoints')} verb={g('verb')} from={g('from_span')} to={g('to_span')} of={g('of_span')} and={g('and_span')} tried={g('tried')} tail={int(tail)} legdash={int(legdash)}" )
PY
}
xmlok(){ if command -v xmllint >/dev/null 2>&1; then xmllint --noout "$1" 2>/dev/null; else return 0; fi; }

run --for="path from startEntry to leaf_sink" >"$TMP/p1.xml"
f="$( facts "$TMP/p1.xml" )"
if [ "$f" = "n=1 endpoints=subject verb=--path=startEntry,leaf_sink from=startEntry to=leaf_sink of=- and=- tried=- tail=1 legdash=0" ] \
   && grep -q '<path [^>]*from="startEntry" to="leaf_sink"[^>]* reachable="1" hops="2"' "$TMP/p1.xml" && xmlok "$TMP/p1.xml"
then ok "(p1) path form: one <graph_verb endpoints=\"subject\"> with the --path answer (reachable=1 hops=2) right before </ctx>, well-formed"
else no "(p1) path form: $f"; fi

run --for="path from main to leaf_sink for a script file" >"$TMP/p2.xml"
f="$( facts "$TMP/p2.xml" )"
case "$f" in *"verb=--path=main,leaf_sink from=main to=leaf_sink of=- "*) ok "(p2) the qualifier tail is cut and \`main\` is an endpoint: $f";; *) no "(p2) $f";; esac

run --for="who are the callers of leaf_sink" >"$TMP/c1.xml"
f="$( facts "$TMP/c1.xml" )"
if [ "${f%% tried=*}" = "n=1 endpoints=subject verb=--callers=leaf_sink from=- to=- of=leaf_sink and=-" ] \
   && grep -q '<callers [^>]*of="leaf_sink"' "$TMP/c1.xml" && grep -q 'n="mid_hop"' "$TMP/c1.xml" && grep -q 'n="other_caller"' "$TMP/c1.xml"
then ok "(c1) callers form: the --callers answer for leaf_sink, both callers listed"
else no "(c1) callers form: $f"; fi

run --for="callers of leaf_sink with the local backend" >"$TMP/c2.xml"
f="$( facts "$TMP/c2.xml" )"
case "$f" in *" of=leaf_sink and=- "*) ok "(c2) the \"with ...\" tail is cut: of_span=leaf_sink";; *) no "(c2) $f";; esac

inner(){ python3 -c '
import re, sys
s = open( sys.argv[1], encoding="utf8", errors="replace" ).read()
m = re.search( r"<graph_verb [^>]*>(.*)</graph_verb></ctx>$", s, re.S )
sys.stdout.write( m.group( 1 ) if m else "NO-BLOCK" )' "$1"; }
for arm in "v1|p1|--path=startEntry,leaf_sink" "v2|c1|--callers=leaf_sink"
do
    id="${arm%%|*}"; rest="${arm#*|}"; src="${rest%%|*}"; flag="${rest#*|}"
    run "$flag" >"$TMP/$id.bare"; inner "$TMP/$src.xml" >"$TMP/$id.inner"
    if [ -s "$TMP/$id.bare" ] && cmp -s "$TMP/$id.bare" "$TMP/$id.inner"
    then ok "($id) the block's verb answer is byte-identical to the bare $flag call (same default caps, same compact legend)"
    else no "($id) the block's verb answer differs from the bare $flag call ($( wc -c <"$TMP/$id.inner" ) vs $( wc -c <"$TMP/$id.bare" ) B)"; fi
done

run --for="common callers of mid_hop and other_caller" >"$TMP/m1.xml"
f="$( facts "$TMP/m1.xml" )"
if [ "${f%% tried=*}" = "n=1 endpoints=subject verb=--connect=mid_hop,other_caller from=- to=- of=mid_hop and=other_caller" ] && grep -q '<connect ' "$TMP/m1.xml"
then ok "(m1) common-callers form: the --connect answer for both names"
else no "(m1) common-callers form: $f"; fi

run --for="path from startEntry to leaf_sink notReal_xq" >"$TMP/t1.xml"
f="$( facts "$TMP/t1.xml" )"
case "$f" in *"verb=--path=startEntry,leaf_sink "*"tried=2 "*) ok "(t1) the unresolved last-first candidate falls through, tried=2: $f";; *) no "(t1) $f";; esac

for arm in "n1|path from startEntry to serialising a dict" "n2|path from startEntry to spawning ripgrep" \
           "n3|path from startEntry to notReal_xq" "n4|how does startEntry reach leaf_sink"
do
    id="${arm%%|*}"; q="${arm#*|}"
    run --for="$q" >"$TMP/$id.xml"
    if [ -s "$TMP/$id.xml" ] && ! grep -q 'graph_verb' "$TMP/$id.xml" && ! grep -qi 'not found' "$TMP/$id.xml"
    then ok "($id) no block for \"$q\""
    else no "($id) a block (or a refusal) for \"$q\": $( facts "$TMP/$id.xml" )"; fi
done

run --for="path from startEntry to leaf_sink" --json >"$TMP/j1.json"
if [ -s "$TMP/j1.json" ] && ! grep -q 'graph_verb' "$TMP/j1.json"; then ok "(j1) --json carries no block"; else no "(j1) --json has a block or is empty"; fi

mcp_for(){ printf '{"jsonrpc":"2.0","id":1,"method":"tools/call","params":{"name":"for","arguments":{"path":"%s","task":"%s"}}}\n' "$FX" "$1" \
           | ( cd "$FX" || exit 1; "$BIN" --mcp 2>/dev/null ) | python3 -c '
import sys, json
for line in sys.stdin:
    try: d = json.loads( line )
    except Exception: continue
    c = d.get( "result", {} ).get( "content" )
    if c: print( c[0].get( "text", "" ), end="" )'; }
mcp_for "path from startEntry to leaf_sink" >"$TMP/x1.xml"
cli_tag="$( grep -o '<graph_verb [^>]*>' "$TMP/p1.xml" )"; mcp_tag="$( grep -o '<graph_verb [^>]*>' "$TMP/x1.xml" )"
if [ -n "$cli_tag" ] && [ "$cli_tag" = "$mcp_tag" ] && grep -q '<path [^>]*from="startEntry" to="leaf_sink"[^>]* reachable="1" hops="2"' "$TMP/x1.xml" \
   && [ "$( facts "$TMP/x1.xml" | sed 's/.* tail=/tail=/' )" = "tail=1 legdash=0" ]
then ok "(x1) MCP for: the same <graph_verb> tag and the path answer, before </ctx>"
else no "(x1) MCP for: cli=[$cli_tag] mcp=[$mcp_tag] $( facts "$TMP/x1.xml" )"; fi

[ "$fail" -eq 0 ] && echo "forroutecheck: PASS" || echo "forroutecheck: FAIL"
exit "$fail"
