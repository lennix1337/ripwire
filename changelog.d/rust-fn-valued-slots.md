### Added — Rust functions stored in struct fields are joined to the `(x.f)( … )` calls through those fields (`--callers`, `--callees`, `--for`)

A Rust function stored in a struct field — a struct expression (`CallEval { eval: direct_eval }`, `Self { eval }`), a value
wrapped in `Box::new` / `Rc::new` / `Arc::new`, a field assignment (`self.h = h`), or an argument of a registrar reached by a
path call that stores its parameter there (`CallEval::new( stack, eval_block_with_early_return::<D> )` with `Self { …, eval }`
in its body; a method's `self` is position 0) — is now joined to the calls through that field, `(self.eval)( … )` (the only
spelling Rust has for calling a function-valued field). `--callers` gains `called_by= called_at= via= cands=K` on a `<vr>`
row, `--callees` lists `<vs through="self.eval" slot="CallEval.eval" targets=K>`, and `--for` prints `slot_by=` /
`slot_fields="F=K@C"` on shown rows. A slot type is the in-tree struct its name spells (its own file's, else its crate's, else
the repo's only one; `Self` is the impl type), so two crates' same-named structs are two slots; a `self.f` call is typed
(`via="slot"`), a call or store through another receiver joins by the field name only (`via="slot-name"`, `~` on `--for`). Not
joined (floors): a closure stored in a field, a method-call registrar (`r.set( f )`), a value inside a macro, an enum variant's
field. Every row is a MAY-call clue, never a proven call and never in `count=`. Measured on nushell (`CallEval.eval`, called
by `CallEval::run` / `run_prebound`). Gate: `test/fnslotcheck.sh` (R*/N8-N12/X2/F3 arms).
