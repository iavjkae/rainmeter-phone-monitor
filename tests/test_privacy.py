"""Privacy and package boundaries are checked using only invented filenames and values."""

import ast
import importlib.util
import io
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]


def load_tool(name):
    spec = importlib.util.spec_from_file_location(name, ROOT / "tools" / f"{name}.py")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class PrivacyBoundary(unittest.TestCase):
    def test_force_tracked_runtime_and_diagnostics_are_rejected_without_payload(self):
        privacy = load_tool("privacy")
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for name in (
                "__cmake_systeminformation/system.txt",
                "capture.log",
                "fps-state.txt.tmp",
                "fps-state.txt",
                "FPSBridge.local.ini",
                ".env.review",
                "build-private/identity.txt",
                "probe.dll",
            ):
                with self.subTest(name=name):
                    file = root / name
                    file.parent.mkdir(parents=True, exist_ok=True)
                    file.write_bytes(b"synthetic-private-payload")
                    with (
                        patch.object(privacy, "ROOT", root),
                        patch.object(privacy, "candidates", return_value=[file]),
                        self.assertRaises(ValueError) as error,
                    ):
                        privacy.check()
                    self.assertNotIn("synthetic-private-payload", str(error.exception))
                    self.assertIn(str(file.relative_to(root)), str(error.exception))

    def test_absolute_paths_are_rejected_and_public_urls_are_allowed(self):
        privacy = load_tool("privacy")
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            file = root / "example.md"
            with (
                patch.object(privacy, "ROOT", root),
                patch.object(privacy, "candidates", return_value=[file]),
            ):
                for value in ("Q" + ":/private/example", "Z" + ":\\private\\example"):
                    file.write_text(value, encoding="utf-8")
                    with self.assertRaises(ValueError) as error:
                        privacy.check()
                    self.assertNotIn(value, str(error.exception))
                file.write_text("https://example.invalid/public", encoding="utf-8")
                with patch("sys.stdout", new=io.StringIO()):
                    privacy.check()

    def test_package_does_not_copy_personal_or_unknown_files(self):
        package = load_tool("package")
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            package.ROOT = root
            resources = root / "skins/PhoneMonitor/@Resources"
            resources.mkdir(parents=True)
            (root / "config").mkdir()
            (root / "config/themes.toml").write_text(
                '["Example"]\nMode="EXAMPLE"\n', encoding="utf-8"
            )
            (root / "skins/PhoneMonitor/Example.ini").write_text("[Rainmeter]\n", encoding="utf-8")
            (resources / "FPSBridge.ini").write_text("[Selection]\nExclude=\n", encoding="utf-8")
            (resources / "FPSBridge.local.ini").write_text(
                "synthetic private fixture", encoding="utf-8"
            )
            (resources / "custom.ini").write_text("synthetic private fixture", encoding="utf-8")
            (resources / "fps-state.txt").write_text("synthetic runtime fixture", encoding="utf-8")
            build = root / "build"
            build.mkdir()
            for name in ("PhoneTelemetry.dll", "FPSBridge.exe"):
                (build / name).write_bytes(b"synthetic build fixture")
            for name in ("LICENSE", "THIRD_PARTY_NOTICES.md", "README.md"):
                (root / name).write_text("synthetic public fixture", encoding="utf-8")
            output = root / "output"
            package.package(build, output, False)
            names = {path.name for path in output.rglob("*") if path.is_file()}
            self.assertIn("Example.ini", names)
            self.assertIn("FPSBridge.ini", names)
            self.assertFalse(names & {"FPSBridge.local.ini", "custom.ini", "fps-state.txt"})
            with self.assertRaises(FileExistsError):
                package.package(build, output, False)

    def test_package_allowlist(self):
        package = load_tool("package")
        skin = ROOT / "skins" / "PhoneMonitor"
        for name in ("DisplayControl.lua", "FPSBridge.ini", "TelemetrySettings.inc"):
            self.assertTrue(package.public_skin_file(skin / "@Resources" / name))
        for name in (
            "FPSBridge.local.ini",
            "custom.ini",
            "Rainmeter.ini",
            "state.txt",
            "capture.png",
        ):
            self.assertFalse(package.public_skin_file(skin / "@Resources" / name))

    def test_hardware_opt_in_and_no_payload_print(self):
        tree = ast.parse((ROOT / "tools/smoke_telemetry.py").read_text(encoding="utf-8"))
        for node in ast.walk(tree):
            if (
                isinstance(node, ast.Call)
                and isinstance(node.func, ast.Name)
                and node.func.id == "print"
            ):
                names = {name.id for name in ast.walk(node) if isinstance(name, ast.Name)}
                self.assertFalse(names & {"payload", "fields", "usage"})
        source = (ROOT / "tools/smoke_telemetry.py").read_text(encoding="utf-8")
        self.assertIn('"--hardware"', source)

    def test_generation_is_host_independent(self):
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "skin"
            subprocess.run(
                [sys.executable, "tools/generate_themes.py", "--output", str(output)],
                cwd=ROOT,
                capture_output=True,
                check=True,
            )
            for generated in output.rglob("*"):
                if generated.is_file():
                    self.assertEqual(
                        generated.read_bytes(),
                        (ROOT / "skins/PhoneMonitor" / generated.relative_to(output)).read_bytes(),
                    )


if __name__ == "__main__":
    unittest.main()
