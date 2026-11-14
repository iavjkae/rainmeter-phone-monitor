"""Catch errors that can silently break a loaded Rainmeter skin."""

import configparser
import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SKIN = ROOT / "skins" / "PhoneMonitor"


class SkinValidation(unittest.TestCase):
    def test_utf16_and_unique_sections(self):
        for path in [*SKIN.glob("*.ini"), *SKIN.glob("@Resources/*.inc")]:
            with self.subTest(path=path.name):
                self.assertTrue(path.read_bytes().startswith(b"\xff\xfe"), path)
                text = path.read_text(encoding="utf-16")
                parser = configparser.ConfigParser(interpolation=None, strict=True)
                parser.read_string(text)
                self.assertNotRegex(text, r"[A-Za-z]:[\\/]|__BACKUP__|8Digit Pin")

    def test_measure_bindings_exist(self):
        includes = (SKIN / "@Resources/Measures.inc").read_text(encoding="utf-16")
        for path in SKIN.glob("*.ini"):
            with self.subTest(path=path.name):
                text = includes + "\n" + path.read_text(encoding="utf-16")
                parser = configparser.ConfigParser(interpolation=None, strict=True)
                # Measures.inc starts with a Rainmeter section, then the variant's Variables.
                parser.read_string(text)
                measures = {s.casefold() for s in parser if parser.has_option(s, "Measure")}
                for section in parser:
                    for key, value in parser[section].items():
                        if re.fullmatch(r"measurename\d*", key):
                            self.assertIn(value.casefold(), measures, f"{section}.{key}: {value}")

    def test_dependencies_and_fps_state_path(self):
        text = (SKIN / "@Resources/Measures.inc").read_text(encoding="utf-16")
        self.assertIn("FPSStateFile=#@#fps-state.txt", text)
        for path in SKIN.glob("*.ini"):
            self.assertIn(
                "@IncludeSettings=#@#DisplaySettings.inc", path.read_text(encoding="utf-16")
            )
        for name in ["DisplayControl.lua", "DisplaySettings.inc", "start_fps.ps1"]:
            self.assertTrue((SKIN / "@Resources" / name).is_file())


if __name__ == "__main__":
    unittest.main()
