"""Load the native plugin and validate a real sensor payload without Rainmeter."""

import argparse
import ctypes
import math
from pathlib import Path


def smoke(path: Path) -> None:
    dll = ctypes.WinDLL(str(path.resolve()))
    state = ctypes.c_void_p()
    dll.Initialize.argtypes = [ctypes.POINTER(ctypes.c_void_p), ctypes.c_void_p]
    dll.Update.argtypes = [ctypes.c_void_p]
    dll.Update.restype = ctypes.c_double
    dll.GetString.argtypes = [ctypes.c_void_p]
    dll.GetString.restype = ctypes.c_wchar_p
    dll.Finalize.argtypes = [ctypes.c_void_p]
    dll.Initialize(ctypes.byref(state), None)
    try:
        for _ in range(3):
            usage = dll.Update(state)
            payload = dll.GetString(state)
            fields = payload.split("|")
            assert len(fields) == 13, f"Invalid telemetry field count: {payload}"
            assert math.isfinite(usage) and 0 <= usage <= 100
            assert fields[0] and fields[12], "Missing GPU/CPU names"
            for index in [1, 2, 3, 4, 5, 6, 7, 8]:
                assert math.isfinite(float(fields[index])), payload
            assert fields[10] == "0", "No state file configured: FPS should be inactive"
            print(payload)
    finally:
        dll.Finalize(state)
    print("Native sensor smoke check passed.")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--dll", type=Path, required=True)
    smoke(parser.parse_args().dll)
