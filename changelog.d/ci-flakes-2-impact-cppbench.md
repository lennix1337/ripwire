### Fixed — two gates that timed out on slow macOS runners: the impact-partition seed loop and the C++ localization smoke gate

Neither is a product change; both were gates whose own cost outgrew the budget a slow shared runner leaves them.

- `test/impactpartitioncheck.sh`: the gate compares `--impact` against `--test-gate` for every symbol `src/graph.h` defines. That
  file now defines 577 distinct names, and the loop spawned one `ripwire . --impact=` process per name, each loading the cache and
  building the call graph of the ~4,400-file repo again (1.7 s each on an idle 10-core Mac, 870 s on a macOS CI runner, 1200 s
  budget: killed on 2026-10-27). The loop now asks one persistent `--mcp` server, started with `--mcp-legend=inline` so each answer
  is the document the command line prints. Every assertion reads the same documents as before. To keep the swap honest, every
  15th seed plus the first three type seeds are also run through the command line and must be byte-identical to the server's answer
  (root path aside), with a planted-difference control; `IMPACTPARTITION_CLI=1` runs every seed through the command line as before.
  Wall time on a busy 10-core Mac: 1980 s before, 156 to 210 s after.
- `test/cppbenchcheck.sh` and `test/pargates.py`: the gate ran three scoring passes, the middle one only to restore a lock it had
  just tampered with. The tamper now edits a copy, the determinism comparison is two independent cold index builds, and the re-mine
  is its own arm on one benchmark arm instead of three that also checks the re-mined lock hash matches. That is 57 `ripwire`
  calls instead of 69. The gate was still killed at its 1200 s budget on three slow macOS draws (2026-10-13, 10-23, 10-27; 640 s
  when green), so its declared budget is now 2400 s, twice the figure the slowest-runner arithmetic gives (worst green 856 s times
  the 1.5 slowdown seen on every gate that day).
