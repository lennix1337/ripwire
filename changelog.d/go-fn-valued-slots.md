### Added — Go functions stored in struct fields are joined to the calls through those fields (`--callers`, `--callees`, `--for`)

A Go function stored in a struct field — a keyed composite-literal element (`Command{ EvalRW: Put }`), a field assignment
(`c.EvalRW = Put`), or an argument of a registrar that stores its parameter there (`RegisterReadWriteCommand( kvpb.Put,
declareKeysPut, Put )` with `Command{ EvalRW: impl }` in its body) — is now joined to every member call through a field of that
name (`cmd.EvalRW( … )`): `--callers=Put` gains `called_by= called_at= via="slot-name" cands=K` on its `<vr>` row (K = the
distinct functions that slot holds), `--callees` of the dispatcher lists `<vs through="cmd.EvalRW" slot=".EvalRW" targets=K>`, and
a `--for` answer marks a shown function row with `slot_by=` (the functions that may call it through a slot) and a shown struct
row with `slot_fields="F=K@C"`. A method value of the method's own receiver (`f = b.opPlan; … f( … )`) binds to that receiver
type's method only. Every row is a MAY-call clue by field name, never a proven call and never in `count=`. Not joined: a field
named like any Go method in the tree (the call may be that method), a nested literal without its own type, a map or slice
literal, a registrar reached through a method call, two same-named registrars storing different fields, a package whose name
differs from its directory. Measured on cockroach (batcheval `Command.EvalRW`, 29 commands, called by `evaluateCommand`) and
terraform (`Local.opPlan` stored in `f`, called by `Local.Operation`). Gate: `test/fnslotcheck.sh`.
