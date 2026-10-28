### Fixed — a line selector never picks the innermost of two definitions, a namespaced class's printed name resolves, and a refusal counts what it refused

A `file:LINE` selector (the `p=` a row prints) used to bind the innermost of two definitions starting on one line
(`class P { q() {} }`, an arrow nested on its parent's line) without saying which. It now refuses, lists the `n=` of each
candidate on stderr, and a line where one definition starts selects it as before. A TS class inside a `namespace` or `module`
prints `C.m`; that name and the namespace-qualified `NS.C.m` both resolve now, and two namespaces holding a class of one name make
the short spelling a refusal instead of a silent pick. A refusal over several fitting definitions (`Widget.run` beside a class
and an object literal of that name, or a line selector with several definitions) adds `refused="N"` to its `found="0"` answer (N definitions
fit, none chosen): refused, not absent. Gate: `test/jsdefnamingcheck.sh` (fix-1 arms, and its absence arms now fail on a non-zero exit).
