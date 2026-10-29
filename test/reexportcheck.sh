#!/usr/bin/env bash
# reexportcheck.sh — ALIAS-IMPORT (lane alias-import): a --for answer about a definition the task names carries the
# re-export statements that make it reachable under another module path (src/forreexport.h renderForReexports), from the
# TS/JS ReExport records the binds channel writes (src/ingest_importcap.h webReexportBindOf) and the Rust `use` records.
#
# Before the lane the answer printed the definition and stopped: no `pub use` / `export … from` statement was a row.
#
# Fixture test/reexportfix (a Cargo workspace of five crates + a TS tree):
#   R1  `where Boom used in main.rs is defined`: core_x/src/errors/mod.rs:2 `pub use shell::Boom` (item), core_x/src/lib.rs:4
#       `pub use errors::*` (glob="1"), engine/src/prelude.rs:1 `pub use core_x::{Boom, Other}` (a crate by its Cargo name
#       core-x), facade/src/lib.rs:1 `pub use core_x::errors` (a MODULE re-export), u app/src/main.rs:1 (the task-named file).
#   R2  NEAR MISSES: engine/src/alias.rs `pub use core_x::Boom as Bang` is a RENAME (#445): never a row, counted as_skipped="1";
#       engine/src/private.rs `use core_x::Boom` is not `pub`: never a row; other/src/lib.rs's same-named Boom has no re-export:
#       no def row of its own and none of core_x's rows.
#   T1  `where sayHello used in use.ts is defined`: ts/m/index.ts:1 `export { sayHello } from './impl'`, ts/alias.ts:1 as="sayHi",
#       ts/barrel.ts:1 `export * from './m'` (glob="1"), u ts/use.ts:1.
#   T2  `export * as ns from './impl'` (ts/m/index.ts:3) is a namespace: never a row; `export type { Shape }` IS a row (it moves a
#       name) and never a call edge: --callers=sayHello lists no ts/m/index.ts row.
#   C1  cycles terminate: core_x a.rs/b.rs glob each other; ts cyc1.ts/cyc2.ts star each other (one row each).
#   N2  a task spelling `(…)` / `[…]` around file names terminates (the tokenizer once spun on a `)` it never skipped).
#   N3  no cue word and no task-named file on the chain: no block; a task-named directory (app_main) alone suffices.
#   P1  `X from <module>` keeps only definitions under that package's directory (`from @x/ts`: ts/…, never core_x/…).
#   N1  a task with no code-shaped name prints no block; --json prints none; RIPWIRE_NO_REEXPORT_HOPS=1 removes exactly the block.
#
# Usage: bash test/reexportcheck.sh [BIN]     (default build/ripwire; exits non-zero on any failure)
set -u
ROOT="$( cd "$( dirname "$0" )/.." && pwd )"
. "$ROOT/test/lib/clean-env.sh"
BIN="${1:-${RIPWIRE_BIN:-$ROOT/build/ripwire}}"
[ "${BIN#/}" = "$BIN" ] && BIN="$ROOT/$BIN"
FIX="$ROOT/test/reexportfix"
TMP="$( mktemp -d )"; trap 'rm -rf "$TMP"' EXIT
fail=0
ok(){ printf '  PASS  %s\n' "$*" || { fail=1; printf '  FAIL  could not write the PASS line for: %s\n' "$*"; }; return 0; }
no(){ printf '  FAIL  %s\n' "$*"; fail=1; }

[ -x "$BIN" ] || { echo "FAIL: no binary at $BIN"; exit 1; }
echo "reexportcheck: $( "$BIN" --version 2>/dev/null )"

ask(){ ( cd "$FIX" || exit 1; HOME="$TMP" "$BIN" "$FIX" --no-cache "$@" </dev/null 2>/dev/null ); }
block(){ grep -o '<reexports>.*</reexports>' "$1"; }
has(){ local f="$1" what="$2" pat="$3"; if block "$f" | grep -qF -- "$pat"; then ok "$what"; else no "$what (missing: $pat)"; fi; }
hasnt(){ local f="$1" what="$2" pat="$3"; if block "$f" | grep -qF -- "$pat"; then no "$what (present: $pat)"; else ok "$what"; fi; }

ask "--for=where Boom used in main.rs is defined" >"$TMP/r1.xml"
has  "$TMP/r1.xml" "R1 def row core_x Boom"            '<def n="Boom" p="core_x/src/errors/shell.rs" l="1" at="core_x::errors::shell::Boom"'
has  "$TMP/r1.xml" "R1 item re-export errors/mod.rs:2"  '<rx p="core_x/src/errors/mod.rs" l="2" use="shell::Boom" at="core_x::errors::Boom"/>'
has  "$TMP/r1.xml" "R1 glob hop lib.rs:4 hedged"        '<rx p="core_x/src/lib.rs" l="4" use="errors::*" at="core_x::Boom" glob="1"/>'
has  "$TMP/r1.xml" "R1 cross-crate prelude.rs:1"        '<rx p="engine/src/prelude.rs" l="1" use="core_x::Boom" at="engine::prelude::Boom"/>'
has  "$TMP/r1.xml" "R1 module re-export facade"         '<rx p="facade/src/lib.rs" l="1" use="core_x::errors"'
n=$( block "$TMP/r1.xml" | grep -o '<rx p="facade/src/lib.rs"' | wc -l | tr -d ' ' )
if [ "$n" = 1 ]; then ok "R1 one statement, one row (the module re-export publishes two spellings)"; else no "R1 facade/src/lib.rs printed $n times"; fi
has  "$TMP/r1.xml" "R1 task-named use site main.rs:1"   '<u p="app/src/main.rs" l="1" use="engine::prelude::*" glob="1"/>'
has  "$TMP/r1.xml" "R2 rename counted (#445)"           'as_skipped="1"'
hasnt "$TMP/r1.xml" "R2 rename not a row"               'engine/src/alias.rs'
hasnt "$TMP/r1.xml" "R2 private use not a row"          'engine/src/private.rs'
hasnt "$TMP/r1.xml" "R2 same-named Boom keeps identity" 'other/src/lib.rs'

ask "--for=where sayHello used in use.ts is defined" >"$TMP/t1.xml"
has  "$TMP/t1.xml" "T1 barrel export-from index.ts:1"   '<rx p="ts/m/index.ts" l="1" from="./impl"/>'
has  "$TMP/t1.xml" "T1 renamed export-from as="         '<rx p="ts/alias.ts" l="1" from="./m/index" as="sayHi"/>'
has  "$TMP/t1.xml" "T1 star hop hedged"                 '<rx p="ts/barrel.ts" l="1" from="./m" glob="1"/>'
has  "$TMP/t1.xml" "T1 task-named import use.ts:1"      '<u p="ts/use.ts" l="1" from="./barrel"/>'
hasnt "$TMP/t1.xml" "T2 namespace export not a row"     'p="ts/m/index.ts" l="3"'
ask "--for=where Shape is defined" >"$TMP/t2.xml"
has  "$TMP/t2.xml" "T2 type-only re-export is a row"    '<rx p="ts/m/index.ts" l="2" from="./impl"/>'
ask --callers=sayHello >"$TMP/t2c.xml"
if grep -q 'ts/m/index.ts' "$TMP/t2c.xml"; then no "T2 a re-export is never a caller row"; else ok "T2 a re-export is never a caller row"; fi

ask "--for=where cycFn is defined" >"$TMP/c1.xml"; rc1=$?
ask "--for=where loopyFn is defined" >"$TMP/c2.xml"; rc2=$?
if [ "$rc1" = 0 ] && [ "$rc2" = 0 ]; then ok "C1 cyclic globs terminate (rc 0)"; else no "C1 rc $rc1 / $rc2"; fi
has  "$TMP/c1.xml" "C1 Rust glob cycle one row"         'total="1"'
has  "$TMP/c2.xml" "C1 TS star cycle one row"           'total="1"'

# N2: a task-file token ending at a break the skip loop did not eat (`)`, `]`) once spun forever (S1 kysely-02 timed out at 900 s)
( cd "$FIX" && HOME="$TMP" perl -e 'my $p = fork; if( !$p ) { open STDIN, "<", "/dev/null"; open STDOUT, ">", shift @ARGV; open STDERR, ">", "/dev/null"; exec @ARGV; exit 127 } $SIG{ALRM} = sub { kill 9, $p; waitpid( $p, 0 ); exit 124 }; alarm 120; waitpid( $p, 0 ); exit( $? >> 8 )' "$TMP/n2.xml" "$BIN" "$FIX" --no-cache "--for=where Boom (in main.rs) [see lib.rs] is defined)." )   # the child is KILLED at 120 s, never orphaned
n2rc=$?
if [ "$n2rc" = 0 ] && grep -q '<reexports>' "$TMP/n2.xml"; then ok "N2 a task with ) and ] terminates"; else no "N2 a task with ) and ] did not finish in 120 s (or printed no block)"; fi
# N3 (the WHEN gate): a code-shaped name with re-exports, but a task that does not ask where it comes from and names no file
# on its chain, prints nothing; the same name with only a task-named directory on the chain (no cue word) prints the block
ask "--for=how does Boom work at runtime" >"$TMP/n3.xml"
if grep -q '<reexports>' "$TMP/n3.xml"; then no "N3 uncued task, no task-named file: no block"; else ok "N3 uncued task, no task-named file: no block"; fi
ask "--for=Boom in the app_main example" >"$TMP/n4.xml"
has  "$TMP/n4.xml" "N3 uncued task whose chain reaches a task-named directory" '<u p="app_main/src/main.rs" l="1"'
ask "--for=where Shape from @fix/m is defined" >"$TMP/p1.xml"
has  "$TMP/p1.xml" "P1 a from-module keeps the def in its package"   '<def n="Shape" p="ts/m/impl.ts"'
ask "--for=where Boom from @fix/ts is defined" >"$TMP/p2.xml"
if grep -q '<reexports>' "$TMP/p2.xml"; then no "P1 a def outside the named package prints nothing"; else ok "P1 a def outside the named package prints nothing"; fi
ask "--for=add a new built-in filter command" >"$TMP/n1.xml"
if grep -q '<reexports>' "$TMP/n1.xml"; then no "N1 no code-shaped name: no block"; else ok "N1 no code-shaped name: no block"; fi
ask --json "--for=where Boom used in main.rs is defined" >"$TMP/n2.json"
if grep -q 'reexports' "$TMP/n2.json"; then no "N1 --json carries no block"; else ok "N1 --json carries no block"; fi
( cd "$FIX" || exit 1; RIPWIRE_NO_REEXPORT_HOPS=1 HOME="$TMP" "$BIN" "$FIX" --no-cache "--for=where Boom used in main.rs is defined" </dev/null 2>/dev/null ) >"$TMP/off.xml"
sed 's|<reexports>.*</reexports>||' "$TMP/r1.xml" >"$TMP/r1.stripped"
if cmp -s "$TMP/off.xml" "$TMP/r1.stripped"; then ok "N1 the off switch removes exactly the block"; else no "N1 the off switch changed more than the block"; fi
if command -v xmllint >/dev/null 2>&1; then
    for f in r1 t1 t2 c1; do if xmllint --noout "$TMP/$f.xml" 2>/dev/null; then ok "xml well-formed $f"; else no "xml malformed $f"; fi; done
fi

[ "$fail" = 0 ] && { echo "reexportcheck: PASS"; exit 0; }
echo "reexportcheck: FAIL"; exit 1
