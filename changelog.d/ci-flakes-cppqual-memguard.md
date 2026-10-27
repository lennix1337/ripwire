### Fixed — two recurring macOS CI reds: the C++ qualified-call gate's cold parse and the memory guard's parse-stop arm

Neither is a product change; both were gates whose own calibration did not survive a shared macOS runner.

- `test/cppqualcheck.sh`: the first `ripwire . --uses=…` call at the repo root parses all ~4,400 tracked files cold. That
  is CPU-bound, so it slows in step with the load: 3.8 s alone on a 10-core Mac, 11.7 s next to three more cold parses,
  28.7 s next to seven. A macOS CI runner has 3 or 4 cores shared by three gates, and the gate's 30 s per-call alarm sat
  just under that, so the macOS Release shard that held it went red on most runs of 2026-10-26. `test/pargates.py` now
  hands the gate the same `--budget-scale` factor it already applies to the gate's whole budget
  (`PARGATES_BUDGET_SCALE`, 4 on CI, 1 locally). A cold repo-root call gets the alarm times that factor, a warm call keeps
  the plain alarm, and a hang still trips either. The gate prints each cold call's time against its budget.
- `test/memguardcheck.sh` (B12/B12b): the arm's fixture sat close to the hard line (OS peak footprint 62.9–66.7 MB of the
  67.1 MB limit, and rc 5 from 1.6 times its function count), so a runner that moved the footprint failed it even through
  its attempt loop (PR #443, macOS plain shard 1). The arm now has its own fixture of the same shape whose big files call
  nothing: peak 35.4–40.4 MB (53–60 % of the limit), still passing at 2400 functions per file. That runner's shift was not
  reproduced locally, so this widens the margin without naming its cause. The product's parse line and cut rule are
  unchanged.
