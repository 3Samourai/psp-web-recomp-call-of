"""Link the real web kernel into a small Node/WebAssembly callback test."""
from pathlib import Path
import os
import shutil
import subprocess

root = Path(__file__).resolve().parents[1]
build = root / "build/web-cod-roads-to-victory"
objects = [str(path) for path in (build / "profiles/web/CMakeFiles/pspweb.dir/host").glob("*.cpp.o")
           if path.name != "main.cpp.o"]
assert objects, "Build the web profile first."
compiler = shutil.which("em++")
assert compiler, "Source tools/emsdk/emsdk_env.sh first."
for source in ("ge_callbacks", "savedata_sizes", "font", "timeslice", "interrupts", "ram_decoders", "controller"):
    output = root / f"build/test-{source}.js"
    preload = ["--preload-file", str(root / "games/cod-roads-to-victory/preload/fonts") + "@/game/fonts"] if source == "font" else []
    generated = [str(build / "profiles/web/libpspweb_generated.a")] if source == "ram_decoders" else []
    subprocess.run([compiler, "-std=c++20", "-O1", "-pthread", "-fwasm-exceptions",
                "-I", str(root / "PSPRecomp/include"), "-I", str(root / "profile/host"),
                str(root / f"tests/{source}.cpp"), *objects, *generated,
                str(build / "libpsprecomp_core.a"), str(build / "profiles/web/libpspweb_at3.a"),
                "-sENVIRONMENT=node", "-sEXIT_RUNTIME=1", "-sALLOW_MEMORY_GROWTH=1",
                "-sINITIAL_MEMORY=134217728", "-sSTACK_SIZE=8388608",
                "-sOFFSCREENCANVAS_SUPPORT=1", "-sMIN_WEBGL_VERSION=2", "-sMAX_WEBGL_VERSION=2",
                *preload, "-o", str(output)], check=True)
    subprocess.run([os.environ["EMSDK_NODE"], str(output)], check=True, timeout=15, cwd=output.parent)
