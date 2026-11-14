"""Format all native implementation and API files consistently."""

import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def sources() -> list[Path]:
    return sorted(
        path.relative_to(ROOT)
        for folder in ("src", "include", "tests")
        for path in (ROOT / folder).rglob("*")
        if path.suffix in {".cpp", ".h"}
    )


if __name__ == "__main__":
    subprocess.run(["clang-format", "-i", *map(str, sources())], cwd=ROOT, check=True)
