### Added — queue hand-offs join both hedged walks (`via="queue"`)

A Python producer that puts onto a queue field of its own object (`self.q.put( item )`, `self.q.put_nowait( item )`) now
hands off to the functions that take from it (`self.q.get()`, `self.q.get_nowait()`) in the same class or a base or subclass
of it, and to their proven callers in that family, one level up (`message = await self._get_message()`). `--impact` lists
such a hop as a `<rh … via="queue" … d="1" cands=K/>` row and counts it in `reaches_hedged=`. The `--path` hedged
`<bridge>` crosses it. On textual, `--path=post_message,_dispatch_message` used to end with no bridge. It now prints
post_message → `_process_messages_loop` (`via="queue" cands="3"`) → `_dispatch_message`, and `--path=post_message,on_*`
reaches `on_event` the same way.

What it does NOT mean: a hand-off is not a call. It is never counted in `reaches=`, `reachable=`, `hops=` or `by_depth=`,
and `<bridge proven="0">` stays. The pairing reads the attribute name, the class family and the argument count: a
`put( k, v )` or `get( k )` is no queue operation, and a sibling class or another class with the same attribute name never
pairs. The family is the resolved class: a base counts only where the index proves it (the same-file declaration the clause
binds to, or an import that names it), never a base spelled the same in another file. `timeout=` and `block=` are the queue
signature's own keywords and count as no argument (`get( timeout=2 )`, `put( v, block=False )` pair); the one floor is that
a call with any other keyword is read as no queue operation. A queue the class hands to a worker as an argument, or reads
through a local alias, has readers this rule does not see, so the readers listed are a floor. The rule reads Python only:
Go channels, Rust split handles (`mpsc::channel` sender and receiver) and JS/TS event emitters are not covered yet. Gate:
`test/queuehopcheck.sh`.
