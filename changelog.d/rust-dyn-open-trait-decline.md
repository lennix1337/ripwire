### Fixed — Rust: a `&dyn Trait` call never binds a same-file impl by name when the trait's implementor list is open

When a trait's implementors could not all be listed (two same-named implementor types — mdbook's test crate has two `Spy`
structs —, `impl T for &X`, a blanket impl), the dispatch tier stepped aside and the name tiers bound `renderer.render( … )` on a
`&dyn Renderer` to the one impl in the caller's file (mdbook `execute_build_process` → `TestRenderer::render`, via="name"). Now
the call declines as `<dc gate="iface">` over every bodied method of that name inside one of the trait's `impl T for X` blocks
(plus the trait's own default): mdbook's five renderers. It is never a proof (a macro-made impl writes no body the index sees),
an open list with a single candidate (the trait's default) declines too, never a by-name inherent method of the caller's file, an
`impl other_crate::Renderer for X` is no in-tree implementor, and a trait name another in-tree type shares answers nothing. Every `gate="iface"` row now lists its non-test candidates first
(`cand=` order on `--legend=full`; the set and its count are unchanged). Gate: `test/receiverfixescheck.sh` (R).
