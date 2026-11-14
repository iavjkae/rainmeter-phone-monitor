"""Fetch the pinned, standalone official PresentMon console executable."""

import argparse
import hashlib
from pathlib import Path
from urllib.request import Request, urlopen

ROOT = Path(__file__).resolve().parents[1]
VERSION = "2.6.0"
ASSET = f"PresentMon-{VERSION}-x64.exe"
URL = f"https://github.com/GameTechDev/PresentMon/releases/download/v{VERSION}/{ASSET}"
SHA256 = "b2a706bc6ad475749e3b7e3409263aa1e6906d45bdcf993f6dbc0f660188f1af"


def fetch(destination: Path) -> None:
    request = Request(URL, headers={"User-Agent": "rainmeter-phone-monitor"})
    with urlopen(request, timeout=60) as response:
        data = response.read()
    digest = "sha256:" + hashlib.sha256(data).hexdigest()
    if digest != "sha256:" + SHA256:
        raise ValueError("Pinned PresentMon SHA-256 mismatch")
    if not data.startswith(b"MZ"):
        raise ValueError("Downloaded asset is not a Windows executable")
    destination.mkdir(parents=True, exist_ok=True)
    (destination / "PresentMon.exe").write_bytes(data)
    (destination / "PresentMon.sha256").write_text(digest + "\n", encoding="utf-8")
    license_url = f"https://raw.githubusercontent.com/GameTechDev/PresentMon/v{VERSION}/LICENSE.txt"
    with urlopen(license_url, timeout=60) as response:
        license_text = response.read()
    (destination / "PresentMon-LICENSE.txt").write_bytes(license_text)
    print(f"Downloaded official PresentMon {VERSION}: {len(data):,} bytes; {digest}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=ROOT / "build" / "vendor")
    fetch(parser.parse_args().output)
