#!/usr/bin/env python3
"""Doctor legends remain stable when the isolated cache crosses its scan cap."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
import xml.etree.ElementTree as ET

BINARY = Path(sys.argv.pop(1)).resolve()


class DoctorCacheLegendTest(unittest.TestCase):
    def test_scan_cap_changes_declared_fields_not_their_dictionary(self):
        with tempfile.TemporaryDirectory(prefix="ripwire-doctor-legend-") as scratch:
            sandbox = Path(scratch)
            corpus = sandbox / "corpus"
            corpus.mkdir()
            (corpus / "probe.cpp").write_text("int probe(){return 1;}\n")
            cache = sandbox / "cache"
            cache.mkdir()
            env = dict(os.environ, TMPDIR=str(cache), TMP=str(cache), TEMP=str(cache), XDG_CACHE_HOME=str(cache))

            def doctor(dialect):
                result = subprocess.run(
                    [str(BINARY), str(corpus), "--doctor", "--no-cache", "--legend=" + dialect],
                    env=env, capture_output=True, text=True, timeout=30,
                )
                self.assertIn(result.returncode, (0, 1), result.stderr)
                row = ET.fromstring(result.stdout).find("./c[@n='cache-dir']")
                self.assertIsNotNone(row, result.stdout)
                legend, marker, _ = result.stdout.partition("-->")
                self.assertTrue(marker, "doctor output must include its dictionary")
                return legend, row.attrib

            before = {dialect: doctor(dialect) for dialect in ("compact", "full")}
            cache_dir = Path(before["compact"][1]["dir"]).resolve()
            self.assertTrue(cache_dir.is_relative_to(sandbox), "refusing to write outside the disposable cache")
            for index in range(4100):
                (cache_dir / ("ripwire-blob-%d.bin" % index)).touch()
            for dialect in ("compact", "full"):
                with self.subTest(dialect=dialect):
                    legend, row = doctor(dialect)
                    self.assertNotIn("blobs_floor", before[dialect][1])
                    self.assertEqual(row.get("blobs_floor"), "1", "fixture must cross the scan cap")
                    self.assertIn("blobs_floor=", legend, "a capped count must remain defined")
                    self.assertEqual(before[dialect][0], legend, "live counts must not change dictionary prose")


if __name__ == "__main__":
    unittest.main()
