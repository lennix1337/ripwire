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
#   N3 [G] a map literal value is no field slot;  N4 [G] two same-named registrars storing different fields decide nothing
#   N5 [G] a same-named struct in another package: batcheval.Command's slot_fields= never lists cobra.Command's RunE
#   F1 [P] --for: slot_fields= on the struct row (EvalRW=2: the `c.EvalRW =` store has no type, so it is not this struct's), slot_by= on the stored function, the clause rides;  F2 [G] the A/B handle
#   L1 [P] the compact legend defines cands= when it rides;  J1 [P] the JSON twin carries cands
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
ok(){ printf '  PASS  %s\n' "$*"; }
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
hasnt "N4 two same-named registrars storing different fields decide nothing" "$o" 'called_by='
o="$( run '--for=how does evaluateCommand run the Command EvalRW of Put' )"
row(){ printf '%s' "$1" | grep -o "<d [^>]*n=\"$2\" p=\"$3\"[^>]*>" | head -1; }   # row ANSWER NAME PATH: that row's head
has   "F1a slot_fields= on batcheval.Command" "$( row "$o" Command batcheval/command.go )" 'slot_fields="DeclareKeys=1,EvalRW=2@evaluateCommand"'
hasnt "N5 batcheval.Command never lists cobra.Command's field" "$( row "$o" Command batcheval/command.go )" 'RunE'
has   "F1b slot_by= on the stored function" "$( row "$o" Put batcheval/cmd_put.go )" 'slot_by="evaluateCommand"'
has   "F1c the clause rides beside the attributes" "$o" 'd slot_by= functions that MAY call this one'
o="$( RIPWIRE_NO_SLOT_JOIN_ROWS=1 run '--for=how does evaluateCommand run the Command EvalRW of Put' )"
hasnt "F2 the A/B handle turns the rows and the clause off" "$o" 'slot_by'
o="$( run --callers=Put --json )"
has   "J1 the JSON twin carries cands" "$o" '"cands":"3"'
if [ "$fail" -eq 0 ]; then echo "fnslotcheck: PASS"; else echo "fnslotcheck: FAIL"; fi
exit "$fail"
