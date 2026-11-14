"""Stage an installable tree without adding generated binaries to Git."""

import argparse
import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def package(build: Path, output: Path, presentmon: bool) -> None:
    required = [build / "PhoneTelemetry.dll", build / "FPSBridge.exe"]
    for path in required:
        if not path.is_file():
            raise FileNotFoundError(f"Build artifact missing: {path}")
    resources = output / "Skins" / "PhoneMonitor" / "@Resources"
    shutil.copytree(ROOT / "skins", output / "Skins", dirs_exist_ok=True)
    plugins = output / "Plugins"
    plugins.mkdir(parents=True, exist_ok=True)
    shutil.copy2(required[0], plugins / required[0].name)
    shutil.copy2(required[1], resources / required[1].name)
    for name in ("LICENSE", "THIRD_PARTY_NOTICES.md", "README.md"):
        shutil.copy2(ROOT / name, output / name)
    if presentmon:
        for name in ("PresentMon.exe", "PresentMon-LICENSE.txt", "PresentMon.sha256"):
            source = ROOT / "build" / "vendor" / name
            if not source.is_file():
                raise FileNotFoundError(f"Run tools/fetch_presentmon.py first: {name}")
            shutil.copy2(source, resources / name)
    print(f"Staged package: {output}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, default=ROOT / "build")
    parser.add_argument("--output", type=Path, default=ROOT / "dist")
    parser.add_argument("--with-presentmon", action="store_true")
    args = parser.parse_args()
    package(args.build_dir.resolve(), args.output.resolve(), args.with_presentmon)
