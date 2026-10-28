### Added — a posted message class names its handlers (`via="name-key"`, `<handler_rule>` on `--path`)

A Python message framework dispatches by a name the message's class carries: textual's `Message.__init_subclass__` writes
`cls.handler_name = f"on_{name}"`, and the pump looks the handler up with `cls.__dict__.get( method_name )`. That lookup takes a
parameter, so it can name every `on_*` method. The class is known where the message is posted, though:
`self.post_message( Button.Pressed( self ) )` names `Button.Pressed`, and its handler is `on_button_pressed`. Such a post site
is now a name-key hop to the methods of that name, with `cands=K` beside it. The hop shows in `--impact`
(`reaches_hedged_via="name-key"`), in the `--path` hedged bridge (at most 16 candidates), and in `near_sink`, where a
dispatched handler is now `why="declined:N" via="name-key"`, no longer `why="external"`. When `from=` is a poster and the
posted classes name a `to=` handler, a reachable="0" `--path` also prints `<handler_rule>`. It holds the rule's `p=` and its
spelling, the functions that read the handler-name attribute with the method each one passes it to (textual:
`MessagePump._on_message` passes it to `MessagePump._get_dispatch_methods`, the MRO walk), and the posted classes, widest
first. The handler definitions are listed only when there are at most 16 of them. Above that, the answer gives a count and
`next=`, never a cut list. This is the post-site half of the name-keyed dispatch that lane string-keyed declined at the lookup.

The rule is read from the repo's own source, never from a framework list. It needs a Python `__init_subclass__` that has a
`cls.A = f"P{v}"` line and calls a camel-to-snake converter (a body holding the `[a-z][A-Z]` regex and `.lower()`) on
`__qualname__`'s last two parts or on `cls.__name__`. A `{namespace}_` branch reads a `namespace="x"` class keyword. A post
site is a call to a method of a class that reads `.A` outside test paths, with a first argument that constructs a subclass:
`X( … )`, `self.X( … )`, `Owner.X( … )` or `module.X( … )`.

What it does NOT mean: the name is matched, not traced. Which candidate runs depends on the receiving object's class, and the
index does not decide that. A hop is never counted in `reaches=`, `reachable=` or `hops=`. The floors are: a variable
argument (`msg = Tick(); post_message( msg )`); a factory (`events.Click.from_event( … )`); a converter or qualname cut of any
other shape; `@on( Message )`-decorated handlers (already shown as value refs); and a class body's own `handler_name =`
(the rule runs after the class body and overwrites it). Gate: `test/handlerrulecheck.sh`.
