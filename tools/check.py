"""Run repository checks after installing the locked development dependencies."""

import hashlib
import shutil
import subprocess
import sys
from pathlib import Path

import yaml
from format_cpp import sources
from privacy import check as check_privacy

ROOT = Path(__file__).resolve().parents[1]


def run(*args: str) -> None:
    # Display repository-relative arguments without exposing interpreter/install locations.
    display = [Path(args[0]).name]
    for arg in args[1:]:
        path = Path(arg)
        display.append(str(path.relative_to(ROOT)) if path.is_relative_to(ROOT) else arg)
    print("+ " + " ".join(display), flush=True)
    subprocess.run(args, cwd=ROOT, check=True)


def check() -> None:
    check_privacy()
    for path in [ROOT / ".gitlab-ci.yml", *ROOT.glob(".github/workflows/*.yml")]:
        parsed = yaml.safe_load(path.read_text(encoding="utf-8"))
        if not isinstance(parsed, dict):
            raise ValueError(f"Expected a YAML mapping: {path}")
    run("clang-format", "--dry-run", "--Werror", *map(str, sources()))
    compiler = shutil.which("luac") or shutil.which("luac5.1")
    lua_file = ROOT / "skins/PhoneMonitor/@Resources/DisplayControl.lua"
    if compiler:
        run(compiler, "-p", str(lua_file.relative_to(ROOT)))
    else:
        from lupa.lua51 import LuaRuntime

        LuaRuntime().execute("assert(loadfile(...))", str(lua_file))
        print("Lua 5.1 syntax check passed (Lupa).")
    shell = shutil.which("pwsh") or shutil.which("powershell")
    script = (
        "$failed = $false; Get-ChildItem -Recurse -Filter *.ps1 tools,skins | ForEach-Object { "
        "$tokens = $null; $errors = $null; "
        "[void][System.Management.Automation.Language.Parser]::ParseFile($_.FullName, "
        "[ref]$tokens, [ref]$errors); if ($errors) { $errors | Write-Output; $failed = $true } }; "
        "if ($failed) { exit 1 }"
    )
    if shell:
        run(shell, "-NoProfile", "-Command", script)
    elif sys.platform == "win32":
        raise RuntimeError("PowerShell not found")
    else:
        print("PowerShell syntax is checked by the Windows CI job.")
    files = [*ROOT.glob("skins/**/*.ini"), *ROOT.glob("skins/**/Measures.inc")]
    before = {p: hashlib.sha256(p.read_bytes()).digest() for p in files}
    run(sys.executable, "tools/generate_themes.py")
    after = {p: hashlib.sha256(p.read_bytes()).digest() for p in files}
    if before != after:
        raise RuntimeError("Generated skins differ; run mise run generate and review them")
    run(sys.executable, "-m", "unittest", "discover", "-s", "tests", "-v")
    print("Repository checks passed.")


if __name__ == "__main__":
    check()
