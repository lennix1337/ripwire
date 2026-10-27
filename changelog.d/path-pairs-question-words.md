### Changed — `--path` prints first the reaching pair whose owner you named

When several definition pairs reach, a pair whose `from=` owner (the `to=` owner only when no pair has a `from=` owner word) equals a word of the path argument as typed
(`--path=Loop,matcher.go:extendedMatch` and `Matcher.Loop`) now prints first among the chains of the same proof class, marked
`qword=`; an all-proven chain still leads every hedged one. What it does NOT mean: `qword=` is a name match, not a resolution.
Answers whose argument names no owner keep their bytes.
