### Fixed — `--dead-code` and `--safe-delete` read `static` as internal linkage only in C, C++ and Objective-C

The internal-linkage evidence behind `--dead-code` rows (and `--safe-delete`'s `dead_code_candidate=`) was a whole-word `static`
anywhere in the function's signature, in every language. A Python parameter `static=False`, a TypeScript literal type
`'static' | 'dynamic'` and a Rust `'static` lifetime are the same four letters and no linkage, so public functions in those
languages were listed as high-confidence candidates. The token now counts only for a language whose row in
`LangFacts` says the keyword is the storage class (`staticLinkage`: C, C++, Objective-C); a function in the others needs
other evidence and is not a `<d>` row. Found while adding the `<to>` rows of `--dead-code`.
