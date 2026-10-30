### Changed — the versions this change moves, stated once

`kParserVer` 259 → 271 (file tag 272): the C++ member-pointer slot records (a `&Cls::m` value reference, a
`( o.*x.f )( … )` slot call, a C nested designated path or macro-cast slot call, a class body fact's scope path, and a C++
base that is a template parameter of the deriving class; 269 on main), the interface-receiver peel's records (a C++
smart-pointer alias carries its pointee, a member whose written type is a template parameter is marked, a Rust `&dyn T` /
`Box<dyn T>` / `impl T` result is recorded) and the TS/JS re-export specifier records each carried lower numbers on their
branches (257–262), so a cache written before them would replay the older rows. 271 sits above every live branch and
pull-request number (the string/literal records carry 263, 265 and 267 on theirs). `kCacheVersion` 46 → 48: a re-export
specifier is a new binding kind appended to the cache record (its branch carried 47). A cache written by 0.6.9 is re-parsed
once. The quality-snapshot scheme stays 24: no snapshot record changes shape. The session legend dictionary is `dictv=1d522ffeeb3f37c2 entries=981`.
