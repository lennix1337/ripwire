### Changed — the versions this change moves, stated once

`kParserVer` 206 → 259 (file tag 260): the Rust written-type receivers carried 202 (203) on their branch and the struct-field
slot join 204 (205), and its fix rounds changed what 204 extracts, so a cache from that branch would replay the older slot
rows; the interface-dispatch records carried 210 and 212 on theirs; the JS/TS written-binding records carried 218 and the
Python rebind-withholding records a 224 placeholder on theirs; train 34 took 226; the Kotlin trailing-lambda capture (#155)
carried a 228 placeholder and the Rust precise-capture re-parse 229 (230) — it re-parses a Rust file whose first parse holds
error bytes with its `use<..>` bounds blanked, so the defs differ — and train 37b took 233 (234); the queue hand-off
records (a Python queue call's keyword arguments on its call reference) carried another 228 placeholder on their branch and
train 38 took 235 (236); the receiver typing of TS `#private` fields and of Python annotations and `mod.X( … )` constructors
carried a 237 (238) placeholder on its branch and train 39 took 239 (240); the JS/TS naming records (a bare `module.exports`
function, a named object literal's binding) carried 238, the name-keyed lookup qualifier 241 (242) and the Go function-valued
slots 247 (248) on their branches and train 40 took 251 (252); the Rust slot join carried 249 (250) and the Python conditional
module-scope definition record (a `def` or `class` inside an `if`/`try`/`with`/loop at module scope) 255 (256) on theirs, and the
C/C++ slot join carries 257 (258) on its, so 259 sits above every live branch and pull-request number. A cache
written by 0.6.8 is re-parsed once. `kCacheVersion` 36 → 46: a Rust binding typed only through a trait
(`&dyn T`, `impl T`, a bound), a Rust `impl T for X` block and a JS/TS binding written after its declaration are new binding
records, as is a JS/TS module-scope object literal's binding name, a Kotlin call's trailing-lambda byte (#155) widens the reference record, and a file's parse-health record gains the
Rust re-parse count, so a cache written before them is rejected and rebuilt (the branches that added them used 37, 38, 39, 41, 42 and 45). The quality-snapshot scheme moves 22 → 24 (#228: Django and
unittest hooks and closure self-dispatch change the dead set; old snapshots are not used; the branch used 23). The session legend dictionary is `dictv=05b48ed93c807ee2 entries=981`.
