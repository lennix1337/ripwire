### Fixed — a module `__getattr__` no longer withholds a renamed-import call to a function the module defines

A Python call through a renamed import (`from m import f as g; g()`) stayed undecided whenever `m` defined a module-level
`__getattr__` (PEP 562), as if `__getattr__` could replace `m.f`. It cannot: Python consults it only for a name the module
does not bind. The call now binds `m.f` as a proven edge in that case too, and a module `__getattr__` in the calling file no
longer withholds it either. It still stays undecided when `f`'s definition in `m` sits inside an `if`, `try`, `with`, `match` or
loop at module scope (it may not run, and `__getattr__` then answers), when `m` writes its namespace dynamically (`globals()[...]`,
`setattr(...)`), and in every case listed for the renamed-import change. Known limit, unchanged: a module that rebinds a
name some other way (`vars()[...]`, `locals()[...]`, `exec(...)`, `__dict__[...]`, `sys.modules[__name__].name = ...`) is not
recognized as a dynamic write, so a pin into it can be wrong, now also in files that define `__getattr__`. On CPython's standard library this restores 11
call sites (`re.sub`, `re.escape`, `re.compile`, `platform.system`, `platform.architecture`). Gate: `test/pyaliascallcheck.sh`.
