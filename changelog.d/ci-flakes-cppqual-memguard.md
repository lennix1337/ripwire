### Fixed — two recurring macOS CI reds: the C++ qualified-call gate's cold parse and the memory guard's parse-stop arm

Neither is a product change; both were gates whose own calibration did not survive a shared macOS runner.

- `test/cppqualcheck.sh`: the first `ripwire . --uses=…` call at the repo root parses all ~4,400 tracked files cold. That
  is CPU-bound, so it slows in step with the load: 3.8 s alone on a 10-core Mac, 11.7 s next to three more cold parses,
  28.7 s next to seven. A macOS CI runner has 3 or 4 cores shared by three gates, and the gate's 30 s per-call alarm sat
  just under that, so the macOS Release shard that held it went red on most runs of 2026-10-26. `test/pargates.py` now
  hands the gate the same budget scale the harness already applies to the gate's whole budget
  (`PARGATES_BUDGET_SCALE`, 4 on CI, 1 locally). A cold repo-root call gets the alarm times that factor, a warm call keeps
  the plain alarm, and a hang still trips either. The gate prints each cold call's time against its budget.
- `test/memguardcheck.sh` (B12/B12b): the arm's fixture reached the memory guard's parse line (half the limit) through the facts the
  parse retains, and that shape depends on the worker count. With the pool forced to the macOS runner's 3 workers, the parse
  needed 33 to 34 files to read 32 MB, almost all of it facts the graph build then roughly doubles, so the run ended on the
  hard line right after the stop (macOS Release, run of 2026-10-27: "34 of them parsed", then exit 5), while the 10-core
  development host stopped after 10 files and passed. The arm now builds a few big files whose parse trees carry the footprint
  (one array-holding function each, no calls) and walks a short ladder of sizes by outcome: no stop means heavier, the hard line
  after the stop means lighter. A red arm now prints the rungs it tried and the unguarded peak in the failing line itself. The
  product's parse line and cut rule are unchanged.
