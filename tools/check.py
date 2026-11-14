"""Run repository checks; install clang-format, Lua and PowerShell first."""

import hashlib
import shutil
import subprocess
import sys
from pathlib import Path

import yaml

ROOT = Path(__file__).resolve().parents[1]


def run(*args: str) -> None:
    print("+ " + " ".join(map(str, args)), flush=True)
    subprocess.run(args, cwd=ROOT, check=True)


def check() -> None:
    for path in [ROOT / ".gitlab-ci.yml", *ROOT.glob(".github/workflows/*.yml")]:
        parsed = yaml.safe_load(path.read_text(encoding="utf-8"))
        if not isinstance(parsed, dict):
            raise ValueError(f"Expected a YAML mapping: {path}")
    paths = [*ROOT.glob("src/*.cpp"), *ROOT.glob("tests/*.cpp")]
    run("clang-format", "--dry-run", "--Werror", *map(str, paths))
    compiler = shutil.which("luac") or shutil.which("luac5.1")
    if not compiler:
        raise RuntimeError("Lua compiler not found (luac or luac5.1)")
    run(compiler, "-p", str(ROOT / "skins/PhoneMonitor/@Resources/DisplayControl.lua"))
    shell = shutil.which("pwsh") or shutil.which("powershell")
    if not shell:
        raise RuntimeError("PowerShell not found")
    script = (
        "$failed = $false; Get-ChildItem -Recurse -Filter *.ps1 tools,skins | ForEach-Object { "
        "$tokens = $null; $errors = $null; "
        "[void][System.Management.Automation.Language.Parser]::ParseFile($_.FullName, "
        "[ref]$tokens, [ref]$errors); if ($errors) { $errors | Write-Output; $failed = $true } }; "
        "if ($failed) { exit 1 }"
    )
    run(shell, "-NoProfile", "-Command", script)
    files = [*ROOT.glob("skins/**/*.ini"), *ROOT.glob("skins/**/Measures.inc")]
    before = {p: hashlib.sha256(p.read_bytes()).digest() for p in files}
    run(sys.executable, "tools/generate_themes.py")
    after = {p: hashlib.sha256(p.read_bytes()).digest() for p in files}
    if before != after:
        raise RuntimeError("Generated skins differ; regenerate and commit them")
    run(sys.executable, "-m", "unittest", "discover", "-s", "tests", "-v")
    print("Repository checks passed.")


if __name__ == "__main__":
    check()
