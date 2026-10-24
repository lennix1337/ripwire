# historywalkscan.py — arm 9 of test/ripwirepubliccheck.sh: the unpinned git-history walk sweep.
#
# Usage: python3 historywalkscan.py TRACKED_Z OK_SET   (run from the repository root)
#   TRACKED_Z  a NUL-separated pathname list (`git ls-files -z`)
#   OK_SET     newline-separated `path:contenthash` exemptions (sha256 of the matched line, first 16 hex digits)
# Prints one line per offender; prints nothing for a clean sweep. The heuristic, the scope and the exemption
# policy are documented where they are owned: the arm 9 comment block in test/ripwirepubliccheck.sh.
#
# MARKDOWN FENCES. A .md file is scanned only inside a shell-language fence (a recipe, not prose). The tracker
# follows the CommonMark delimiter rules that matter here: a fence opens on a run of three or more backticks or
# tildes, and closes only on a bare run of the SAME character at least as long as the opener. Every other line
# inside a fence — including a shorter run of backticks and a nested language-tagged opener — is content. That
# is what keeps a docs/captures wrapper (five backticks around recorded output) opaque when the recorded text
# itself contains a truncated three-backtick shell opener: the nested opener is output, and the prose after the
# wrapper closes is prose, not a recipe.
import hashlib, os, re, sys

SCOPE_RE = re.compile(r'^(bench/|docs/|scripts/|test/lib/)')
SELF = 'test/ripwirepubliccheck.sh'
GIT_LOGREV = re.compile(r'git[^|&;\n]{0,40}\b(log|rev-list|for-each-ref)\b')
# CASE-SENSITIVE on purpose: under re.IGNORECASE `\bHEAD\b` matched the `| head -n 50` a shell recipe pipes
# into, and an unpinned `git log --format=%H | head -n 50` read as pinned. Each accepted spelling is listed.
PIN_SIGNAL = re.compile(r'\bHEAD\b|--ref\b|\b(?:ref|REF)\b|\bPIN\b|\b[0-9a-fA-F]{7,40}\b|\bv[0-9]+\.[0-9]+(?:\.[0-9]+)?\b')
ALL_TOKEN = re.compile(r'--all\b|--branches\b|--remotes\b')
SHELL_LANGS = ('bash', 'sh', 'shell', 'zsh')
FENCE_RUN = re.compile(r'^(`{3,}|~{3,})(.*)$')


def fenceOpening(stripped):
    """(fenceChar, fenceLength, isShell) when this line opens a fence, else None."""
    m = FENCE_RUN.match(stripped)
    if not m:
        return None
    run, info = m.group(1), m.group(2).strip()
    if run[0] == '`' and '`' in info:
        return None   # a backtick run with a backtick in its info string is inline code, not a fence
    return run[0], len(run), info in SHELL_LANGS


def fenceCloses(stripped, fenceChar, fenceLength):
    """True when this line is a bare run of fenceChar at least fenceLength long."""
    return len(stripped) >= fenceLength and stripped == fenceChar * len(stripped)


def main(trackedPath, okText):
    paths = [p.decode('utf-8', 'surrogateescape')
             for p in open(trackedPath, 'rb').read().split(b'\0') if p]
    okSet = {l.strip() for l in okText.splitlines() if l.strip()}
    for p in paths:
        if p == SELF or not SCOPE_RE.match(p):
            continue
        ext = os.path.splitext(p)[1]
        if ext not in ('.py', '.sh', '.md'):
            continue
        try:
            data = open(p, 'rb').read()
        except OSError as e:
            print(f'{p}: tracked in-scope file could not be read ({e.strerror}) — arm 9 did not scan it')
            continue
        if b'\0' in data:
            continue   # binary, skip
        lines = data.decode('utf-8', 'replace').split('\n')
        fence = None   # (fenceChar, fenceLength, isShell) of the fence the line sits in, else None
        for lineIndex, line in enumerate(lines):
            i = lineIndex + 1
            stripped = line.strip()
            if ext == '.md':
                if fence is None:
                    fence = fenceOpening(stripped)
                    continue   # an opener line is never scanned; a prose line outside a fence is skipped
                if fenceCloses(stripped, fence[0], fence[1]):
                    fence = None
                    continue
                if not fence[2]:
                    continue   # recorded output or another language: content of an opaque fence
            if stripped.startswith('#'):
                continue
            m = GIT_LOGREV.search(line)
            if not m:
                continue
            # test the PIN/--all signals against the line with the matched invocation text masked out —
            # `for-each-ref`'s own command name contains a `\bref\b`-shaped substring, which otherwise
            # satisfies PIN_SIGNAL by itself and makes every for-each-ref call read as pre-pinned,
            # including the unpinned `for-each-ref ... | git log` shape arm 9 exists to catch
            rest = line[:m.start()] + line[m.end():]
            if not (ALL_TOKEN.search(rest) or not PIN_SIGNAL.search(rest)):
                continue   # a ref-shaped token is on this line — the floor calls it pinned
            # keyed by CONTENT, not by line number: an edit anywhere else in the file must not move this
            # exemption off its target, and a real edit to this exact line must drop it back into the sweep
            contentHash = hashlib.sha256(line.encode('utf-8', 'surrogateescape')).hexdigest()[:16]
            if f'{p}:{contentHash}' in okSet:
                continue   # allowlisted — this exact line's content is unchanged from the reviewed hit
            print(f'{p}:{i}: unpinned {m.group(0)!r} — {line.strip()[:140]}')


if __name__ == '__main__':
    main(sys.argv[1], sys.argv[2])
