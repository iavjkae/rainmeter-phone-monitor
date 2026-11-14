"""Generate skins from presentation templates and checked-in palette configuration."""

import argparse
import re
import tomllib
from pathlib import Path

from skin_builder.layout import render_layout
from skin_builder.measures import render_measures

ROOT = Path(__file__).resolve().parents[1]
PALETTE_KEYS = {"Bg", "Card", "Text", "Mute", "Accent", "Soft", "Second", "Edge", "Mode"}


def generate(output: Path) -> None:
    palettes = tomllib.loads((ROOT / "config/themes.toml").read_text(encoding="utf-8"))
    for name, palette in palettes.items():
        if not re.fullmatch(r"[A-Za-z][A-Za-z0-9 -]*", name) or set(palette) != PALETTE_KEYS:
            raise ValueError("Invalid palette name or fields")
        if not all(
            re.fullmatch(r"[A-Fa-f0-9]{6}", value)
            for key, value in palette.items()
            if key != "Mode"
        ):
            raise ValueError("Palette colors must be six-digit RGB values")
        if not re.fullmatch(r"[A-Z]+", palette["Mode"]):
            raise ValueError("Invalid palette label")
    resources = output / "@Resources"
    resources.mkdir(parents=True, exist_ok=True)
    measures = render_measures(ROOT / "templates/Measures.inc")
    # Keep editable templates in UTF-8; Rainmeter's generated configs need UTF-16 for labels.
    (resources / "Measures.inc").write_text(measures, encoding="utf-16", newline="\n")
    for name, palette in palettes.items():
        (output / (name + ".ini")).write_text(
            render_layout(name, palette, palettes), encoding="utf-16", newline="\n"
        )
    print("Rainmeter configuration generated without inspecting the host.")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=ROOT / "skins/PhoneMonitor")
    generate(parser.parse_args().output)
