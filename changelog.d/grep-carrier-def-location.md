### Changed — `--grep`: a carrier `<enc>` row that lists `vals=` now names its own definition as `p=file:line`

A grep `<enc>` row for a variable, constant or initialiser that stores function values (a command table, a route map)
printed `vals="exec=cmd_split_window_exec@76"`: the line of each stored function, but no file for the row itself, so a
reader could not tell which file `@76` was in or where the table was defined. That row now also carries
`p="cmd-split-window.c:58"`, the carrier's own definition; a `vals=` entry without a path is in that file. It is printed
only when the row stands for exactly one definition and lists `vals=`. A row that groups two same-named definitions
(`defs=2`), and a carrier with no stored functions, stay as they were. The MCP `grep` verb's `enclosing` entries carry
the same value under `p`. Measured on the 163 in-sample rows of the round-5 replay: one answer changes (tmux
`--grep=split-window`, +26 bytes), and its two missing definition items (`cmd_split_window_entry` at line 58,
`cmd_split_window_exec` at line 76) are now named with their file. Gate: `test/countfloorcheck.sh` (P/A3).
