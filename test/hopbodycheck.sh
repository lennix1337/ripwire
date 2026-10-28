#!/usr/bin/env bash
# hopbodycheck.sh — THE BODY OF A MIDDLE HOP (lane path-hop-body, train 41; src/hopbody.h).
#
# THE CONTRACT THIS PINS (arms tagged [P] are RED on the pre-change binary 5163ffbdb; [G] arms are twins and near-miss negatives
# that hold on both binaries and must keep holding — PROCESS rule 4, no gate erosion):
#
#   PATH-HOP-BODY  a reached --path (and MCP path_between) whose shown chain has 2 hops prints, after its hop rows,
#                  <bodies of="hops" shown= total= capped=> holding the body of its INTERMEDIATE hop (never an endpoint), each
#                  the --expand body element (b t= l= p= n=, CDATA = the definition's source text, no callee rows). A body
#                  over 4000 B is cut at a line end and says so (truncated="1" lines="1-K/T" next= serves the rest). A chain of
#                  more hops prints no body and its root carries next= (the first middle hop's --expand). Inside a --for answer's
#                  graph-verb block a middle hop whose text the answer already carries (a served class holding it) prints no copy.
#   WRAPPER-FOLLOW a --for fold body that is a THIN DELEGATOR (one proven callee, no other call, at most 3 statement lines) is
#                  followed by its callee's body <b follow_of="WRAPPER">, at most 2 levels, never a text the answer already
#                  carries; a by-name edge, a second call, or a fourth statement line is not a delegator.
#   LEGEND         every new reading is present-only: the compact path reading `bodies of=hops`, the full path clause, the
#                  fold's follow_of= clause ride exactly when the element/attribute does.
#   A/B            RIPWIRE_NO_HOP_BODY=1 is the answer without either mechanism.
#
# Arms (colour on 5163ffbdb in brackets):
#   P1 [P] fetch -> dispatch -> compose: one <bodies of="hops"> with dispatch's body, CDATA = its source text; no endpoint body
#   P2 [P] a 3-hop chain (past the gate) prints no body; root next="--expand=src/chain.ts:5:stepTwo"
#   P3 [P] a 4-hop chain prints no body; root next="--expand=src/chain.ts:5:stepTwo", and that call serves stepTwo's body
#   P9 [P] the --for graph-verb block: gatewayEntry -> relayHop prints relayHop's body (P9a); Server.fetch's twin, whose
#          answer already serves the Server class holding dispatch, prints no copy (P9b) — kills a NodeId-only dedupe
#   P4 [P] a middle body over the guard: truncated="1" lines="1-K/T" next=, capped="1", served text <= 4000 B, next= serves the rest
#   P5 [G] a 1-hop chain and an unreached pair: no <bodies>, no next=
#   P6 [P] legend present-only, compact and full: the reading rides iff the element does; the past-the-gate next= reading in full
#   P7 [P] MCP path_between prints the CLI's <bodies of="hops"> element byte for byte (P1, P4) and the 3- and 4-hop next=
#   P8 [G] RIPWIRE_NO_HOP_BODY=1: the path answer is the head answer minus the element, its clause and the 4-hop next=
#   F1 [P] the 2-level follow: handleRequest -> forwardInput -> runPipeline, each <b follow_of=> right after its wrapper,
#          CDATA = the source text; the follow_of clause rides
#   F2 [G] a one-call body of four statement lines (handleTimedRequest) is never followed
#   F3 [G] a two-call body (listen) is never followed; a delegate inside a served body (fetch -> dispatch in Server) prints no copy
#   F4 [G] a by-name delegate (worker.crunchInput, via=name) is never followed
#   F5 [G] each text once: no two <b> of one answer share p= and l=
#   F6 [G] RIPWIRE_NO_HOP_BODY=1: no follow_of=, no clause, answer == head minus the follow bodies and clause
#   X1 [G] determinism   X2 [G] xmllint well-formed   X3 [G] no "--" inside a legend comment
#   I  [optional, RIPWIRE_BASE_BIN=<pre-change binary>] RIPWIRE_NO_HOP_BODY=1 head == the base binary on every question here
#
# Usage:  bash test/hopbodycheck.sh [BIN]   |   RIPWIRE_BIN=asan/ripwire bash test/hopbodycheck.sh
# BIN is $1, else RIPWIRE_BIN, else build/ripwire; the first output line names the binary used. Exits non-zero on any failure.

set -u
ROOT="$( cd "$( dirname "$0" )/.." && pwd )"
. "$ROOT/test/lib/clean-env.sh"
unset RIPWIRE_NO_HOP_BODY RIPWIRE_NO_BODY_FOLD RIPWIRE_NO_BODY_SLOT
BIN="${1:-${RIPWIRE_BIN:-$ROOT/build/ripwire}}"
[ "${BIN#/}" = "$BIN" ] && BIN="$ROOT/$BIN"
echo "hopbodycheck: BIN=$BIN"
fail=0
[ -x "$BIN" ] || { echo "no ripwire binary at $BIN — build first"; exit 2; }
command -v python3 >/dev/null 2>&1 || { echo "python3 required"; exit 2; }
cd "$ROOT"
WEB="test/hopbodyfix/web"; FOL="test/hopbodyfix/follow"; LOO="test/hopbodyfix/loose"
[ -d "$WEB/src" ] && [ -d "$FOL/src" ] && [ -d "$LOO/src" ] || { echo "fixture test/hopbodyfix missing"; exit 2; }

TMP="$( mktemp -d )"; trap 'rm -rf "$TMP"' EXIT
export HOME="$TMP/home"; mkdir -p "$HOME"
run(){ local d="$1"; shift; ( cd "$d" || exit 1; "$BIN" "$ROOT/$d" --no-cache "$@" 2>/dev/null ); }

T_FOLLOW="How is a request handled?"
T_TIMED="How does handling a request work?"
T_WEB="How does a request flow from fetch to its Response?"
T_LISTEN="How does the server listen for a request?"
T_LOOSE="How is a loose request handled?"

run "$WEB" --path=fetch,compose        >"$TMP/p1"
run "$WEB" --path=stepOne,stepFour     >"$TMP/p2"
run "$WEB" --path=stepOne,stepFive     >"$TMP/p3"
run "$WEB" --path=bigEntry,bigSink     >"$TMP/p4"
run "$WEB" --path=stepFour,stepFive    >"$TMP/p5a"
run "$WEB" --path=compose,fetch        >"$TMP/p5b"
run "$WEB" --for="Trace the path from gatewayEntry() to finalStage()" >"$TMP/p9a"
run "$WEB" --for="Trace the path from Server.fetch to compose()"  >"$TMP/p9b"
run "$WEB" --path=fetch,compose --legend=full    >"$TMP/p1f"
run "$WEB" --path=stepOne,stepFive --legend=full >"$TMP/p3f"
run "$WEB" --path=stepFour,stepFive --legend=full >"$TMP/p5af"
RIPWIRE_NO_HOP_BODY=1 run "$WEB" --path=fetch,compose    >"$TMP/p1.off"
RIPWIRE_NO_HOP_BODY=1 run "$WEB" --path=stepOne,stepFour >"$TMP/p2.off"
RIPWIRE_NO_HOP_BODY=1 run "$WEB" --path=stepOne,stepFive >"$TMP/p3.off"
RIPWIRE_NO_HOP_BODY=1 run "$WEB" --path=bigEntry,bigSink >"$TMP/p4.off"
run "$FOL" --for="$T_FOLLOW"           >"$TMP/f1"
run "$FOL" --for="$T_FOLLOW" --legend=full >"$TMP/f1f"
RIPWIRE_NO_HOP_BODY=1 run "$FOL" --for="$T_FOLLOW" >"$TMP/f1.off"
run "$FOL" --for="$T_TIMED"            >"$TMP/f2"
run "$WEB" --for="$T_WEB"              >"$TMP/f3a"
run "$WEB" --for="$T_LISTEN"           >"$TMP/f3b"
run "$LOO" --for="$T_LOOSE"            >"$TMP/f4"
run "$LOO" --callees=handleLooseRequest >"$TMP/f4c"

cat > "$TMP/chk.py" <<'PY'
import html, re, shlex, subprocess, sys, os
T, BIN, ROOT = sys.argv[1], sys.argv[2], sys.argv[3]
fails = 0
def rd(n): return open(os.path.join(T, n), "rb").read().decode("utf-8", "replace")
def ok(m): print("  PASS  " + m)
def no(m):
    global fails; fails += 1; print("  FAIL  " + m)
def check(c, m): ok(m) if c else no(m)
def attrs(s): return dict((k, html.unescape(v)) for k, v in re.findall(r'(\w+)="([^"]*)"', s))
def cdata(x): return "".join(re.findall(r'<!\[CDATA\[(.*?)\]\]>', x, re.S))
def bodies(x): return [(attrs(m.group(1)), cdata(m.group(2)), m.group(0)) for m in re.finditer(r'<b ([^>]*)>(.*?)</b>', x, re.S)]
def hopel(t):
    m = re.search(r'<bodies of="hops"[^>]*>.*?</bodies>', t, re.S); return m.group(0) if m else ""
def root(t):
    m = re.search(r'<path ([^>]*)>', t); return attrs(m.group(1)) if m else {}
def legend(t): return " ".join(re.findall(r'<!--(.*?)-->', t, re.S))
def is_source(b, d):
    """the body's CDATA is its definition's source text: a substring of the file p= starting on line l="""
    a, cd = b[0], b[1]
    try: src = open(os.path.join(ROOT, d, a.get("p", "")), encoding="utf-8").read()
    except OSError: return False
    i = src.find(cd)
    return cd != "" and i >= 0 and src[:i].count("\n") + 1 == int(a.get("l", "0"))
def paste(nxt, d):
    argv = shlex.split(nxt)
    return subprocess.run([BIN, os.path.join(ROOT, d), "--no-cache"] + argv, cwd=os.path.join(ROOT, d), capture_output=True).stdout.decode("utf-8", "replace")
WEB, FOL = "test/hopbodyfix/web", "test/hopbodyfix/follow"

# P1
p1 = rd("p1"); el = hopel(p1); bs = bodies(el)
check(len(bs) == 1 and bs[0][0].get("n") == "dispatch" and bs[0][0].get("l") == "10", "(P1) one middle-hop body: dispatch l=10")
check(bs and is_source(bs[0], WEB) and "return composed(request)" in bs[0][1], "(P1) its CDATA is the definition's source text, whole")
check(not re.search(r'<b [^>]*n="(fetch|compose)"', el) and "<calls" not in el and "sibs=" not in el, "(P1) no endpoint body, no callee rows, no file context")
check('of="hops" shown="1" total="1" capped="0"' in el, "(P1) envelope shown=1 total=1 capped=0")
# P2
p2 = rd("p2"); r2 = root(p2)
check(hopel(p2) == "" and r2.get("hops") == "3", "(P2) a 3-hop chain (past the gate) prints no body")
check(r2.get("next") == "--expand=src/chain.ts:5:stepTwo", "(P2) its root next= is the first middle hop's --expand: %r" % r2.get("next"))
# P9: the --for graph-verb block
gv = lambda t: (re.search(r'<graph_verb .*?</graph_verb>', t, re.S) or re.search(r'$', t)).group(0)
g9a, g9b = gv(rd("p9a")), gv(rd("p9b"))
b9a = bodies(hopel(g9a))
check(g9a != "" and len(b9a) == 1 and b9a[0][0].get("n") == "relayHop" and is_source(b9a[0], WEB), "(P9a) the graph-verb path block prints the middle hop's body")
check(re.search(r'<b [^>]*n="Server"', rd("p9b")) and g9b != "" and hopel(g9b) == "", "(P9b) a middle hop inside a body the answer serves prints no copy")
# P3
p3 = rd("p3"); r = root(p3)
check(hopel(p3) == "" and r.get("hops") == "4", "(P3) a 4-hop chain prints no body")
check(r.get("next") == "--expand=src/chain.ts:5:stepTwo", "(P3) root next= is the first middle hop's --expand: %r" % r.get("next"))
check(r.get("next") and "return stepThree(n * 2)" in cdata(paste(r["next"], WEB)), "(P3) the pasted next= serves stepTwo's body")
# P4
el = hopel(rd("p4")); bs = bodies(el); a = bs[0][0] if bs else {}
check(a.get("truncated") == "1" and re.fullmatch(r'1-\d+/\d+', a.get("lines", "")) and a.get("next", "").startswith("--expand=src/big.ts:5:bigMiddle:"), "(P4) the long middle body is cut and says so: %r" % {k: a.get(k) for k in ("truncated", "lines", "next")})
check('capped="1"' in el and bs and len(bs[0][1].encode()) <= 4000, "(P4) capped=1 and the served text is <= 4000 B")
if bs and a.get("next"):
    rest = cdata(paste(a["next"], WEB)); hi = int(a["lines"].split("-")[1].split("/")[0]); tot = int(a["lines"].split("/")[1])
    check(rest.count("\n") + 1 == tot - hi and bs[0][1].endswith("\n") is False, "(P4) next= serves exactly the lines the cut left (%d of %d)" % (tot - hi, tot))
# P5
for f in ("p5a", "p5b"):
    t = rd(f); check(hopel(t) == "" and "next=" not in (re.search(r'<path [^>]*>', t).group(0) if re.search(r'<path [^>]*>', t) else ""), "(P5) %s: no <bodies>, no root next=" % f)
check('reachable="0"' in rd("p5b"), "(P5) compose,fetch is the unreached twin")
# P6
check("bodies of=hops" in legend(p1) and "bodies of=hops" not in legend(rd("p3")) and "bodies of=hops" not in legend(rd("p5a")), "(P6) compact: the reading rides iff the element does")
check("bodies of=hops" in legend(rd("p1f")) and "bodies of=hops" not in legend(rd("p5af")), "(P6) full: the clause rides iff the element does")
check("first middle hop" in legend(rd("p3f")) and "first middle hop" not in legend(rd("p1f")), "(P6) full: the 4-hop next= reading rides iff that next= does")
# P8
for f in ("p1", "p4"):
    on, off = rd(f), rd(f + ".off")
    strip = re.sub(r'<bodies of="hops".*?</bodies>', '', on, flags=re.S)
    strip = re.sub(r' bodies of=hops: .*?(?= -->|\. (?=[a-z_]+[=:])|$)\.?', '', strip, flags=re.S)
    check(hopel(off) == "" and re.sub(r'<!--.*?-->', '', strip, flags=re.S) == re.sub(r'<!--.*?-->', '', off, flags=re.S), "(P8) %s: RIPWIRE_NO_HOP_BODY=1 == head minus the element (payload)" % f)
check('next=' not in re.search(r'<path [^>]*>', rd("p3.off")).group(0) and 'next=' not in re.search(r'<path [^>]*>', rd("p2.off")).group(0), "(P8) RIPWIRE_NO_HOP_BODY=1: no past-the-gate next=")
# F1
f1 = rd("f1"); bs = bodies(f1); names = [b[0].get("n") for b in bs]
fo = {b[0].get("n"): b for b in bs if "follow_of" in b[0]}
check(fo.get("forwardInput", [{}])[0].get("follow_of") == "handleRequest" and fo.get("runPipeline", [{}])[0].get("follow_of") == "forwardInput", "(F1) the 2-level follow: %r" % {k: v[0].get("follow_of") for k, v in fo.items()})
check("handleRequest" in names and names.index("forwardInput") == names.index("handleRequest") + 1 and names.index("runPipeline") == names.index("forwardInput") + 1 if "forwardInput" in names and "runPipeline" in names and "handleRequest" in names else False, "(F1) each follow right after its wrapper")
check("forwardInput" in fo and is_source(fo["forwardInput"], FOL) and "runPipeline" in fo and is_source(fo["runPipeline"], FOL) and "segments.length === 0" in fo["runPipeline"][1], "(F1) follow CDATA are the definitions' source text, whole")
check("follow_of=W" in legend(f1) and "follow_of=W" in legend(rd("f1f")), "(F1) the follow_of clause rides in both legends")
# F2-F4
check(not re.search(r'follow_of="handleTimedRequest"', rd("f2")) and re.search(r'<b [^>]*n="handleTimedRequest"', rd("f2")) and not re.search(r'<b [^>]*n="stampInput"', rd("f2")), "(F2) the 4-statement-line wrapper is served and never followed (its callee stampInput never prints)")
check("follow_of=" not in rd("f3a") and "follow_of=W" not in legend(rd("f3a")), "(F3) fetch's delegate lies inside the served Server body: no copy, no clause")
check(re.search(r'<b [^>]*n="listen"', rd("f3b")) and 'follow_of="listen"' not in rd("f3b"), "(F3) the two-call listen is never followed")
check('via="name"' in rd("f4c") and re.search(r'<b [^>]*n="handleLooseRequest"', rd("f4")) and "follow_of=" not in rd("f4"), "(F4) a by-name delegate is never followed")
# F5
for f in ("f1", "f2", "f3a", "f3b", "f4", "p1", "p9a", "p9b"):
    keys = [(b[0].get("p"), b[0].get("l")) for b in bodies(rd(f))]
    check(len(keys) == len(set(keys)), "(F5) %s: each text once" % f)
# F6
off = rd("f1.off")
check("follow_of" not in off, "(F6) RIPWIRE_NO_HOP_BODY=1: no follow_of= and no clause")
strip = re.sub(r'<b [^>]*follow_of="[^"]*"[^>]*>.*?</b>', '', f1, flags=re.S)
mask = lambda t: re.sub(r' est_tokens="\d+"', '', re.sub(r'<!--.*?-->', '', t, flags=re.S))
check(mask(strip) == mask(off), "(F6) the --for answer minus the follow bodies == the RIPWIRE_NO_HOP_BODY=1 answer (payload, est_tokens masked)")
# X3
for f in ("p1", "p1f", "p3f", "p4", "f1", "f1f"):
    check(all("--" not in c for c in re.findall(r'<!--(.*?)-->', rd(f), re.S)), "(X3) %s: no -- inside a legend comment" % f)
sys.exit(1 if fails else 0)
PY
python3 "$TMP/chk.py" "$TMP" "$BIN" "$ROOT" || fail=1

# P7: MCP path_between twin
python3 - "$BIN" "$ROOT/$WEB" "$TMP" <<'PY' || fail=1
import json, re, subprocess, sys
binp, web, tmp = sys.argv[1], sys.argv[2], sys.argv[3]
cases = [("fetch", "compose", "p1"), ("stepOne", "stepFour", "p2"), ("bigEntry", "bigSink", "p4"), ("stepOne", "stepFive", "p3")]
reqs = [{"jsonrpc": "2.0", "id": 0, "method": "initialize", "params": {"protocolVersion": "2024-11-05", "capabilities": {}, "clientInfo": {"name": "g", "version": "1"}}}]
for i, (a, b, _) in enumerate(cases):
    reqs.append({"jsonrpc": "2.0", "id": i + 1, "method": "tools/call", "params": {"name": "path_between", "arguments": {"path": web, "from": a, "to": b}}})
p = subprocess.run([binp, "--mcp", "--mcp-legend=inline"], input="".join(json.dumps(r) + "\n" for r in reqs), capture_output=True, text=True, timeout=120, cwd=web)
resp = {}
for line in p.stdout.splitlines():
    try: o = json.loads(line)
    except ValueError: continue
    if isinstance(o.get("id"), int) and o["id"] > 0:
        resp[o["id"]] = "".join(c.get("text", "") for c in o.get("result", {}).get("content", []))
bad = 0
el = lambda t: (re.search(r'<bodies of="hops".*?</bodies>', t, re.S) or re.search(r'$', t)).group(0)
for i, (a, b, f) in enumerate(cases):
    cli = open(tmp + "/" + f).read(); mcp = resp.get(i + 1, "")
    if f in ("p2", "p3"):
        m = re.search(r'<path [^>]*next="([^"]*)"', mcp); good = bool(m) and m.group(1) == "--expand=src/chain.ts:5:stepTwo" and el(mcp) == ""
    else:
        good = el(mcp) != "" and el(mcp) == el(cli)
    print("  %s  (P7) MCP path_between %s,%s: %s" % ("PASS" if good else "FAIL", a, b, "the CLI's element" if f not in ("p2", "p3") else "the past-the-gate next="))
    bad += 0 if good else 1
sys.exit(1 if bad else 0)
PY

# X1 determinism, X2 well-formed
run "$WEB" --path=fetch,compose >"$TMP/x1"; run "$FOL" --for="$T_FOLLOW" >"$TMP/x2"
if cmp -s "$TMP/p1" "$TMP/x1" && cmp -s "$TMP/f1" "$TMP/x2"; then echo "  PASS  (X1) deterministic"; else echo "  FAIL  (X1) two runs differ"; fail=1; fi
if command -v xmllint >/dev/null 2>&1; then
    for f in p1 p2 p3 p4 p1f f1 f1f p9a p9b; do
        if xmllint --noout "$TMP/$f" 2>/dev/null; then echo "  PASS  (X2) $f well-formed"; else echo "  FAIL  (X2) $f not well-formed"; fail=1; fi
    done
else
    echo "  SKIP  (X2) xmllint absent"
fi

# I: optional base-binary identity of the A/B handle
if [ -n "${RIPWIRE_BASE_BIN:-}" ] && [ -x "${RIPWIRE_BASE_BIN}" ]; then
    i=0
    while IFS='|' read -r d a; do
        i=$(( i + 1 ))
        ( cd "$d" || exit 1; "$RIPWIRE_BASE_BIN" "$ROOT/$d" --no-cache "$a" 2>/dev/null ) | sed 's/ at="[^"]*"//' >"$TMP/i.base"
        ( cd "$d" || exit 1; RIPWIRE_NO_HOP_BODY=1 "$BIN" "$ROOT/$d" --no-cache "$a" 2>/dev/null ) | sed 's/ at="[^"]*"//' >"$TMP/i.head"
        if [ -s "$TMP/i.base" ] && cmp -s "$TMP/i.base" "$TMP/i.head"; then echo "  PASS  (I$i) RIPWIRE_NO_HOP_BODY=1 == base: $d $a"; else echo "  FAIL  (I$i) differs from base: $d $a"; fail=1; fi
    done <<ARGV
$WEB|--path=fetch,compose
$WEB|--path=stepOne,stepFive
$WEB|--path=bigEntry,bigSink
$FOL|--for=$T_FOLLOW
$WEB|--for=$T_WEB
ARGV
else
    echo "  SKIP  (I) RIPWIRE_BASE_BIN unset or not executable — base-binary identity not run here"
fi
exit $fail
