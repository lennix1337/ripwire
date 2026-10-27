### Fixed — Python: annotated fields and `mod.X( … )` constructors type a receiver

`self.f: T`, `self.f: T | None`, `Optional[ T ]` (and `typing.Optional`) annotations now type the field, an annotation types the
field when the value names no class (`self.f: T | None = None` stays a `T`), but a constructed value beats it, as it does for a
local (`self.s: Strategy = FastStrategy()` holds a `FastStrategy`), and a constructor called through a module (`self.m = checker.Manager()`,
`x = checker.Manager()`) types the field or local when the module is in the tree (the written-qualifier check `x: mod.T` already
used: `requests.Session()` proves nothing). A parameter `p: T | None` is read the same way. Before, only a bare `X( … )` value
typed anything, so `self.file_checker_manager.start()` bound the caller's own `start` by name (flake8 `Application`, hop 1 of
`main` → `run_checks` is now proven). A type two in-tree classes share by name stays a split, never proven; a union of two
classes types nothing. Gate: `test/receiverfixescheck.sh` (Y).
