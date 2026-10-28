### Fixed — a JavaScript or TypeScript method row names its class, and a printed `file:LINE` selects that definition

JavaScript and TypeScript store no owner scope, so every member row printed a bare name: nunjucks' `environment.js` has two
`render` methods, and `--path=render,compile`, `--callers`, `--impact` and the `<d>` signature rows printed `render` twice with
nothing to tell them apart. A method row now prints the one class whose body holds it (`Environment.render`, `Template.render`),
read by the same containment rule the `Class.method` spelling binds by, so the printed name resolves back. A method of an object
literal, a method a function or method encloses, and a method inside two nested classes stay bare, and so do the member names in the hedged `<bridge>`, `<gap>` and `<near_sink>` rows of an
unreached `--path` answer (their `p=` tells definitions apart). The `p=` a row prints is a
selector too: `--callers=nunjucks/src/environment.js:445` answers about `Template.render`. It is tried only after every name
spelling found nothing, so a numeric JSON key keeps resolving as a name, and it binds only the definition whose own signature line
it is (a body line stays "not found"). The legend says how a TS/JS owner was read only when such a row is shown. Measured on the
163 S1 rows: 35 JS/TS rows change, Rule C lower on none, nunjucks-14 gains three rubric items (`Template.render`,
`Template.compile`, `Compiler.compile`). Gate: `test/jsdefnamingcheck.sh`.
