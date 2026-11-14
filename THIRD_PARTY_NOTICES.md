# Third-party components

The skin layouts, Lua controller, C++ collectors, and project scripts in this
repository are MIT-licensed; see [LICENSE](LICENSE).

## PresentMon (optional, downloaded separately)

- Project: <https://github.com/GameTechDev/PresentMon>
- Pinned console release: **2.6.0** (`PresentMon-2.6.0-x64.exe`).
- License: MIT, with upstream copyright and notices retained in
  `PresentMon-LICENSE.txt` when packaging with `--with-presentmon`.
- PresentMon binaries are excluded from this Git repository.

## Rainmeter and NVIDIA

Rainmeter is a separately installed host application and retains its own license.
The NVIDIA driver and its NVML library are separately installed NVIDIA software.
Neither Rainmeter nor NVML is bundled or relicensed by this repository.

## Fonts and Windows APIs

The skins reference Windows-installed Bahnschrift and Segoe UI fonts. No font
files are distributed. Windows libraries are supplied by the operating system.

## Compiler runtime

Local MinGW builds statically link the GCC runtime under its applicable licenses
and the GCC Runtime Library Exception. Compiler-runtime rights are independent
of this project's MIT license. See <https://www.gnu.org/licenses/gcc-exception-3.1.html>.
