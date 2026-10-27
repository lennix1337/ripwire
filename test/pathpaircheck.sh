#!/usr/bin/env bash
# pathpaircheck.sh — --path / path_between list EVERY reaching definition pair of a many-definition endpoint (src/pathpairs.h).
#
# THE DEFECT. --path=A,B with several definitions of A or B printed the first chain one multi-source BFS met and nothing else,
# so a reader could not tell that another definition pair also reached — fzf `--path=Loop,extendedMatch` printed Terminal.Loop's
# chain while Matcher.Loop (the one meant) reaches in 5 proven hops. No reach or hop key can pick between such pairs.
#
# THE CONTRACT. When two or more definition pairs reach (a directed chain each, then at most one hedged bridge per to= definition
# no chain reaches), the answer prints pairs_reaching=N and up to 3 <pair hops= hedged=> chains: fewest hedged hops first (an
# all-proven chain before any hedged one), then fewest hops; hedged= counts the chain's via=/prov=split/anchor=missing rows, so a
# hedged chain never reads as proven. More than 3 → alt=N + one <alt> row each + next= running the first alternative. A chain
# whose tail repeats an earlier printed chain stops where it meets it (joins=J). Exactly one reaching pair and no reaching pair
# keep today's answer (no pairs_reaching=, hop rows directly under <path>). A glob endpoint keeps its any-of-these meaning.
# MCP path_between prints the same rows.
#
# ARMS
#   (A) Go: A.Run -> midA -> leaf and B.Run -> leaf: pairs_reaching=2, two all-proven chains, the 1-hop one first.
#   (B) Python: Wrong.go -> target (proven) and Gold.go -> x.relay() (via=name) -> target: both printed, the proven one first,
#       the hedged one hedged=1 with its via="name" row; the via=name legend clause rides.
#   (C) Python: five C<i>.go -> mid<i> -> target: pairs_reaching=5, 3 chains, alt=2 + 2 <alt> rows; next= replays the first alt
#       as a one-definition answer (from_defs=1, reachable=1, from_p= the alt row's).
#   (W) twenty C<i>.go: alt=17, 12 <alt> rows, alt_capped="1" (the row cap is disclosed, the count is whole).
#   (D) exactly-one control: only A.go reaches: no pairs_reaching=, no <pair>, the hop rows sit under <path>.
#   (E) none-reach control: reachable=0, no pairs_reaching=, no <pair>.
#   (J) shared tail: B.go -> a_hop -> m1 -> m2 -> m3 -> target beside A.go -> m1 -> …: joins="1", 3 rows printed for pair 2.
#   (G) glob endpoint (g*): no pairs_reaching= (a glob already means any of these).
#   (X) every printed non-joined pair's hedged= equals its marked rows; hedged=0 pairs carry no via=/prov= row.
#   (L) the full legend carries the pairs clause exactly when pairs print; answers parse as XML (xmllint when present).
#   (T) one site, two definitions: JS `compile: function compile` is the property and the function expression on ONE line — one
#       pair, today's bytes (no pairs_reaching=, no <pair>); rv-trace-ep F1 printed two byte-identical pairs, pairs_reaching=2.
#   (V) minified same-line SIBLINGS `class A{render(){}}class B{render(){}}` (disjoint spans) are TWO sites: pairs_reaching=2, two
#       <pair>s; only NESTED same-line twins (T) fold (rv-delta-38 F1-b: the fold keyed on file+line+name folded these into one).
#   (U) 17 to= definition groups (past the 16 walked): the one chain stays, DISCLOSED — pairs_unlisted="17" + next= + the legend
#       clause, and no pairs_reaching=; a 16-group control lists its pairs (no pairs_unlisted=).
#   (N) five to= definition groups no chain reaches (past the 4 bridge searches): bridges_unsearched="1" says one was not searched;
#       a 4-group control carries no bridges_unsearched= (CodeRabbit 4387143972: the cap was silent).
#   (M) MCP path_between prints the CLI's <pair>/<alt>/<s> rows on A, B, C, J.
#
# Usage: test/pathpaircheck.sh [BIN]   (BIN defaults to RIPWIRE_BIN, then build/ripwire)
set -u
ROOT="$( cd "$( dirname "$0" )/.." && pwd )"
BIN="${1:-${RIPWIRE_BIN:-$ROOT/build/ripwire}}"
[ "${BIN#/}" = "$BIN" ] && BIN="$ROOT/$BIN"
echo "pathpaircheck: BIN=$BIN"
TMP="$( mktemp -d )"; trap 'rm -rf "$TMP"' EXIT
fail=0
ok(){ printf '  PASS  %s\n' "$*" || { fail=1; printf '  FAIL  could not write the PASS line for: %s\n' "$*"; }; return 0; }
no(){ printf '  FAIL  %s\n' "$*"; fail=1; }

[ -x "$BIN" ] || { echo "no ripwire binary at $BIN — build first"; exit 2; }
command -v python3 >/dev/null 2>&1 || { echo "python3 required"; exit 2; }

mkdir -p "$TMP/min" "$TMP/site" "$TMP/capd" "$TMP/capok" "$TMP/wide" "$TMP/two" "$TMP/hedge" "$TMP/many" "$TMP/bcap" "$TMP/bok" "$TMP/one" "$TMP/none" "$TMP/join"
cat > "$TMP/two/m.go" <<'EOF'
package m

type A struct{}
type B struct{}

func leaf() int { return 1 }
func midA() int { return leaf() }
func (a A) Run() int { return midA() }
func (b B) Run() int { return leaf() }
EOF
cat > "$TMP/hedge/h.py" <<'EOF'
class Relay:
    def relay(self):
        return target()

def target():
    return 1

class Wrong:
    def go(self):
        return target()

class Gold:
    def go(self, x):
        return x.relay()
EOF
cat > "$TMP/site/a.js" <<'EOF'
function sink() { return 1; }
function helper() { return sink(); }
module.exports = {
  compile: function compile(src) { return helper(); }
};
EOF
cat > "$TMP/site/c.js" <<'EOF'
const a = require('./a');
function start(x) { return a.compile(x); }
EOF
cat > "$TMP/min/m.js" <<'EOF'
class A{render(){return 1}}class B{render(){return 2}}
function viaA(a){return a.render()}
function viaB(b){return b.render()}
function start(){const a=new A();const b=new B();viaA(a);viaB(b);a.render();b.render()}
module.exports={A,B,start}
EOF
python3 - "$TMP" <<'PY'
import sys
t = sys.argv[1]
for d, n in ( ( "capd", 17 ), ( "capok", 16 ) ):
    open( t + "/" + d + "/k.py", "w" ).write( "".join( "class C%d:\n    def target(self):\n        return 1\n\n" % i for i in range( n ) ) + "def start(x):\n    return x.target()\n" )
for d, n in ( ( "bcap", 5 ), ( "bok", 4 ) ):   # two go() definitions reach the module target; n more target() definitions nothing reaches
    open( t + "/" + d + "/k.py", "w" ).write( "def target():\n    return 1\n\n" + "".join( "class G%d:\n    def go(self):\n        return target()\n\n" % i for i in range( 2 ) )
                                              + "".join( "class U%d:\n    def target(self):\n        return 2\n\n" % i for i in range( n ) ) )
open( t + "/many/k.py", "w" ).write( "def target():\n    return 1\n\n" + "".join( "class C%d:\n    def go(self):\n        return mid%d()\n\ndef mid%d():\n    return target()\n\n" % ( i, i, i ) for i in range( 5 ) ) )
open( t + "/wide/w.py", "w" ).write( "def target():\n    return 1\n\n" + "".join( "class C%d:\n    def go(self):\n        return target()\n\n" % i for i in range( 20 ) ) )
open( t + "/one/o.py", "w" ).write( "def target():\n    return 1\n\nclass A:\n    def go(self):\n        return target()\n\nclass B:\n    def go(self):\n        return 2\n" )
open( t + "/none/n.py", "w" ).write( "def target():\n    return 1\n\nclass A:\n    def go(self):\n        return 3\n\nclass B:\n    def go(self):\n        return 2\n" )
open( t + "/join/j.py", "w" ).write( "def target():\n    return 1\n\ndef m3():\n    return target()\n\ndef m2():\n    return m3()\n\ndef m1():\n    return m2()\n\n"
                                     "class A:\n    def go(self):\n        return m1()\n\nclass B:\n    def go(self):\n        return a_hop()\n\ndef a_hop():\n    return m1()\n" )
PY
run(){ ( cd "$TMP/$1" || exit 1; "$BIN" "$TMP/$1" --no-cache "--path=$2" "${@:3}" </dev/null ); }
run two  Run,leaf     > "$TMP/A.out"; run hedge go,target > "$TMP/B.out"; run many go,target > "$TMP/C.out"
run one  go,target    > "$TMP/D.out"; run none  go,target > "$TMP/E.out"; run join go,target > "$TMP/J.out"
run min start,render > "$TMP/V.out"; run site start,compile > "$TMP/T.out"; run capd start,target > "$TMP/U.out"; run capok start,target > "$TMP/UK.out"
run many 'g*,target'  > "$TMP/G.out"; run wide go,target > "$TMP/W.out"
run bcap go,target > "$TMP/BC.out"; run bok go,target > "$TMP/BK.out"

python3 - "$TMP" "$BIN" <<'PY'
import re, shlex, subprocess, sys
tmp, binp = sys.argv[1], sys.argv[2]
def rd( k ): return open( "%s/%s.out" % ( tmp, k ) ).read()
def root( x ):
    m = re.search( r'<path [^>]*>', x ); return dict( re.findall( r'(\w+)="([^"]*)"', m.group( 0 ) ) ) if m else {}
def pairs( x ): return re.findall( r'<pair ([^>]*)>(.*?)</pair>', x, re.S )
def attrs( s ): return dict( re.findall( r'(\w+)="([^"]*)"', s ) )
bad = []
def check( arm, cond, msg ):
    print( "  %s  (%s) %s" % ( "PASS" if cond else "FAIL", arm, msg ) )
    if not cond: bad.append( arm )
a = rd( "A" ); ra, pa = root( a ), pairs( a )
check( "A", ra.get( "pairs_reaching" ) == "2" and len( pa ) == 2 and all( attrs( p[0] ).get( "hedged" ) == "0" for p in pa )
       and [ attrs( p[0] ).get( "hops" ) for p in pa ] == [ "1", "2" ], "two all-proven pairs, 1-hop first: %r" % [ p[0] for p in pa ] )
b = rd( "B" ); rb, pb = root( b ), pairs( b )
hb = [ attrs( p[0] ) for p in pb ]
check( "B", rb.get( "pairs_reaching" ) == "2" and len( pb ) == 2 and hb[0].get( "hedged" ) == "0" and hb[1].get( "hedged" ) == "1"
       and 'via="name"' in pb[1][1] and 'via="name"' not in pb[0][1] and "h.py:13" in hb[1].get( "from_p", "" ) and "via=name" in b,
       "proven chain first, the hedged one labelled hedged=1 with its via=name row: %r" % hb )
c = rd( "C" ); rc, pc = root( c ), pairs( c )
alts = [ attrs( s ) for s in re.findall( r'<alt ([^>]*)/>', c ) ]
check( "C", rc.get( "pairs_reaching" ) == "5" and rc.get( "alt" ) == "2" and len( pc ) == 3 and len( alts ) == 2 and rc.get( "next", "" ).startswith( "--path=" ),
       "3 chains + alt=2 + 2 <alt> rows + next=: %r" % { k: rc.get( k ) for k in ( "pairs_reaching", "alt", "next" ) } )
if rc.get( "next" ):
    argv = shlex.split( rc[ "next" ].replace( "&apos;", "'" ) )
    o = subprocess.run( [ binp, tmp + "/many", "--no-cache" ] + argv, capture_output = True, text = True, cwd = tmp + "/many", stdin = subprocess.DEVNULL ).stdout
    r2 = root( o )
    check( "C", r2.get( "reachable" ) == "1" and r2.get( "from_defs" ) == "1" and alts and r2.get( "from_p" ) == alts[0].get( "from_p" ) and "pairs_reaching" not in r2,
           "next= replays the first alt as one pair: %r" % { k: r2.get( k ) for k in ( "reachable", "from_defs", "from_p" ) } )
w = rd( "W" ); rw_ = root( w )
check( "W", rw_.get( "pairs_reaching" ) == "20" and rw_.get( "alt" ) == "17" and rw_.get( "alt_capped" ) == "1" and len( re.findall( r'<alt ', w ) ) == 12 and len( pairs( w ) ) == 3,
       "20 pairs: 3 chains, alt=17, 12 rows, alt_capped=1: %r" % { k: rw_.get( k ) for k in ( "pairs_reaching", "alt", "alt_capped" ) } )
check( "W", "alt_capped" not in root( rd( "C" ) ), "alt_capped= absent when every alt row prints" )
d = rd( "D" ); rd_ = root( d )
check( "D", "pairs_reaching" not in rd_ and "<pair" not in d and rd_.get( "reachable" ) == "1" and re.search( r'<path [^>]*><s ', d ) is not None,
       "exactly one reaching pair keeps the hop rows under <path>, no pairs_reaching=" )
e = rd( "E" )
check( "E", "pairs_reaching" not in e and "<pair" not in e and root( e ).get( "reachable" ) == "0", "no reaching pair keeps today's reachable=0 answer" )
j = rd( "J" ); pj = pairs( j )
check( "J", len( pj ) == 2 and attrs( pj[1][0] ).get( "joins" ) == "1" and len( re.findall( r'<s ', pj[1][1] ) ) == 3 and attrs( pj[1][0] ).get( "hops" ) == "5",
       "shared tail: pair 2 joins=1 after 3 rows, hops=5 counts the whole chain: %r" % [ p[0] for p in pj ] )
g = rd( "G" )
check( "G", "pairs_reaching" not in g and root( g ).get( "reachable" ) == "1", "a glob endpoint keeps today's answer" )
t = rd( "T" ); rt = root( t )
check( "T", "pairs_reaching" not in rt and "<pair" not in t and rt.get( "reachable" ) == "1" and rt.get( "to_defs" ) == "2",
       "one JS site = one pair, today's bytes: %r" % { k: rt.get( k ) for k in ( "pairs_reaching", "to_defs", "reachable" ) } )
v = rd( "V" ); rv = root( v )
check( "V", rv.get( "pairs_reaching" ) == "2" and len( pairs( v ) ) == 2 and rv.get( "to_defs" ) == "2",
       "minified same-line siblings stay two pairs: %r" % { k: rv.get( k ) for k in ( "pairs_reaching", "to_defs", "reachable" ) } )
u = rd( "U" ); ru = root( u )
check( "U", ru.get( "pairs_unlisted" ) == "17" and "pairs_reaching" not in ru and "<pair" not in u and ru.get( "reachable" ) == "1" and ru.get( "next", "" ).startswith( "--path=" ),
       "17 to= groups: the one chain, disclosed with pairs_unlisted= + next=: %r" % { k: ru.get( k ) for k in ( "pairs_unlisted", "pairs_reaching", "next", "to_defs" ) } )
if ru.get( "next" ):
    argv = shlex.split( ru[ "next" ].replace( "&apos;", "'" ) )
    o = subprocess.run( [ binp, tmp + "/capd", "--no-cache" ] + argv, capture_output = True, text = True, cwd = tmp + "/capd", stdin = subprocess.DEVNULL ).stdout
    r2 = root( o )
    check( "U", r2.get( "to_defs" ) == "1" and "pairs_unlisted" not in r2, "next= narrows to one to= definition: %r" % { k: r2.get( k ) for k in ( "to_defs", "reachable", "pairs_unlisted" ) } )
uk = rd( "UK" ); ruk = root( uk )
check( "U", "pairs_unlisted" not in ruk and ruk.get( "pairs_reaching", "" ) != "", "16 to= groups are walked and listed (no pairs_unlisted=): %r" % { k: ruk.get( k ) for k in ( "pairs_unlisted", "pairs_reaching" ) } )
bc = rd( "BC" ); rbc = root( bc ); bk = rd( "BK" ); rbk = root( bk )
check( "N", rbc.get( "pairs_reaching" ) == "2" and rbc.get( "to_defs" ) == "6" and rbc.get( "bridges_unsearched" ) == "1",
       "5 unreached to= groups, 4 bridge searches: the 5th is counted, not silent (CodeRabbit 4387143972): %r" % { k: rbc.get( k ) for k in ( "pairs_reaching", "to_defs", "bridges_unsearched" ) } )
check( "N", rbk.get( "pairs_reaching" ) == "2" and rbk.get( "to_defs" ) == "5" and "bridges_unsearched" not in rbk,
       "4 unreached to= groups are all searched: no bridges_unsearched=: %r" % { k: rbk.get( k ) for k in ( "pairs_reaching", "to_defs", "bridges_unsearched" ) } )
xbad = []
for k in ( "A", "B", "C", "J" ):
    for at, body in pairs( rd( k ) ):
        h = attrs( at )
        if "joins" in h or "bridge" in h: continue
        marked = sum( 1 for s in re.findall( r'<s [^>]*/>', body )[1:] if re.search( r'via="|prov="split"|anchor="missing"', s ) )
        if str( marked ) != h.get( "hedged" ): xbad.append( ( k, at ) )
check( "X", not xbad, "hedged= equals the marked rows of every whole printed chain: %r" % xbad )
sys.exit( 1 if bad else 0 )
PY
[ $? -eq 0 ] || fail=1

echo "(L) legend clause and well-formedness"
run many go,target --legend=full > "$TMP/CL.out"; run one go,target --legend=full > "$TMP/DL.out"
if grep -q 'pairs_reaching=N: N from=/to= definition pairs reach each other' "$TMP/CL.out" && ! grep -q 'pairs_reaching=N' "$TMP/DL.out"; then
    ok "(L) the full legend carries the pairs clause exactly when pairs print"
else
    no "(L) the pairs clause rides wrongly (with pairs: $(grep -c 'pairs_reaching=N' "$TMP/CL.out"), without: $(grep -c 'pairs_reaching=N' "$TMP/DL.out"))"
fi
run capd start,target --legend=full > "$TMP/UL.out"
if grep -q 'pairs_unlisted=G: to= names G definition groups' "$TMP/UL.out" && ! grep -q 'pairs_unlisted=G' "$TMP/DL.out"; then
    ok "(L) the full legend carries the pairs_unlisted clause exactly when a cap disclosure prints"
else
    no "(L) the pairs_unlisted clause rides wrongly"
fi
run bcap go,target --legend=full > "$TMP/BL.out"
if grep -q 'bridges_unsearched=K' "$TMP/BL.out" && grep -q 'first 4 ' "$TMP/CL.out" && ! grep -q 'bridges_unsearched=K' "$TMP/CL.out"; then
    ok "(L) the pairs clause names the 4-search cap; the bridges_unsearched clause rides exactly when a search was skipped"
else
    no "(L) the bridge-search cap is not stated in the pairs legend"
fi
if command -v xmllint >/dev/null 2>&1; then
    xb=0; for k in A B C J CL T V U UL BL; do xmllint --noout "$TMP/$k.out" 2>/dev/null || { xb=1; echo "    $k.out is not well-formed"; }; done
    if [ $xb -eq 0 ]; then ok "(L) A B C J and the full-legend answer parse as XML"; else no "(L) malformed answer"; fi
else
    echo "  SKIP  (L) xmllint absent"
fi

echo "(M) MCP path_between serves the CLI's pair rows"
python3 - "$BIN" "$TMP" <<'PY'
import json, re, subprocess, sys
binp, tmp = sys.argv[1], sys.argv[2]
cases = [ ( "two", "Run", "leaf" ), ( "hedge", "go", "target" ), ( "many", "go", "target" ), ( "join", "go", "target" ) ]
reqs = [ { "jsonrpc": "2.0", "id": 0, "method": "initialize", "params": { "protocolVersion": "2024-11-05", "capabilities": {}, "clientInfo": { "name": "g", "version": "1" } } } ]
for i, ( d, a, b ) in enumerate( cases ):
    reqs.append( { "jsonrpc": "2.0", "id": i + 1, "method": "tools/call", "params": { "name": "path_between", "arguments": { "path": tmp + "/" + d, "from": a, "to": b } } } )
p = subprocess.run( [ binp, "--mcp", "--mcp-legend=inline" ], input = "".join( json.dumps( r ) + "\n" for r in reqs ), capture_output = True, text = True, timeout = 120 )
resp = {}
for line in p.stdout.splitlines():
    try:
        o = json.loads( line )
    except ValueError:
        continue
    if isinstance( o.get( "id" ), int ) and o[ "id" ] > 0:
        resp[ o[ "id" ] ] = "".join( c.get( "text", "" ) for c in o.get( "result", {} ).get( "content", [] ) )
bad = 0
rows = lambda x: re.findall( r'<pair [^>]*>|<alt [^>]*/>|<s [^>]*/>|</pair>', x )
for i, ( d, a, b ) in enumerate( cases ):
    mcp = resp.get( i + 1, "" )
    cli = open( "%s/%s.out" % ( tmp, { "two": "A", "hedge": "B", "many": "C", "join": "J" }[ d ] ) ).read()
    mr, cr = rows( mcp ), rows( cli )
    if not any( r.startswith( "<pair" ) for r in cr ):
        print( "  %s: CLI premise: no <pair> rows" % d ); bad = 1
    elif mr != cr:
        print( "  %s: MCP rows %r vs CLI %r" % ( d, mr[:4], cr[:4] ) ); bad = 1
    m = re.search( r'pairs_reaching="(\d+)"', mcp ); c = re.search( r'pairs_reaching="(\d+)"', cli )
    if not m or not c or m.group( 1 ) != c.group( 1 ):
        print( "  %s: pairs_reaching= differs MCP vs CLI" % d ); bad = 1
sys.exit( bad )
PY
mrc=$?
if [ "$mrc" -eq 0 ]; then ok "(M) path_between pair rows and pairs_reaching= equal the CLI's on A B C J"; else no "(M) MCP path_between disagrees with the CLI (rc=$mrc, details above)"; fi

if [ "$fail" -eq 0 ]; then echo "pathpaircheck: PASS"; exit 0; fi
echo "pathpaircheck: FAIL"; exit 1
