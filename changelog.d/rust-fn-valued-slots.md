### Added — Rust functions stored in struct fields are joined to the `(x.f)( … )` calls through those fields (`--callers`, `--callees`, `--for`)

A Rust function stored in a struct field — a struct expression (`CallEval { eval: direct_eval }`, `Self { eval }`), a value
wrapped in `Box::new` / `Rc::new` / `Arc::new`, a field assignment (`self.h = h`), or an argument of a registrar reached by a
path call that stores its parameter there (`CallEval::new( stack, eval_block_with_early_return::<D> )` with `Self { …, eval }`
in its body; a method's `self` is position 0) — is now joined to the calls through that field, `(self.eval)( … )` (the only
spelling Rust has for calling a function-valued field). `--callers` gains `called_by= called_at= via= cands=K` on a `<vr>`
row, `--callees` lists `<vs through="self.eval" slot="CallEval.eval" targets=K>`, and `--for` prints `slot_by=` /
`slot_fields="F=K@C"` on shown rows. A slot type, a stored function and a registrar are the in-tree definition their path AS
WRITTEN reaches: a module path (`crate::a::Cfg`, `super::eval::f`, `mod a; a::f`) the one file it names, never the writing
file's own namesake; a path led by an in-tree crate's name that crate; a bare name its own file's, else its crate's, else the
repo's only one in a crate the file `use`s (`Self` is the impl type). `std` / `core` / `alloc`, an out-of-tree crate
(`use tokio::spawn`, `use extcrate::Hook`), a `use … as` rename or an unknown module reach no in-tree definition: no join. Two
crates' same-named structs are two slots; a bare name in a file that `use`s an out-of-tree glob (`use extcrate::*`) is its own
file's definition only, since another file's namesake is one guess beside the glob's (it joins by the field name, `via="slot-name"`); a `self.f` call is typed
(`via="slot"`), a call or store through another receiver joins by the field name only (`via="slot-name"`, `~` on `--for`). Not
joined (floors): a closure stored in a field, a method-call registrar (`r.set( f )`), a value inside a macro, an enum variant's
field, a value or registrar inside an inline `mod m { … }` (including `#[cfg(test)] mod tests`: its functions are module
members). Rust value references are read only as path-call arguments and field stores: a method-call argument
(`iter().map( helper )`), `let g = f`, an array element or a returned function is not read, so `--safe-delete` / `--dead-code`
can still call such a function unreferenced. Every row is a MAY-call clue, never a proven call and never in `count=`. Measured on nushell (`CallEval.eval`, called
by `CallEval::run` / `run_prebound`). Gate: `test/fnslotcheck.sh` (R*/N8-N12/X2/F3 arms).
