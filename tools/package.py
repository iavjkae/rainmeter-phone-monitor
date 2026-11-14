"""Stage an installable tree without adding generated binaries to Git."""

import argparse
import shutil
import tomllib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def public_skin_file(path: Path) -> bool:
    """Copy only skin source/config files, never local overrides or runtime state."""
    fixed_resources = {
        "DisplayControl.lua",
        "DisplaySettings.inc",
        "Measures.inc",
        "TelemetrySettings.inc",
        "FPSBridge.ini",
        "start_fps.ps1",
    }
    relative = path.relative_to(ROOT / "skins" / "PhoneMonitor")
    if relative.parent == Path("@Resources"):
        return relative.name in fixed_resources
    palettes = {
        name + ".ini"
        for name in tomllib.loads((ROOT / "config/themes.toml").read_text(encoding="utf-8"))
    }
    return relative.parent == Path(".") and relative.name in palettes


def package(build: Path, output: Path, presentmon: bool) -> None:
    required = [build / "PhoneTelemetry.dll", build / "FPSBridge.exe"]
    if presentmon:
        required.extend(
            ROOT / "build" / "vendor" / name
            for name in ("PresentMon.exe", "PresentMon-LICENSE.txt", "PresentMon.sha256")
        )
    for path in required:
        if not path.is_file():
            raise FileNotFoundError(f"Build artifact missing: {path}")
    resources = output / "Skins" / "PhoneMonitor" / "@Resources"
    # Stage from an allowlist and require a new folder: existing output may contain private data.
    if output.exists():
        raise FileExistsError("Package directory already exists; select a fresh --output directory")
    for source in (ROOT / "skins" / "PhoneMonitor").rglob("*"):
        if not source.is_file() or not public_skin_file(source):
            continue
        target = output / "Skins" / source.relative_to(ROOT / "skins")
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, target)
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
    print("Clean package staged; no local overrides or runtime data copied.")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, default=ROOT / "build")
    parser.add_argument("--output", type=Path, default=ROOT / "dist")
    parser.add_argument("--with-presentmon", action="store_true")
    args = parser.parse_args()
    package(args.build_dir.resolve(), args.output.resolve(), args.with_presentmon)
