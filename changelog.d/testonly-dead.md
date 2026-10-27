### Added — `--dead-code` lists library functions that only tests call (`test-only=`)

`--dead-code` now prints `<to n= t= p= l=>` rows after its `<d>` rows: library functions whose every indexed
caller is test code (a test path, or an in-file test scope). They are not dead and are not in `count=`; the
index holds no library caller for them. `test-only=N` on the root counts them and is absent at 0. The rows
carry the same internal evidence as the `<d>` rows (`static`), or a Python module-level def with a
leading-underscore name or module/package (PEP 8's internal-interface convention). Any whole-word spelling of
the name in a non-test code file, outside its own signature, keeps a function off the list, so an aliased
import or a module-level call never leaves a used function on it.

Kept off the list: a def a setup.cfg or pyproject.toml entry point names, a def a package glob-imports
(`from ._impl import *`), a def a notebook cell spells, and any `conftest.py` def (pytest's own test file).

Known floors (a candidate, not proof; the default legend names them):

- A use only in a file the crawl does not index (an `.inc` table), in a docs page, or through a name built at run time
  (`globals()["_cmd_" + name]()`) is invisible.
- A public helper package such as `testing/` (numpy.testing's shape) is listed as library code when only tests call it.
- A test helper outside a test path, fixture files, Go `_test` packages outside the `_test.go` file rule and test
  binaries named only in a build file read as library callers (a miss).
- A comment, string or namesake mention in a library file keeps a test-only function off the list.
- Methods are not listed (a class in a private module is often public API).
