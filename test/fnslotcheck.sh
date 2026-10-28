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
# part 2 (lane fn-slot-2, Rust; fixture test/fnslotrust/, its own root; model.h kVrSlotLangs Rust row: ParenFieldCall, ModuleType):
#   R1 [P] a registrar path call `CallEval::new( 1, eval_block )` → `Self { stack, eval }` → `(self.eval)( … )` in CallEval::run:
#          called_by="run" via="slot" (typed: `Self` is the impl type on both sides), cands=4
#   R2 [P] a turbofish value `eval_block_generic::<u8>` is the function;  R3 [P] a struct expression `CallEval { eval: direct_eval }`
#   R4 [P] `Box::new( hook_target )` in `Hooks { eval: … }` lands in Hooks.eval (no Box::new argument row);  R4b [G] no row into Box::new
#   R5 [P] --callees: `(self.eval)( … )` prints the typed slot and its 4 targets
#   R6 [P] a path call to a METHOD `Reg::set( r, path_reg_target )` (self = position 0) feeds `self.h = h`, called in Reg::go
#   N8 [G] a local closure of a stored name (`let shadowed = |x| …; CallEval::new( 3, shadowed )`) is no function value: no row
#   N9 [P] a same-named struct of ANOTHER crate (crates/twin CallEval) is another slot: eval_block's cands stays 4, twin_run targets 1
#   N10 [G] (N10a [P]) an enum variant `Msg::Fire { eval: variant_target }` is no struct field slot: no called_by=
#   N11 [G] a method-call registrar `r.set( method_reg_target )` is a floor: no row
#   N12 [P] `x.cb = loose_target` and `(y.cb)( … )` (types not read) join by the field name only: via="slot-name", --for `@~`
#   N13 [P] a store in another file of the crate keys the crate's own CallEval (crate tier), typed: via="slot"
#   R7 [G] a registrar's owner type decides it: `Hooks::new` (stores into Hooks.eval) never feeds CallEval.eval, nor the reverse
#   X2 [G] a Go field of the same name (gox Box.eval) never joins a Rust slot (family guard), nor the reverse
#   F3 [P] --for: slot_fields= per struct (eval=4@CallEval::run, the twin's eval=1@CallEval::twin_run), slot_by= on eval_block
# fix round 1 (review rv-fn-slot-2 F-1..F-4; fixture test/fnslotrust2/, its own root: crates app, lib, qual):
#   E1 [P] `use extcrate::Hook; Hook { cb: f }` is an out-of-tree struct: never the in-tree Hook.cb (F-1)
#   E2 [P] `use tokio::spawn; spawn( f )` is an out-of-tree registrar: never the in-tree `spawn` that stores into Hook.cb (F-1)
#   E3 [P] `std::thread::spawn( f )` is std's: never the in-tree `spawn` (F-1);  E4 [G] the in-tree Hook keeps its own store
#   Q1 [P] `crate::a::Cfg { cb: f }` written in c.rs (which declares its own Cfg) is a.rs's Cfg: called_by="go" (F-2)
#   Q1b [G] the twin: c.rs's bare `Cfg { cb: f }` is its own Cfg (called_by="local_go")
#   Q2 [P] `crate::other::register( f )` beside a local storing `register` is other.rs's (stores nothing): no called_by= (F-2)
#   Q3 [P] `S::new( crate::other::target )` is other.rs's target, never here.rs's namesake (F-2);  Q3b [G] the bare twin
#          `register( local_t )` still lands in S.f
#   Q4 [P] `crate::p::Amb { f }` with two Amb in the crate is p.rs's (the path names the module file): called_by="go_p", typed
#   N14 [G] a BARE `Amb { f }` with two Amb in the crate (neither in the writing file) is never typed: via="slot-name" — kills
#          the source mutant "nearestOf guesses within an ambiguous tier" (review MG1)
#   N15 [G] `(self.inner.f)( … )` is never typed as Self's field: in_t keeps its caller, out_t (Outer.f) is never via="slot" —
#          kills the source mutant "slotCallAt types any self.… chain" (review MG2)
#   W1 [P] `std::sync::Arc::new( f )` is the wrapper by its LAST path segment: the value lands in Dyn.g (F-4)
# fix round 2 (second lens rv-fn-slot-2-lens L-1, L-2; fixture crates selfmod, ga, gx):
#   S1 [P] `use crate::workspace_settings_module::{self, Other}` (module path past SSO) binds the module: `workspace_settings_module::Ws { … }`
#          is that file's Ws, typed (L-1: the `self` branch kept a view into the string it then reassigned)
#   S2 [P] `use crate::{a::{self}}` binds `a`: `a::Sa { … }` is a.rs's Sa, never the writing file's own Sa (L-1 short module)
#   X1 [P] a bare `Gh { … }` in a file with an OUT-OF-TREE glob (`use extcrate::*`) is a guess among crate namesakes: via="slot-name",
#          never typed (L-2 tier 1)
#   X2 [P] a bare `Gw { … }` in a file that `use`s the defining crate by name AND has an out-of-tree glob: the same hedge (L-2 tier 2)
#   X3 [G] a local `Gm` shadows the glob (tier 0: typed);  X4 [G] no glob: the crate namesake stays typed;  X5 [G] an in-tree glob
#          (`use crate::hk::*`) and X6 [G] a glob led by an in-tree crate (`use ga::*`) are not out-of-tree: typed
#   X7 [G] a glob led by a `mod` the file declares, X8 [G] `use super::*`, X9 [G] `use self::Kind::*` are in-tree: typed;
#   X10 [G] tier 2 without a glob (`use ga::Gother` only) stays typed
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

FIX="$ROOT/test/fnslotrust"
[ -d "$FIX" ] || { echo "  FAIL  fixture dir missing: $FIX"; exit 2; }
o="$( run --callers=eval_block )"
has   "R1 a registrar path call lands in the typed slot its parameter is stored in" "$o" 'into="CallEval::new#arg1" called_by="run" called_at="src/engine/eval.rs:14" via="slot" cands="4"'
o="$( run --callers=eval_block_generic )"
has   "R2 a turbofish value is the function" "$o" 'into="CallEval::new#arg1" called_by="run"'
o="$( run --callers=direct_eval )"
has   "R3 a struct expression's field value" "$o" 'into="CallEval.eval" called_by="run" called_at="src/engine/eval.rs:14" via="slot"'
o="$( run --callers=hook_target )"
has   "R4 Box::new( f ) lands in the field" "$o" 'into="Hooks.eval" called_by="fire" called_at="src/other/hooks.rs:11" via="slot"'
hasnt "R4b no argument row into the Box::new wrapper" "$o" 'into="Box::new'
o="$( run --callees=CallEval::run )"
has   "R5 the (self.eval)( … ) call prints its typed slot and targets" "$o" 'through="self.eval" slot="CallEval.eval" via="slot" targets="4"'
o="$( run --callers=path_reg_target )"
has   "R6 a path call to a method counts self as position 0" "$o" 'into="Reg::set#arg1" called_by="go" called_at="crates/twin/src/lib.rs:60" via="slot"'
o="$( run --callers=shadowed )"
hasnt "N8 a local closure of a stored name is no function value" "$o" '<vr '
o="$( run --callers=twin_target )"
has   "N9 the twin crate's CallEval is its own slot" "$o" 'into="CallEval::new#arg0" called_by="twin_run" called_at="crates/twin/src/lib.rs:11" via="slot"'
hasnt "N9b the twin's value is not counted with the other crate's" "$o" 'cands='
o="$( run --callees=CallEval::twin_run )"
has   "N9c the twin's call targets its one function" "$o" 'slot="CallEval.eval" via="slot" targets="1"'
o="$( run --callers=variant_target )"
has   "N10a the variant store prints" "$o" 'into="Msg::Fire.eval"'
hasnt "N10 an enum variant field is no struct slot" "$o" 'called_by='
o="$( run --callers=method_reg_target )"
hasnt "N11 a method-call registrar is a floor" "$o" '<vr '
o="$( run --callers=loose_target )"
has   "N12 untyped receivers join by the field name only" "$o" 'into=".cb" called_by="call_loose" called_at="crates/twin/src/lib.rs:48" via="slot-name"'
o="$( run --callers=crate_tier_target )"
has   "N13 a store in another file of the crate keys the crate's struct" "$o" 'into="CallEval.eval" called_by="run" called_at="src/engine/eval.rs:14" via="slot"'
o="$( run --callees=Hooks::fire )"
has   "R7 Hooks.eval holds only its own store (Hooks::new feeds nothing else)" "$o" 'slot="Hooks.eval" via="slot" targets="1"'
o="$( run --callees=CallEval::run )"
has   "R7b CallEval.eval holds its four functions (crate tier included)" "$o" 'slot="CallEval.eval" via="slot" targets="4"'
o="$( run --callers=goTarget )"
has   "X2a the Go slot keeps its own call" "$o" 'called_by="fire" called_at="gox/g.go:11" via="slot-name"'
hasnt "X2 a Go value never counts a Rust slot's functions" "$o" 'cands='
o="$( run '--for=how CallEval runs eval_block' )"
has   "F3 slot_fields= on the crate's CallEval" "$( row "$o" CallEval src/engine/eval.rs )" 'slot_fields="eval=4@CallEval::run"'
has   "F3b slot_fields= on the twin crate's CallEval" "$( row "$o" CallEval crates/twin/src/lib.rs )" 'slot_fields="eval=1@CallEval::twin_run"'
has   "F3c slot_by= on the stored function (typed: no ~)" "$( row "$o" eval_block src/engine/eval.rs )" 'slot_by="CallEval::run"'
has   "X2b the Go struct row keeps its own name-tier caller" "$( row "$o" Box gox/g.go )" 'slot_fields="eval=1@~Box.fire"'
FIX="$ROOT/test/fnslotrust2"
[ -d "$FIX" ] || { echo "  FAIL  fixture dir missing: $FIX"; exit 2; }
o="$( run --callers=worker_ext_struct )"
has   "E1a the out-of-tree struct store still prints" "$o" 'into="Hook.cb"'
hasnt "E1 an out-of-tree struct is never the in-tree Hook" "$o" 'called_by='
o="$( run --callers=worker_bare_ext )"
has   "E2a the argument row still prints" "$o" 'into="spawn#arg0"'
hasnt "E2 a used out-of-tree registrar is never the in-tree spawn" "$o" 'called_by='
o="$( run --callers=worker_path_ext )"
hasnt "E3 std::thread::spawn is never the in-tree spawn" "$o" 'called_by='
o="$( run --callers=lib_target )"
has   "E4 the in-tree Hook keeps its own store" "$o" 'into="Hook.cb" called_by="fire" called_at="crates/lib/src/lib.rs:3" via="slot"'
o="$( run --callers=fc_qualified )"
has   "Q1 a crate:: path to a struct is the module file's struct" "$o" 'called_by="go" called_at="crates/qual/src/a.rs:3" via="slot"'
hasnt "Q1c never the writing file's own namesake" "$o" 'local_go'
o="$( run --callers=fc_local )"
has   "Q1b the bare twin is the file's own struct" "$o" 'called_by="local_go" called_at="crates/qual/src/c.rs:3" via="slot"'
o="$( run --callers=qual_t )"
has   "Q2a the argument row still prints" "$o" 'into="register#arg0"'
hasnt "Q2 a crate:: registrar path is never the local namesake" "$o" 'called_by='
o="$( run --callees=S::go )"
has   "Q3 a crate:: value path is the module file's function" "$o" '<t n="target" p="crates/qual/src/other.rs:1"/>'
hasnt "Q3c never the writing file's own namesake" "$o" '<t n="target" p="crates/qual/src/here.rs:3"/>'
has   "Q3b the bare registrar twin still lands in S.f" "$o" '<t n="local_t" p="crates/qual/src/here.rs:5"/>'
o="$( run --callers=amb_t )"
has   "Q4 crate::p::Amb is p.rs's Amb, typed" "$o" 'into="Amb.f" called_by="go_p" called_at="crates/qual/src/p.rs:2" via="slot"'
o="$( run --callers=amb_bare_t )"
has   "N14a the bare store joins by the field name" "$o" 'via="slot-name"'
hasnt "N14 an ambiguous crate tier is never typed" "$o" 'via="slot"'
o="$( run --callers=in_t )"
has   "N15a (self.inner.f)( … ) keeps Inner.f's caller" "$o" 'into="Inner.f" called_by="go_inner"'
o="$( run --callers=out_t )"
hasnt "N15 (self.inner.f)( … ) is never typed as Self's field" "$o" 'via="slot"'
o="$( run --callers=arc_t )"
has   "W1 std::sync::Arc::new( f ) hands the value to the field" "$o" 'into="Dyn.g" called_by="go" called_at="crates/qual/src/wrap.rs:2" via="slot"'
hasnt "W1b no argument row into the Arc::new wrapper" "$o" 'into="Arc::new'
o="$( run --callers=self_long_t )"
has   "S1 {self} of a long module path binds the module" "$o" 'into="Ws.scb" called_by="fire_ws" called_at="crates/selfmod/src/workspace_settings_module.rs:2" via="slot"'
hasnt "S1b never the writing file's own Ws" "$o" 'fire_ws_local'
o="$( run --callers=self_short_t )"
has   "S2 {self} of a short module path binds the module" "$o" 'into="Sa.acb" called_by="fire_sa" called_at="crates/selfmod/src/a.rs:2" via="slot"'
hasnt "S2b never the writing file's own Sa" "$o" 'fire_sa_local'
o="$( run --callers=glob_t1 )"
has   "X1a the store still prints, joined by the field name" "$o" 'into="Gh.hcb" called_by="fire_gh"'
has   "X1 an out-of-tree glob hedges the crate tier: slot-name" "$o" 'via="slot-name"'
hasnt "X1b never typed" "$o" 'via="slot"'
o="$( run --callers=glob_t2 )"
has   "X2a the store still prints, joined by the field name" "$o" 'into="Gw.wcb" called_by="fire_ga"'
has   "X2 an out-of-tree glob hedges the used-crate tier: slot-name" "$o" 'via="slot-name"'
hasnt "X2b never typed" "$o" 'via="slot"'
o="$( run --callers=glob_loc_t )"
has   "X3 a local struct shadows the glob: typed" "$o" 'into="Gm.mcb" called_by="fire_gm_local" called_at="crates/gx/src/g3.rs:3" via="slot"'
o="$( run --callers=plain_t )"
has   "X4 no glob: the crate namesake stays typed" "$o" 'into="Gh.hcb" called_by="fire_gh" called_at="crates/gx/src/hk.rs:2" via="slot"'
o="$( run --callers=intree_glob_t )"
has   "X5 an in-tree glob is no out-of-tree glob: typed" "$o" 'into="Gh.hcb" called_by="fire_gh" called_at="crates/gx/src/hk.rs:2" via="slot"'
o="$( run --callers=crate_glob_t )"
has   "X6 a glob led by an in-tree crate: typed" "$o" 'into="Gw.wcb" called_by="fire_ga" called_at="crates/ga/src/lib.rs:2" via="slot"'
o="$( run --callers=mod_glob_t )"
has   "X7 a glob led by a mod the file declares: typed" "$o" 'into="Gh.hcb" called_by="fire_gh" called_at="crates/gx/src/hk.rs:2" via="slot"'
o="$( run --callers=super_glob_t )"
has   "X8 use super::*: typed" "$o" 'into="Gh.hcb" called_by="fire_gh" called_at="crates/gx/src/hk.rs:2" via="slot"'
o="$( run --callers=self_glob_t )"
has   "X9 use self::Kind::*: typed" "$o" 'into="Gh.hcb" called_by="fire_gh" called_at="crates/gx/src/hk.rs:2" via="slot"'
o="$( run --callers=used_crate_t )"
has   "X10 tier 2 without a glob stays typed" "$o" 'into="Gw.wcb" called_by="fire_ga" called_at="crates/ga/src/lib.rs:2" via="slot"'
if [ "$fail" -eq 0 ]; then echo "fnslotcheck: PASS"; else echo "fnslotcheck: FAIL"; fi
exit "$fail"
