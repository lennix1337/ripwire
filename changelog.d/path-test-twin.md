### Changed — `--path` says when it bound a test or fixture copy of a name

When `--path=A,B` binds `from=` (or `to=`) to a definition inside a test or fixture directory while a definition of the same
name exists outside them, the root now says so — `from_test="1"` (or `to_test="1"`) — and `next=` runs the same path from that
other definition. The binding itself is unchanged: the answer discloses it, it never re-picks silently. On zed,
`Editor::backspace` has an evals fixture copy whose chain to `Buffer::edit` is shorter than the editor crate's, so the shortest
answer could bind the fixture. Answers without such a binding keep their bytes; MCP `path_between` prints the same attributes.
