### Added — Go functions stored in struct fields are joined to the calls through those fields (`--callers`, `--callees`, `--for`)

A Go function stored in a struct field — a keyed composite-literal element (`Command{ EvalRW: Put }`), a field assignment
(`c.EvalRW = Put`), or an argument of a registrar that stores its parameter there (`RegisterReadWriteCommand( kvpb.Put,
declareKeysPut, Put )` with `Command{ EvalRW: impl }` in its body) — is now joined to every member call through a field of that
name (`cmd.EvalRW( … )`): `--callers=Put` gains `called_by= called_at= via="slot-name" cands=K` on its `<vr>` row (K = the
distinct functions that slot holds), `--callees` of the dispatcher lists `<vs through="cmd.EvalRW" slot=".EvalRW" targets=K>`, and
a `--for` answer marks a shown function row with `slot_by=` (the functions that may call it through a slot) and a shown struct
row with `slot_fields="F=K@C"`; a caller that matches by the field name alone is marked `~` (`slot_by="~evaluateCommand"`,
`@~C`): a candidate, not a fact. A method value of the method's own receiver (`f = b.opPlan; … f( … )`) binds to that
receiver type's method only. Slot types are keyed by their package DIRECTORY (`x/types.Cmd` and `y/types.Cmd` are two types; a
qualified literal `pkg.T{…}` maps `pkg` through the file's import), a registrar by its definition (a same-named function of
another package, or a method, never feeds the slot), and a slot never joins across languages. Every row is a MAY-call clue,
never a proven call and never in `count=`. Not joined: a field named like any Go method in the tree (the call may be that
method), a nested literal without its own type, a map or slice literal or a named map type, a registrar reached through a
method or a qualified call, one registrar storing a parameter in two fields; a `slot_by=` whose only store is in test code is
not printed on a non-test row. Measured on cockroach (batcheval `Command.EvalRW`, 29 commands, called by `evaluateCommand`)
and terraform (`Local.opPlan` stored in `f`, called by `Local.Operation`). Gate: `test/fnslotcheck.sh`.
