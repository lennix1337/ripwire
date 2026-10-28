### Fixed — a bare `module.exports = (…) => …` is a function, named by its file

`module.exports = (stream) => { … }` in koa's `lib/is-stream.js` defined no symbol: its body's calls sat in the file's
`<file-scope>` row and no verb could name it. The anonymous function a module exports is now a function named by the file's stem
(`is-stream`), the rule a Markdown file's own node is named by; a named function expression keeps its own name
(`module.exports = function onlyNamed() {}`), and `module.exports.NAME = fn` is unchanged. Side effect, measured on koa: test helpers
written this way (`test-helpers/context.js`) are now named like the local that requires them, so their callers bind by name
(unresolved calls 210 → 50). Not fixed: a call through a different local name (`const isStream = require('./is-stream.js')`)
still has no edge, so koa-11's rubric item for `lib/is-stream.js` is not yet credited. Gate: `test/jsdefnamingcheck.sh` (3).
