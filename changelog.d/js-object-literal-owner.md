### Fixed — `Widget.run` no longer binds a class's method when an object literal named `Widget` has a `run` too

`const Widget = { run() {} }` in one file and `class Widget { run() {} }` in another: the written spelling `Widget.run` bound the
class method, because the literal's name was stored nowhere. A module-scope `const|let|var NAME = { … }` now records its binding
name, so that spelling names both and is refused ("not found"); a file-qualified `src/widget.ts:Widget.run` still binds the class,
and a literal with another name (`const mock = { run() {} }`) is not `Widget.run`. A literal with no binding name (`export default
{ … }`, `module.exports = { … }`) keeps the earlier rule: its owner is unknown. Gate: `test/jsdefnamingcheck.sh` (2).
