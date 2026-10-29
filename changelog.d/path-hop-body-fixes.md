### Fixed — the middle-hop body and the wrapper follow say what they prove

- A middle hop reached by name (`obj.process(x)` with two `process` definitions) prints its body with `via="name" cands="2"` on
  the `<b>`, never as if it were the proven callee.
- A `--for` wrapper follow needs a function or method whose text holds exactly one call site of any kind, that call its last
  statement: a declined, external or undefined call beside it, logic around it, a loop aggregator or a class field initializer
  is no wrapper. A C `*out = v;` line counts as a statement; a Python docstring does not.
- `follow_of=` names the wrapper as its row does (`ChildRunner.launch`), and one answer follows at most 3 bodies; past that
  `<follow_cut n= next=>` counts the wrappers left and names the first callee.
