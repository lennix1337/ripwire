#!/usr/bin/env bash
# ifacepeelcheck.sh — receiver typing that hands an interface-typed call to the dispatch tier (lane iface-peel, D1 of the
# dispatch-register design): a C++ member written as a smart-pointer ALIAS, and a Rust local initialised by a call whose
# one proven callee returns a trait object.
#
#   test/ifacepeelcheck.sh [BIN]        # BIN defaults to $RIPWIRE_BIN, then build/ripwire
#
# THE GAP. `using FilterSharedPtr = std::shared_ptr<Filter>; FilterSharedPtr handle_; handle_->decodeHeaders()` and
# `let d = state.get_decl( id ); d.run()` (`-> &dyn Command`) were matched by NAME: every in-tree `decodeHeaders` / `run`,
# an inherent namesake (`CallEval::run`, which a `dyn Command` call never reaches) included, as via="name" rows.
#
# THE CONTRACT.
#   * C++ (LangFacts::aliasPeelsSmartPointer): a member written as an alias of std::shared_ptr<I> / std::unique_ptr<I> —
#     `using`, `typedef`, or a plain alias of such an alias (chain cap 4) — is I through `->`, exactly like a member written
#     `std::shared_ptr<I>`: an interface I declines `gate="iface"` with its implementors counted; a concrete I is a typed edge.
#     Refused (base reading kept): a std pointee, two same-named aliases that disagree, an alias name that is also a class.
#   * Rust (LangFacts::returnTypedDispatchLet): `let x = r.m( … )` whose call commits ONE proven edge to a callee declared
#     `-> &dyn T` / `-> Box<dyn T>` / `-> impl T` types x as some implementor of T (the dispatch tier then declines or proves).
#     A by-name or declined callee types nothing; a generic `-> R` (R: T) is never read.
#
# ARMS (fixture test/ifacepeelfix: cpp/ rs/). [P] red on a binary without the lane; [N] near misses / controls.
#   P1 C++ using-alias / typedef / alias-of-alias field → <dc gate="iface" cands="2">, no via="name"
#   P2 Rust `let d = s.get_decl( id ); d.run()` and `Box<dyn T>` result → <dc gate="iface">, CallEval::run ABSENT
#   N1 C++ alias of a CONCRETE class → the typed edge to Codec::encodeFrame, never the namesake Other::encodeFrame
#   N2 Rust: the inherent namesake CallEval::run is never a candidate (P2's absence check)
#   N3 Rust: a by-name callee (`s.get_decl` on a generic S) types nothing — base by-name rows unchanged, no iface row
#   N4 C++ `.` on the alias member reaches the smart pointer, never the pointee (`handle_.reset()` with an I::reset: no iface row)
#   N5 C++ alias of std::shared_ptr<std::string> types nothing (an in-tree text::string keeps its base by-name row)
#   N6 Rust: a generic `-> R` (R: Command) is never read as a trait object (no iface row)
set -u
ROOT="$( cd "$( dirname "$0" )/.." && pwd )"
BIN="${1:-${RIPWIRE_BIN:-$ROOT/build/ripwire}}"
[ "${BIN#/}" = "$BIN" ] && BIN="$PWD/$BIN"
echo "ifacepeelcheck: BIN=$BIN"
[ -x "$BIN" ] || { echo "no ripwire binary at $BIN — build first"; exit 2; }
FIX="$ROOT/test/ifacepeelfix"
[ -d "$FIX" ] || { echo "fixture missing: test/ifacepeelfix"; exit 2; }
SCR="$( mktemp -d )"; trap 'rm -rf "$SCR"' EXIT
export HOME="$SCR" TMPDIR="$SCR/"
npass=0; nfail=0
say() { if [ "$1" = 1 ]; then npass=$((npass+1)); echo "  PASS  $2"; else nfail=$((nfail+1)); echo "  FAIL  $2"; fi; }
callees() { ( cd "$1" && "$BIN" "$1" --no-cache "--callees=$2" </dev/null 2>/dev/null | sed 's/^<!--.*-->//' ); }
has()  { case "$1" in *"$2"*) echo 1;; *) echo 0;; esac; }
hasnt(){ case "$1" in *"$2"*) echo 0;; *) echo 1;; esac; }

CPP="$FIX/cpp"; RS="$FIX/rs"
for f in viaAlias viaTypedef viaChain; do
  o="$( callees "$CPP" "filter.h::ActiveStreamDecoderFilter::$f" )"
  say "$( has "$o" 'n="decodeHeaders"' )" "P1 $f: an iface <dc> row for decodeHeaders"
  case "$o" in *'<dc n="decodeHeaders"'*'cands="2" via="declined" gate="iface"'*) say 1 "P1 $f: cands=2 gate=iface";; *) say 0 "P1 $f: cands=2 gate=iface";; esac
  say "$( hasnt "$o" 'via="name"' )" "P1 $f: no via=name guess"
done
o="$( callees "$CPP" "filter.h::ActiveStreamDecoderFilter::viaConcrete" )"
say "$( has "$o" '<s t="method" n="Codec::encodeFrame" p="filter.h:19"/>' )" "N1 alias of a concrete class: a typed (not by-name) edge to Codec::encodeFrame"
say "$( hasnt "$o" 'Other::encodeFrame' )" "N1 the namesake Other::encodeFrame is gone"
o="$( callees "$CPP" "filter.h::ActiveStreamDecoderFilter::viaDot" )"
say "$( hasnt "$o" 'gate="iface"' )" "N4 '.' on the alias member (handle_.reset(): the smart pointer's own) never types it as the pointee (no iface row)"

for f in eval_call eval_boxed; do
  o="$( callees "$RS" "src/lib.rs:$f" )"
  case "$o" in *'<dc n="run"'*'cands="2" via="declined" gate="iface"'*) say 1 "P2 $f: d.run declines gate=iface cands=2";; *) say 0 "P2 $f: d.run declines gate=iface cands=2";; esac
  say "$( hasnt "$o" 'CallEval::run' )" "N2 $f: the inherent namesake CallEval::run is absent"
  say "$( hasnt "$o" 'via="name"' )" "P2 $f: no via=name guess"
done
o="$( callees "$RS" "src/lib.rs:eval_unknown" )"
say "$( hasnt "$o" 'gate="iface"' )" "N3 a by-name callee types nothing (no iface row)"
say "$( has "$o" 'n="Ls::run"' )" "N3 base by-name rows unchanged"
o="$( callees "$CPP" "filter.h:nameLen" )"
say "$( has "$o" '<s t="method" n="string::size" p="filter.h:22" via="name"/>' )" "N5 an alias of shared_ptr<std::string> types nothing: the in-tree namesake text::string stays a by-name row (base reading)"
o="$( callees "$RS" "src/lib.rs:eval_generic" )"
say "$( hasnt "$o" 'gate="iface"' )" "N6 a generic -> R result is not read as a trait object"
echo "ifacepeelcheck: $npass passed, $nfail failed"
[ "$nfail" = 0 ]
