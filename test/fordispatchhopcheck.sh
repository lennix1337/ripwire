#!/usr/bin/env bash
# fordispatchhopcheck.sh — THE DISPATCH HOP of a name-exact --for answer (lane mf1-nx wave 4; src/fordispatchhop.h).
#
# THE CONTRACT THIS PINS (arms tagged [P] are RED on the pre-change binary 12056c16; [G] arms are twins and near-miss negatives
# that hold on both binaries and must keep holding — PROCESS rule 4, no gate erosion):
#
#   TRIGGER  CLI --for (XML, compact and full legends) on the name-exact route, DEFAULT regime, when a body candidate (a function
#            or method) makes a call the resolver declined at an interface/trait receiver (gate="iface") AND the task names that
#            call as a member access (`recv->m`, `recv.m`) — not the caller's own name.
#   WHAT     <dispatch call= in= p=file:line cands= shown= [test=] capped= next="--whereis=NAME">: the non-test candidates as
#            <s t= n= p=file:line> rows, the callers of a candidate owner's constructor as <s … ctor=> rows, the one candidate's
#            body <b> when exactly one production candidate is left (test doubles counted in test=).
#   NOT      a task that does not name the call, a call the resolver bound, a wrapper calling its own name, a type subject, --json,
#            --signatures-only, --token-budget=N and RIPWIRE_NO_DISPATCH_HOP=1 carry none (byte-identical to the hop-off answer).
#
# Arms:
#   P1 [P] C++ field receiver (Loader::Load src_->Fetch): rows DiskSource::Fetch and NetSource::Fetch, cands="2" shown="2", no <b>
#   P2 [P] one production candidate + a test double (tests/loader_test.cc FakeSource): shown="1" test="1" and DiskSource::Fetch's body
#   P3 [P] Rust &dyn Trait parameter (build_book r.render): Html::render, Cmd::render and pick_renderers ctor="Cmd::new"
#   P4 [P] the clause rides in both legends when the element does, and defines every attribute the element carries
#   P5 [P] prefix identity: the answer minus <dispatch> and its clause, est_tokens masked, equals RIPWIRE_NO_DISPATCH_HOP=1
#   P6 [G] next= pasted (--whereis=Fetch) lists every candidate the element names
#   N1 [G] no element: the bare subject (build_book), the call as a bare word (Loader::Load the Fetch), a wrapper calling its own
#          name (CachedSource::Fetch inner_->Fetch), a type subject (DiskSource src_->Fetch), a call declined for another
#          reason (Python job.run: the ladder tier, no gate — dispatch_job job.run)
#   N2 [G] no element, byte-identical to RIPWIRE_NO_DISPATCH_HOP=1: --json, --signatures-only, --token-budget=3000
#   M1 [P] MCP `for` parity: the same element (rows, no inline body — that surface serves bodies by handle) with its own clause;
#          the bare subject carries none
#   X1 [G] determinism  X2 [G] xmllint well-formed  X3 [P] the clause present, no "--" inside a legend comment
#
# Usage:  bash test/fordispatchhopcheck.sh [BIN]   |   RIPWIRE_BIN=asan/ripwire bash test/fordispatchhopcheck.sh
# BIN is $1, else RIPWIRE_BIN, else build/ripwire; the first output line names the binary used. Exits non-zero on any failure.

set -u
ROOT="$( cd "$( dirname "$0" )/.." && pwd )"
. "$ROOT/test/lib/clean-env.sh"
unset RIPWIRE_NO_DISPATCH_HOP
BIN="${1:-${RIPWIRE_BIN:-$ROOT/build/ripwire}}"
[ "${BIN#/}" = "$BIN" ] && BIN="$ROOT/$BIN"
echo "fordispatchhopcheck: BIN=$BIN"
fail=0
ok(){ printf '  PASS  %s\n' "$*" || { fail=1; printf '  FAIL  could not write the PASS line for: %s\n' "$*"; }; return 0; }
no(){ printf '  FAIL  %s\n' "$*"; fail=1; }
verdict(){ if [ "$1" -eq 0 ]; then ok "$2"; else no "$3"; fi; }   # rc, the PASS line, the FAIL line

[ -x "$BIN" ] || { echo "no ripwire binary at $BIN — build first"; exit 2; }
command -v python3 >/dev/null 2>&1 || { echo "python3 required"; exit 2; }
cd "$ROOT"
F="test/fordispatchhopfix"
[ -f "$F/cpp/loader.cc" ] && [ -f "$F/cpp1/tests/loader_test.cc" ] && [ -f "$F/rs/src/pick.rs" ] && [ -f "$F/cppwrap/cached.cc" ] && [ -f "$F/ladder/main.py" ] || { echo "fixture $F missing"; exit 2; }

TMP="$( mktemp -d )"; trap 'rm -rf "$TMP"' EXIT
export HOME="$TMP/home"; mkdir -p "$HOME"

run(){ local dir="$1"; shift; "$BIN" "$dir" --no-cache "$@" 2>/dev/null </dev/null; }
off(){ local dir="$1"; shift; RIPWIRE_NO_DISPATCH_HOP=1 "$BIN" "$dir" --no-cache "$@" 2>/dev/null </dev/null; }
hop(){ python3 - "$1" <<'PY'
import re, sys
t = open(sys.argv[1], encoding="utf-8", errors="replace").read()
print("".join(re.findall(r'<dispatch [^>]*>.*?</dispatch>', t, re.S)))
PY
}
strip(){ python3 - "$1" <<'PY'
import re, sys
t = open(sys.argv[1], encoding="utf-8", errors="replace").read()
t = re.sub(r'<dispatch [^>]*>.*?</dispatch>', '', t, flags=re.S)
t = re.sub(r'; dispatch call=.*?every definition', '', t, flags=re.S)
t = re.sub(r' est_tokens="\d+"', '', t)
sys.stdout.write(t)
PY
}
has(){ printf '%s' "$1" | grep -qF -- "$2"; }

# ── P1 ────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
run "$F/cpp" "--for=Loader::Load src_->Fetch" >"$TMP/p1"
H1="$( hop "$TMP/p1" )"
if grep -q 'route="name-exact(' "$TMP/p1"; then ok "(P1pre) the C++ subject routes name-exact"; else no "(P1pre) the C++ subject did not route name-exact — the fixture proves nothing"; fi
if has "$H1" 'in="Loader::Load"' && has "$H1" 'cands="2" shown="2"' && has "$H1" 'n="DiskSource::Fetch" p="loader.cc:14"' && has "$H1" 'n="NetSource::Fetch" p="loader.cc:15"' \
   && has "$H1" 'next="--whereis=Fetch"' && ! has "$H1" '<b '; then
    ok "(P1) the field-receiver call names both implementations, no body (two candidates)"
else no "(P1) expected DiskSource::Fetch + NetSource::Fetch rows, cands=2 shown=2, no <b>: [$H1]"; fi

# ── P2 ────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
run "$F/cpp1" "--for=Loader::Load src_->Fetch" >"$TMP/p2"
H2="$( hop "$TMP/p2" )"
if has "$H2" 'cands="2" shown="1" test="1"' && has "$H2" 'n="DiskSource::Fetch"' && ! has "$H2" 'FakeSource' && has "$H2" 'int doubled = k * 2;'; then
    ok "(P2) one production candidate: the test double is counted (test=1), the candidate's body rides"
else no "(P2) expected shown=1 test=1, DiskSource::Fetch with its body, no FakeSource: [$H2]"; fi

# ── P3 ────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
run "$F/rs" "--for=build_book r.render" >"$TMP/p3"
H3="$( hop "$TMP/p3" )"
if has "$H3" 'call="render" in="build_book"' && has "$H3" 'n="Html::render"' && has "$H3" 'n="Cmd::render"' && has "$H3" 'n="pick_renderers" p="src/pick.rs:22" ctor="Cmd::new"'; then
    ok "(P3) Rust &dyn Trait: both impls and the function constructing Cmd (ctor=Cmd::new)"
else no "(P3) expected Html::render, Cmd::render and pick_renderers ctor=Cmd::new: [$H3]"; fi

# ── P4 ────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
run "$F/cpp" "--for=Loader::Load src_->Fetch" --legend=full >"$TMP/p4f"
if grep -q '; dispatch call=: the task names a call' "$TMP/p4f" && grep -q '; dispatch call= in= p=: a task-named call' "$TMP/p1" \
   && [ -n "$( hop "$TMP/p4f" )" ]; then ok "(P4) the clause rides in both legends with the element"
else no "(P4) the dispatch clause is missing from a legend that carries the element"; fi
for a in call in p cands shown test capped next ctor; do
    grep -qE "; dispatch ([^;]*[ (])?$a=" "$TMP/p4f" || no "(P4) full clause does not define $a="
    grep -qE "; dispatch ([^;]*[ (])?$a=" "$TMP/p1" || no "(P4) compact clause does not define $a="
done

# ── P5 ────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
for leg in compact full; do
    run "$F/cpp1" "--for=Loader::Load src_->Fetch" --legend=$leg >"$TMP/p5a"
    off "$F/cpp1" "--for=Loader::Load src_->Fetch" --legend=$leg >"$TMP/p5b"
    strip "$TMP/p5a" >"$TMP/p5as"; strip "$TMP/p5b" >"$TMP/p5bs"
    if [ -n "$( hop "$TMP/p5a" )" ] && cmp -s "$TMP/p5as" "$TMP/p5bs" && [ -z "$( hop "$TMP/p5b" )" ]; then ok "(P5 $leg) the answer minus the element equals RIPWIRE_NO_DISPATCH_HOP=1"
    else no "(P5 $leg) prefix identity broken: $( diff "$TMP/p5as" "$TMP/p5bs" | head -c 400 )"; fi
done

# ── P6 ────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
run "$F/cpp" --whereis=Fetch >"$TMP/p6"
if grep -q 'DiskSource::Fetch' "$TMP/p6" && grep -q 'NetSource::Fetch' "$TMP/p6"; then ok "(P6) next= --whereis=Fetch lists every candidate"
else no "(P6) --whereis=Fetch does not list both candidates"; fi

# ── N1 ────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
n1(){ local dir="$1" task="$2" why="$3"
    run "$dir" "--for=$task" >"$TMP/n1"
    if [ -s "$TMP/n1" ] && [ -z "$( hop "$TMP/n1" )" ] && ! grep -q '; dispatch call=' "$TMP/n1"; then ok "(N1) no element: $why"
    else no "(N1) an element or clause rode on $why: [$( hop "$TMP/n1" )]"; fi
}
n1 "$F/rs" "build_book" "the bare subject (name-exact, its body makes the iface call)"
run "$F/rs" --for=build_book | grep -q 'route="name-exact(build_book)'
verdict $? "(N1pre) the bare subject routes name-exact (the task-names guard, not the route, holds it)" "(N1pre) build_book did not route name-exact"
n1 "$F/cpp" "Loader::Load the Fetch" "the call as a bare word"
n1 "$F/cppwrap" "CachedSource::Fetch inner_->Fetch" "a wrapper calling its own name"
n1 "$F/cpp" "DiskSource src_->Fetch" "a type subject"
n1 "$F/ladder" "dispatch_job job.run" "a call declined by the ladder tier (not gate=iface)"
if run "$F/ladder" "--for=dispatch_job job.run" | grep -q 'route="name-exact(' && run "$F/ladder" --callees=dispatch_job --legend=full | grep -q '<dc n="run"[^>]*cands="2"'; then
    ok "(N1pre) the ladder fixture routes name-exact and its job.run IS declined with 2 candidates (the gate=iface guard holds it)"
else no "(N1pre) the ladder fixture's premise failed"; fi
run "$F/cppwrap" "--for=CachedSource::Fetch inner_->Fetch" --legend=full >"$TMP/n1w"
grep -q 'route="name-exact(' "$TMP/n1w" && run "$F/cppwrap" --callees=CachedSource::Fetch --legend=full | grep -q 'gate="iface"'
verdict $? "(N1pre) the wrapper routes name-exact and its call IS iface-declined (the guard, not the premise, holds it)" "(N1pre) the wrapper fixture's premise failed (route or the iface decline)"

# ── N2 ────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
for flag in --json --signatures-only --token-budget=3000; do
    run "$F/cpp" "--for=Loader::Load src_->Fetch" "$flag" >"$TMP/n2a"; off "$F/cpp" "--for=Loader::Load src_->Fetch" "$flag" >"$TMP/n2b"
    if [ -s "$TMP/n2a" ] && cmp -s "$TMP/n2a" "$TMP/n2b"; then ok "(N2) $flag: byte-identical to RIPWIRE_NO_DISPATCH_HOP=1"
    else no "(N2) $flag differs from the hop-off answer"; fi
done

# ── M1 ────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
mcp_for(){ printf '{"jsonrpc":"2.0","id":1,"method":"tools/call","params":{"name":"for","arguments":{"path":"%s","task":"%s"}}}\n' "$ROOT/$1" "$2" \
           | "$BIN" --mcp 2>/dev/null | python3 -c '
import sys, json
for l in sys.stdin:
    try: j = json.loads(l)
    except ValueError: continue
    sys.stdout.write("".join(c.get("text", "") for c in j.get("result", {}).get("content", [])))'; }
mcp_for "$F/rs" "build_book r.render" >"$TMP/m1"
mcp_for "$F/cpp" "Loader::Load" >"$TMP/m1n"
M1="$( hop "$TMP/m1" )"
if [ -n "$M1" ] && [ "$M1" = "$H3" ] && grep -q 'dispatch call=: the task names a call' "$TMP/m1" && [ -s "$TMP/m1n" ] && [ -z "$( hop "$TMP/m1n" )" ]; then
    ok "(M1) MCP for carries the CLI's element byte-for-byte (no body on this fixture) with its clause; the bare subject none"
else no "(M1) MCP for: [$M1] vs CLI [$H3]"; fi
H2M="$( mcp_for "$F/cpp1" "Loader::Load src_->Fetch" >"$TMP/m1b"; hop "$TMP/m1b" )"
if has "$H2M" 'shown="1" test="1"' && ! has "$H2M" '<b '; then ok "(M1b) MCP for: the one-candidate element carries no inline body"
else no "(M1b) MCP one-candidate element: [$H2M]"; fi

# ── X ─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
run "$F/rs" "--for=build_book r.render" >"$TMP/x1"
cmp -s "$TMP/p3" "$TMP/x1"; verdict $? "(X1) deterministic" "(X1) two runs differ"
if command -v xmllint >/dev/null 2>&1; then
    for f in p1 p2 p3 p4f; do xmllint --noout "$TMP/$f" 2>/dev/null || no "(X2) $f is not well-formed XML"; done
    ok "(X2) xmllint checked"
else ok "(X2) xmllint absent — skipped"; fi
for f in p1 p4f; do
    python3 - "$TMP/$f" <<'PY' || no "(X3) a '--' inside the dispatch clause ($f)"
import re, sys
t = open(sys.argv[1], encoding="utf-8").read()
m = re.search(r'; dispatch call=[^;]*', t)
sys.exit(1 if m is None or "--" in m.group(0) else 0)
PY
done
ok "(X3) clause scanned"

exit "$fail"
