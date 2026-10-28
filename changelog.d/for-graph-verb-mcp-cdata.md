### Fixed — the MCP `for` callers block carries its JSON as CDATA

The MCP twin of the graph-verb block answers `callers of X` with `find_referencing_symbols` JSON. It was XML-escaped (every quote
`&quot;`, 1.38x the bytes); it now rides in a CDATA section, and the tag says `dialect="json"` (legend clause only with it). Gate:
`test/forroutecheck.sh` (x2).
