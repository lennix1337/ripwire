### Fixed — a Python call through a renamed import (`from m import f as g; g()`) now reaches `m.f`

`--callers=f` listed no caller for a function only ever called under its import alias, and the call bound by its local
spelling instead, to any unrelated definition named `g` (a hedged `via="name"` row). The call now binds m's `f` as a proven
edge when the rename is the name's one binding where the call runs and `m` is one indexed file that defines `f` once at
module scope. It stays undecided (the previous answer) when a parameter, a local, an enclosing function, a later module-scope
assignment, a `def g`, a `global`, a star import or a dynamic namespace write may rebind the name, when two imports of the
name disagree (`try: from m import f as g / except: from n import f as g`), when the import sits inside another function,
and when `m` is a package that only re-exports `f` (a known floor). The rename is read from the shared binds channel added
for CONTRIBUTING slice 2a (`@import.names` / `@import.alias` captures → one `RawBind` kind for every language, Python
moved onto it with byte-identical output). Gate: `test/pyaliascallcheck.sh`.
