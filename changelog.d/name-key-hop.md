### Added — name-keyed lookups join both hedged walks (`via="name-key"`)

A function that looks a method up by a name built from a template now hops to the definitions that name fits, in both hedged
walks. The sites are the template lookups `--callers` already listed as `<pd>` rows: `getattr( self, 'do_' + cmd )`,
`hasattr`, `X.__dict__.get( f"on_{n}" )` in Python, and `o[ 'handle' + e ]` or ``o[ `handle${e}` ]`` in JavaScript and
TypeScript. The `--path` hedged `<bridge>` crosses such a lookup (`<s … via="name-key" cands=K/>`) when the template names at
most 16 definitions. `--impact` follows it in its hedged ring, so `reaches_hedged=` counts the dispatcher and its proven
callers, and a two-hop chain prints `<rh … via="name-key" …/>`. This is slice 7a of issue #375 (a shared capture for calls by
method name), built on the IX-3 template capture.

What it does NOT mean: the name is matched, not traced. A hop is never counted in `reaches=`, `reachable=`, `hops=` or
`by_depth=`, and `<bridge proven="0">` stays. A fitting definition may never be looked up at run time. A `self`, `this` or
`cls` receiver keeps the site's class family only (the class definition that encloses the call, its ancestors and its subclasses),
so a sibling class's `do_rm` is no candidate, in Python, JavaScript and TypeScript, and a class of the same name in another file
is another class. A `hasattr( self, 'do_' + c )` test and a store `this[ 'on' + e ] = fn` are still listed as `<pd>` clues but
draw no hop: neither makes a call through the name. The floors: a lookup by a
plain variable (`getattr( self, name )`), by a literal name (`getattr( self, "do_ls" )`, `obj[ "m" ]()`, Go
`MethodByName( "M" )`), or by a template whose literal side is under three identifier characters (`'on' + e`, `f"_{m}"`)
draws no hop. A name assigned to a local first (`mname = 'do_' + self.command; getattr( self, mname )`) draws no hop either.
Gate: `test/namekeyhopcheck.sh`.
