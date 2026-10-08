from pathlib import Path
import os
import struct
import subprocess
root = Path(__file__).resolve().parents[1]
work = root / "build/test-color-packing"
work.mkdir(parents=True, exist_ok=True)
code = b"".join(struct.pack("<4I", op, 0x03E00008, 0, 0)
                for op in (0xD0598084, 0xD05A8084, 0xD05B8084))
elf = bytearray(0x80 + len(code))
elf[:16] = b"\x7fELF\x01\x01\x01" + bytes(9)
struct.pack_into("<HHIIIIIHHHHHH", elf, 16, 2, 8, 1, 0x08804000, 52, 0, 0, 52, 32, 1, 0, 0, 0)
struct.pack_into("<IIIIIIII", elf, 52, 1, 0x80, 0x08804000, 0x08804000, len(code), len(code), 5, 16)
elf[0x80:] = code
fixture = work / "colors.elf"
fixture.write_bytes(elf)
csv = work / "functions.csv"
csv.write_text("".join(f"color{i},{0x08804000+i*16:08x},c\n" for i in range(3)))
generated = work / "colors.cpp"
subprocess.run([str(root / "build/tools/psp_recomp"), str(fixture), str(csv), str(generated)], check=True)
executable = work / "test"
subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-O1", "-I", str(root / "PSPRecomp/include"),
                str(root / "tests/color_packing.cpp"), str(generated), str(root / "build/tools/libpsprecomp_core.a"),
                "-o", str(executable)], check=True)
subprocess.run([str(executable)], check=True, timeout=5)
