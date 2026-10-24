#!/usr/bin/env python3
"""Resident partial-index disclosures survive later successes and refusals."""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

BINARY = Path(sys.argv.pop(1)).resolve()


class PartialIndexRefusalTest(unittest.TestCase):
    def test_reused_partial_index_and_other_root(self):
        with tempfile.TemporaryDirectory(prefix="ripwire-partial-refusal-") as scratch:
            root = Path(scratch)
            partial = root / "partial"
            for folder in "abcd":
                directory = partial / folder
                directory.mkdir(parents=True)
                for index in range(18):
                    (directory / ("f%02d.c" % index)).write_text("int %s_%02d(void){return 1;}\n" % (folder, index))
            whole = root / "whole"
            whole.mkdir()
            (whole / "probe.c").write_text("int probe(void){return 1;}\n")
            requests = [{"jsonrpc": "2.0", "id": 1, "method": "initialize"}]
            for identifier, name, path, arguments in [
                (2, "grep", partial, {"pattern": "a_01"}),
                (3, "grep", partial, {"pattern": "a_02"}),
                (4, "find_symbol", partial, {"symbol": "__missing_symbol__"}),
                (5, "find_symbol", whole, {"symbol": "__missing_symbol__"}),
            ]:
                requests.append({"jsonrpc": "2.0", "id": identifier, "method": "tools/call", "params": {"name": name, "arguments": dict(arguments, path=str(path))}})
            completed = subprocess.run(
                [str(BINARY), "--mcp", "--max-memory=64M"], cwd=root,
                env=dict(os.environ, RIPWIRE_TEST_MEMGUARD="crawl:30"),
                input="".join(json.dumps(request) + "\n" for request in requests),
                text=True, capture_output=True, timeout=30,
            )
            self.assertEqual(completed.returncode, 0, completed.stderr)
            responses = {response["id"]: response for response in map(json.loads, completed.stdout.splitlines()) if "id" in response}
            first = responses[2]["result"]
            second = responses[3]["result"]
            self.assertIn("memory guard", first.get("_memory_stop", ""))
            self.assertIn("memory guard", second.get("_memory_stop", ""))
            self.assertEqual(first["_index"], second["_index"], "fixture must reuse the same partial index")
            error = responses[4]["error"]
            self.assertEqual(error["code"], -32602, "a miss must remain an MCP refusal")
            self.assertIn("memory guard", error["message"], "a resident partial-index miss must disclose its incomplete evidence")
            self.assertEqual(responses[5]["error"]["code"], -32602)
            self.assertNotIn("memory guard", responses[5]["error"]["message"], "another complete root must not inherit a stale warning")


if __name__ == "__main__":
    unittest.main()
