### Fixed — a JS/TS private member resolves as the rows print it: `Hono.#dispatch`

Train 40 started printing owner-qualified names (`n="Hono.#dispatch"`) on `--path`, `--callers` and `--callees` rows, but pasting
one back failed: `--callees=Hono.#dispatch` answered "not found" and `--path=Hono.#dispatch,compose` exited 1 on hono, because the
spelling splitter read the `#` as Ruby's `Class#method` separator and saw an empty segment. A `#` that starts a segment is now part
of the name, so `Owner.#name` and `file:Owner.#name` bind the one private member they name; Ruby `Cart#total` reads as before, and
`Owner.name` without the `#` still names nothing. Found by the readout-40 grading pass. Gate: `test/writtenspellingcheck.sh` (1b).
