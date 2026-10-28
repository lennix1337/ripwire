#!/usr/bin/env bash
# fnslotcheck.sh — IX1-V2 (lane fn-slot): FN-VALUED SLOTS in Go — a function stored in a struct FIELD (a keyed composite-literal
# element, a field assignment, a parameter a registrar stores there) and the member call `x.F( … )` through that field; a method
# value of the method's own receiver stored in a variable. Rows: --callers <vr called_by= called_at= via=slot-name cands=>,
# --callees <vs through= slot= targets=>, --for slot_by= / slot_fields= on shown rows. Never a proven call: count= never moves.
#
# Arms tagged [P] were RED on the pre-change binary (train-39 3d03da53b); [G] arms are twins and near-miss negatives that hold on
# both binaries and must keep holding (PROCESS rule 4).
#   A1 [P] a registrar parameter: `RegisterReadWriteCommand( 1, declareKeysPut, Put )` stores Put via `Command{ EvalRW: impl }`
#   A2 [P] a direct keyed element `Command{ EvalRW: Direct }`;  A3 [P] a field assignment `c.EvalRW = Assigned`
#   A4 [P] the callees side lists the slot call and its 3 targets;  A5 [G] count= stays 0 (never a proven call)
#   B1 [P] a method value `f = b.opPlan` of the receiver, called through f in a closure: called_by=Operation
#   N1 [G] `f = other.opPlan` (a parameter, not the receiver) makes no row for Remote.opPlan
#   N2 [G] a field named like a Go METHOD (`h.Close()` beside `(*File).Close`) is no slot call
#   N3 [G] a map literal value is no field slot
#   N4 [G] two same-named registrars in two packages: the bare call `Reg( ambiguous )` binds its OWN package's registrar (fix1 F-1:
#          by-design change — this arm said "decide nothing" while registrars were keyed by the bare name; it now pins the
#          identity, and N4c below keeps the "decide nothing" premise on the shape that still has it)
#   N5 [G] a same-named struct in another package: batcheval.Command's slot_fields= never lists cobra.Command's RunE
#   F1 [P] --for: slot_fields= on the struct row (EvalRW=2: the `c.EvalRW =` store has no type, so it is not this struct's), slot_by= on the stored function, the clause rides;  F2 [G] the A/B handle
#   L1 [P] the compact legend defines cands= when it rides;  J1 [P] the JSON twin carries cands
# fix1 (review rv-fn-slot F-1..F-7; fixture test/fnslotfix2/, its own root so the arms above keep their rows):
#   N6 [P] a same-named function of another package that stores nothing (`other.Enlist( otherFn )`) never feeds the slot (F-1)
#   N6b [G] the twin: the registrar's own package's bare call `Enlist( realFn )` still lands in Task.Do
#   N7 [P] a METHOD registrar `(*Router).Mount` never feeds a bare call `Mount( zero, one )` of another package (F-1)
#   N4c [G] one registrar storing its parameter in two fields (`H{A: fn}`, `H{B: fn}`) decides nothing
#   K1 [P] x/types.Cmd and y/types.Cmd are two slots: K=1 on each struct row (F-2; the call side is name-only, so `@~`)
#   K2 [G] a QUALIFIED literal `enlist.Task{Do: litFn}` keys by the import's directory: Task Do=2 (realFn, litFn)
#   X1 [P] a Go member call never targets a C function-pointer field of the same name (F-3); X1b [P] nor does the C row count it
#   M1 [P] --for marks a field-name-only caller `~` (slot_by="~useA" on fb, A.F vs B.F) and the clause rides (F-4)
#   T1 [P] a named MAP type's keyed literal `Table{OpPut: putTab}` is no field slot (F-5); T1b [G] a struct's still is
#   N3b [G] an unnamed map literal `map[Op]func(){OpGet: getMap}` is no field slot (F-6; pins goValueSlot's ty.empty() guard)
#   Q1 [P] a slot_by= whose only store is in test code (`assertMatch( t, MatchV1 )`) is not printed on a non-test row (F-7)
#
# Usage: bash test/fnslotcheck.sh [BIN]   (BIN defaults to RIPWIRE_BIN, then build/ripwire)
set -u
ROOT="$( cd "$( dirname "$0" )/.." && pwd )"
BIN="${1:-${RIPWIRE_BIN:-$ROOT/build/ripwire}}"
[ "${BIN#/}" = "$BIN" ] && BIN="$ROOT/$BIN"
echo "fnslotcheck: BIN=$BIN ($( "$BIN" --version 2>/dev/null | grep -o 'built_from=[^)]*' ))"
[ -x "$BIN" ] || { echo "  FAIL  no ripwire binary at $BIN"; exit 2; }
FIX="$ROOT/test/fnslotfix"
[ -d "$FIX" ] || { echo "  FAIL  fixture dir missing: $FIX"; exit 2; }
fail=0
ok(){ printf '  PASS  %s\n' "$*" || { fail=1; printf '  FAIL  could not write the PASS line for: %s\n' "$*"; }; return 0; }
no(){ printf '  FAIL  %s\n' "$*"; fail=1; }
run(){
    local out rc
    out="$( cd "$FIX" && "$BIN" "$FIX" "$@" --no-cache 2>/dev/null )"; rc=$?
    if [ "$rc" -ne 0 ] || ! printf '%s' "$out" | grep -q -e '<callers ' -e '<callees ' -e '<ctx ' -e '"of":'; then
        printf 'RUNFAIL rc=%s' "$rc"; return 0
    fi
    printf '%s' "$out"
}
has(){ case "$2" in RUNFAIL*) no "$1 (the run failed: $2)"; return;; esac
       if printf '%s' "$2" | grep -qF -- "$3"; then ok "$1"; else no "$1 — missing: $3"; fi; }
hasnt(){ case "$2" in RUNFAIL*) no "$1 (the run failed: $2)"; return;; esac
         if printf '%s' "$2" | grep -qF -- "$3"; then no "$1 — unexpected: $3"; else ok "$1"; fi; }

o="$( run --callers=Put )"
has   "A1 a registrar parameter lands Put in Command.EvalRW, joined to the member call" "$o" 'into="RegisterReadWriteCommand#arg2" called_by="evaluateCommand" called_at="batcheval/evaluate.go:8" via="slot-name" cands="3"'
has   "A5 never a proven call: count stays 0" "$o" 'count="0"'
has   "L1 the compact legend defines cands= beside it" "$o" 'vr cands=K:'
o="$( run --callers=Direct )"
has   "A2 a keyed composite-literal element" "$o" 'into="direct.EvalRW" called_by="evaluateCommand"'
o="$( run --callers=Assigned )"
has   "A3 a field assignment" "$o" 'into="c.EvalRW" called_by="evaluateCommand"'
o="$( run --callees=evaluateCommand )"
has   "A4 the callees side: the slot call and its targets" "$o" '<vs l="8" through="cmd.EvalRW" slot=".EvalRW" via="slot-name" targets="3" shown="3"><t n="Assigned" p="batcheval/cmd_put.go:13"/><t n="Direct" p="batcheval/cmd_put.go:9"/><t n="Put" p="batcheval/cmd_put.go:7"/></vs>'
o="$( run --callers=local/backend.go:Local.opPlan )"
has   "B1 a method value of the receiver, called through the local in a closure" "$o" 'into="f" called_by="Operation"'
hasnt "N1b and no row binds it to the receiver's type either" "$o" 'bind="local/backend.go:19"'
o="$( run --callers=Remote.opPlan )"
hasnt "N1 a method value of a non-receiver parameter makes no row" "$o" '<vr '
o="$( run --callers=closeHook )"
has   "N2a the stored value still prints" "$o" 'into="hooks.Close"'
hasnt "N2 a field named like a Go method is no slot call" "$o" 'called_by='
o="$( run --callers=doneHook )"
hasnt "N3 a map literal value is no field slot" "$o" 'via="slot'
o="$( run --callers=ambiguous )"
has   "N4a the argument row prints" "$o" 'into="Reg#arg0"'
has   "N4 the bare call binds its own package's registrar (misc2.Reg → Hooks2.OnFail)" "$o" 'called_by="fire"'
hasnt "N4b never the other package's registrar's field" "$o" 'useHooks'
o="$( run '--for=how does evaluateCommand run the Command EvalRW of Put' )"
row(){ printf '%s' "$1" | grep -o "<d [^>]*n=\"$2\" p=\"$3\"[^>]*>" | head -1; }   # row ANSWER NAME PATH: that row's head
has   "F1a slot_fields= on batcheval.Command (the caller joins by the field name: @~)" "$( row "$o" Command batcheval/command.go )" 'slot_fields="DeclareKeys=1,EvalRW=2@~evaluateCommand"'
hasnt "N5 batcheval.Command never lists cobra.Command's field" "$( row "$o" Command batcheval/command.go )" 'RunE'
has   "F1b slot_by= on the stored function (field-name join: ~)" "$( row "$o" Put batcheval/cmd_put.go )" 'slot_by="~evaluateCommand"'
has   "F1c the clause rides beside the attributes" "$o" 'd slot_by= functions that MAY call this one'
o="$( RIPWIRE_NO_SLOT_JOIN_ROWS=1 run '--for=how does evaluateCommand run the Command EvalRW of Put' )"
hasnt "F2 the A/B handle turns the rows and the clause off" "$o" 'slot_by'
o="$( run --callers=Put --json )"
has   "J1 the JSON twin carries cands" "$o" '"cands":"3"'

FIX="$ROOT/test/fnslotfix2"
[ -d "$FIX" ] || { echo "  FAIL  fixture dir missing: $FIX"; exit 2; }
o="$( run --callers=otherFn )"
has   "N6a the argument row prints" "$o" 'into="Enlist#arg0"'
hasnt "N6 a same-named function of another package that stores nothing never feeds the slot" "$o" 'called_by='
o="$( run --callers=realFn )"
has   "N6b the registrar's own package's bare call still lands in Task.Do" "$o" 'into="Enlist#arg0" called_by="execTasks"'
o="$( run --callers=one )"
has   "N7a the argument row prints" "$o" 'into="Mount#arg1"'
hasnt "N7 a METHOD registrar never feeds a bare call of another package" "$o" 'called_by='
o="$( run --callers=amb )"
has   "N4c-a the argument row prints" "$o" 'into="Hold#arg0"'
hasnt "N4c one registrar storing its parameter in two fields decides nothing" "$o" 'called_by='
o="$( run '--for=how does execX run the Cmd Run of runX and runY' )"
has   "K1 x/types.Cmd holds one function" "$( row "$o" Cmd x/types/t.go )" 'slot_fields="Run=1@~execX"'
has   "K1b y/types.Cmd holds one function" "$( row "$o" Cmd y/types/t.go )" 'slot_fields="Run=1@'
o="$( run '--for=how does execTasks run the Task Do of realFn and litFn' )"
has   "K2 a qualified literal keys by its import's directory" "$( row "$o" Task enlist/reg.go )" 'slot_fields="Do=2@~execTasks"'
o="$( run --callees=goCall )"
has   "X1a the Go slot call prints" "$o" 'through="x.draw"'
hasnt "X1 a Go member call never targets a C function-pointer field" "$o" 'n="cdraw"'
o="$( run --callers=cdraw )"
has   "X1c the C row keeps its own typed caller" "$o" 'called_by="cfire" called_at="cx/ops.c:4" via="slot"'
hasnt "X1b the C row counts no Go caller" "$o" 'slot_name_also='
o="$( run '--for=how does useA call fa fb through field F of A and B' )"
has   "M1 a field-name-only caller is marked ~" "$( row "$o" fb p/p.go )" 'slot_by="~useA"'
has   "M1b the mark's clause rides" "$o" 'C matches the field name only'
o="$( run --callers=putTab )"
has   "T1a the stored value prints" "$o" 'into="t.OpPut"'
hasnt "T1 a named map type's keyed literal is no field slot" "$o" 'called_by='
o="$( run --callers=opsFn )"
has   "T1b a struct's keyed literal still is" "$o" 'into="o.OpPut" called_by="callOps"'
o="$( run --callers=getMap )"
has   "N3b-a the stored value prints" "$o" 'into="t.OpGet"'
hasnt "N3b an unnamed map literal is no field slot" "$o" 'called_by='
o="$( run '--for=how is MatchV1 scored and matched' )"
has   "Q1a the MatchV1 row is shown" "$o" 'n="MatchV1" p="fzt/algo.go"'
hasnt "Q1 a test-only store prints no slot_by=" "$( row "$o" MatchV1 fzt/algo.go )" 'slot_by='
# CR 4396503857 (train 40): a Go directory whose FIRST file is no .go file (enlist/README.md sorts before reg.go) keys its types all the same
if [ -f "$FIX/enlist/README.md" ]; then ok "K3a the fixture directory is led by a non-Go file"; else no "K3a enlist/README.md is missing: K3 would prove nothing"; fi
o="$( run '--for=how does execTasks run the Task Do of realFn and litFn' )"
has   "K3 a package directory led by README.md still keys enlist.Task: the qualified literal lands (Do=2, not the unknown-type drop)" "$( row "$o" Task enlist/reg.go )" 'slot_fields="Do=2@~execTasks"'
# CR 4396503829 (train 40): a caller whose NAME holds a `~` (C++ operator~) is not a field-name-only caller: no ~ clause rides
o="$( run '--for=how does tfire call tdrawImpl through tdraw' )"
has   "T2a the operator~ caller is printed" "$( row "$o" tdrawImpl tilde/t.cpp )" 'operator~'
hasnt "T2 a typed caller named operator~ never raises the field-name-only clause" "$o" 'matches the field name only'
hasnt "T2b nor the long form of it" "$o" 'a ~ before a slot_by= name'
if [ "$fail" -eq 0 ]; then echo "fnslotcheck: PASS"; else echo "fnslotcheck: FAIL"; fi
exit "$fail"
