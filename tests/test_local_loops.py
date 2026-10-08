"""Compile and execute real AOT loops; a timeout detects a missing yield."""
from pathlib import Path
import os
import struct
import subprocess

root = Path(__file__).resolve().parents[1]
work = root / "build" / "test-local-loops"
work.mkdir(parents=True, exist_ok=True)

for name, branch in {
    "beq": 0x1000FFFE,
    "beql": 0x5000FFFE,
    "j": 0x0A201000,
    "jal": 0x0E201000,
}.items():
    code = struct.pack("<III", 0x24420001, branch, 0x24630001)
    elf = bytearray(0x80 + len(code))
    elf[:16] = b"\x7fELF\x01\x01\x01" + bytes(9)
    struct.pack_into("<HHIIIIIHHHHHH", elf, 16, 2, 8, 1, 0x08804000,
                     52, 0, 0, 52, 32, 1, 0, 0, 0)
    struct.pack_into("<IIIIIIII", elf, 52, 1, 0x80, 0x08804000,
                     0x08804000, len(code), len(code), 5, 16)
    elf[0x80:] = code
    fixture = work / f"{name}.elf"
    fixture.write_bytes(elf)
    generated = work / name
    subprocess.run([str(root / "build/tools/psp_recomp"), str(fixture),
                    "--auto", str(generated), "0x08804000", "64"], check=True,
                   stdout=subprocess.DEVNULL)
    executable = work / f"test-{name}"
    subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-O1",
                    "-I", str(root / "PSPRecomp/include"),
                    str(root / "tests/local_loop.cpp"),
                    *map(str, generated.glob("*.cpp")),
                    str(root / "build/tools/libpsprecomp_core.a"),
                    "-o", str(executable)], check=True)
    subprocess.run([str(executable)], check=True, timeout=5)
    print(f"{name}: passed", flush=True)
