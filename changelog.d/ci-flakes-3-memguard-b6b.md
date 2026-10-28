### Fixed — three flaky gate arms: memguardcheck (B6b, B21) and pargatescheck (I control)

None is a product change; each arm depended on something the gate did not control.

- `test/memguardcheck.sh` (B6b): the arm sent a `find_symbol a_02` as the second MCP call against an index cut by the crawl
  trip at entry 30. Which fixture files survive that cut is the filesystem's directory order (the crawl does not sort before it
  stops): on macOS `a/f02.c` is kept; on a Linux runner whose directory order places it later, `a_02` is outside the cut index and
  the answer is a not-found error, which carries no `_memory_stop` envelope field, so the arm read an empty string (red on main
  and on both train-40 PR runs, ubuntu release shard 4/4, green on rerun). The follow-up call is now `grep`, whose answer is a
  result on any partial index, so the arm checks the same thing (a later answer from the same cut index keeps the disclosure) on
  every filesystem; the `find_symbol` call stays as a third probe asserted on the shape it returns, with a NOTE line when it fell outside the cut.
- `test/pargatescheck.sh` (I control): the control copy that puts the old "wait, then look at the stop" loop back waited 0.5 s
  before looking, leaving 0.2 s for the probe gate to start and pass its 0.3 s mark; at load average 9 to 13 it failed three
  runs of three on unmodified main. The control's first wait is now 3.0 s; the ordering defect it models is unchanged.
- `test/memguardcheck.sh` (B21): the arm required the refusal to return within 4 s of a 6 s git-log stream, but the shim's 60
  spawned sleeps stretch that stream on a busy host and the refusal took 5 s at load average 13 to 19. The arm now asks what a
  join would do, which is wait for the stream's end: the shim writes a marker after its last line and the refusal must return
  without it.
