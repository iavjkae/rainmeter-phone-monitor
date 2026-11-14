"""Reject private assets and machine-specific data from the reviewable source tree."""

import re
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MEDIA = {".png", ".jpg", ".jpeg", ".bmp", ".webp", ".gif", ".mp4", ".mp3"}
LOCAL_SUFFIXES = {
    ".log",
    ".tmp",
    ".dmp",
    ".exe",
    ".dll",
    ".obj",
    ".o",
    ".pdb",
    ".lib",
    ".a",
    ".rmskin",
}
LOCAL_NAMES = {"fps-state.txt", "rainmeter.ini", "mise.local.toml", ".mise.local.toml", ".env"}
LOCAL_DIRECTORIES = {"__cmake_systeminformation", "local", ".venv", "build", "dist"}
PATTERNS = (
    re.compile(r"(?<![A-Za-z0-9_])[A-Za-z]:[\\/]"),
    re.compile(r"/(?:home|Users)/[^/\s]+/"),
    re.compile(r"(?:glpat-|gh[pousr]_)[A-Za-z0-9_-]{16,}"),
    re.compile(r"-----BEGIN (?:OPENSSH |RSA |EC )?PRIVATE KEY-----"),
    re.compile(r"\bGeForce\s+(?:RTX|GTX)\s*\d{3,}\b", re.I),
    re.compile(r"\bRyzen\s+\d\s+\d{3,}\w*\b", re.I),
)


def candidates() -> list[Path]:
    result = subprocess.run(
        ["git", "ls-files", "--cached", "--others", "--exclude-standard", "-z"],
        cwd=ROOT,
        capture_output=True,
        check=True,
    )
    return sorted({ROOT / name for name in result.stdout.decode("utf-8").split("\0") if name})


def check() -> None:
    violations = []
    for path in candidates():
        if not path.is_file():
            continue
        relative = path.relative_to(ROOT)
        # Check paths before decoding. Forced tracking must not bypass .gitignore,
        # and diagnostics can contain private data even without a recognizable signature.
        parts = {part.lower() for part in relative.parts[:-1]}
        name = path.name.lower()
        if (
            path.suffix.lower() in MEDIA | LOCAL_SUFFIXES
            or name in LOCAL_NAMES
            or ".local." in name
            or name.startswith(".env.")
            or parts & LOCAL_DIRECTORIES
            or any(part.startswith("build-") for part in parts)
        ):
            violations.append(
                f"{relative}: private asset, local configuration, or runtime artifact"
            )
            continue
        data = path.read_bytes()
        text = data.decode("utf-16" if data.startswith(b"\xff\xfe") else "utf-8")
        if any(pattern.search(text) for pattern in PATTERNS):
            violations.append(f"{relative}: possible machine identity or credential")
    if violations:
        # Report source locations only; never echo the matched private values.
        raise ValueError("Privacy review failed:\n" + "\n".join(violations))
    print("Source privacy checks passed; no media, local overrides, or identified machine data.")


if __name__ == "__main__":
    check()
