"""Validate DLL exports; querying hardware requires explicit local opt-in."""

import argparse
import ctypes
import math
import sys
from pathlib import Path


def smoke(path: Path, hardware: bool = False) -> None:
    dll = ctypes.WinDLL(str(path.resolve()))
    for name in ("Initialize", "Reload", "Update", "GetString", "Finalize"):
        if not getattr(dll, name, None):
            raise RuntimeError("A required plugin export is missing")
    if not hardware:
        print("Plugin ABI check passed; no hardware queried.")
        return
    state = ctypes.c_void_p()
    dll.Initialize.argtypes = [ctypes.POINTER(ctypes.c_void_p), ctypes.c_void_p]
    dll.Update.argtypes = [ctypes.c_void_p]
    dll.Update.restype = ctypes.c_double
    dll.Reload.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.POINTER(ctypes.c_double)]
    dll.GetString.argtypes = [ctypes.c_void_p]
    dll.GetString.restype = ctypes.c_wchar_p
    dll.Finalize.argtypes = [ctypes.c_void_p]
    dll.Initialize(ctypes.byref(state), None)
    try:
        maximum = ctypes.c_double()
        dll.Reload(state, None, ctypes.byref(maximum))
        for _ in range(3):
            usage = dll.Update(state)
            payload = dll.GetString(state)
            fields = payload.split("|")
            assert len(fields) == 13, "Invalid telemetry field count"
            assert math.isfinite(usage) and 0 <= usage <= 100
            assert fields[0] and fields[12], "Missing GPU/CPU names"
            for index in [1, 2, 3, 4, 5, 6, 7, 8]:
                assert math.isfinite(float(fields[index])), "Non-finite sensor value"
            assert fields[10] == "0", "No state file configured: FPS should be inactive"
    finally:
        dll.Finalize(state)
    print("Local sensor validation passed; all hardware values suppressed.")


if __name__ == "__main__":
    sys.stdout.reconfigure(encoding="utf-8")
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--dll", type=Path, required=True)
    parser.add_argument(
        "--hardware", action="store_true", help="Query local sensors, without logging values"
    )
    args = parser.parse_args()
    smoke(args.dll, args.hardware)
