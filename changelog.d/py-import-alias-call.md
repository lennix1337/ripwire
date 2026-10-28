### Fixed — a Python call through a renamed import (`from m import f as g; g()`) now reaches `m.f`

`--callers=f` listed no caller for a function only ever called under its import alias, and the call bound by its local
spelling instead, to any unrelated definition named `g` (a hedged `via="name"` row). The call now binds m's `f` as a proven
edge when the rename is the name's one binding where the call runs and `m` is one indexed file that binds `f` only by one
module-scope definition. It stays undecided (the previous answer) when a parameter, a local, a comprehension target or lambda
parameter covering the call, an enclosing function, a later module-scope assignment, a `def g`, a `global`, a star import or a
dynamic namespace write may rebind the name, when two imports of the name disagree (`try: from m import f as g / except: from n
import f as g`), when the import sits inside another function, and when `m` is a package that only re-exports `f` (a known
floor). It also stays undecided when `m` itself rebinds `f` after the definition (`from fast import f`, a try-import,
`f = other`, `f = deco(f)`, a `globals()` write, a `global f` in a function) or ends in a star import that may bind `f`: a
star import of a file outside the indexed tree (`from _operator import *`) is unknown, so the call keeps the previous answer
there. The rename is read from the shared binds channel added for CONTRIBUTING slice 2a (`@import.names` / `@import.alias`
captures → one `RawBind` kind for every language, Python moved onto it with byte-identical output). Gate:
`test/pyaliascallcheck.sh`.
