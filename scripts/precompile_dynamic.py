"""Ahead-of-time compilation of this title's finite RAM vertex decoders.

Runs the two translated code builders at build time, then passes their output
through PSPRecomp. All generated game code remains in ignored directories.
"""
from pathlib import Path
import hashlib
import os
import re
import subprocess
import sys

root = Path(__file__).resolve().parents[1]
module, output = map(Path, sys.argv[1:3])
if hashlib.sha256(module.read_bytes()).hexdigest() != "4e27c698ad2b2354cd2800b88408833c619d163c554035e619ebd8e78a41c95a":
    sys.exit(0)
work = root / "build/cod-dynamic"
work.mkdir(parents=True, exist_ok=True)
unit = next(p for p in output.glob("generated_unit_*.cpp") if "L_089F6F3C:" in p.read_text())
text = unit.read_text()
body = text[text.index("L_089F6F3C:"):text.index("L_089F75F0:")]
body = re.sub(r"    local_pc = jump_target;\n    if .*LOCAL_DISPATCH; }\n", "", body)
builder = """#define main psprecomp_codegen_main
#include \"../../PSPRecomp/tools/codegen_main.cpp\"
#undef main
#include \"psprecomp/runtime.hpp\"
using namespace psprecomp;
void build_decoder(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    std::uint32_t jump_target = 0u, local_transfers = 0u;
    if (ctx.pc == 0x089F6F3Cu) goto L_089F6F3C;
    goto L_089F7224;
""" + body + "}\n" + r'''
int main(int argc, char **argv) {
    Runtime rt;
    auto elf = Elf32Image::from_file(argv[1]);
    (void)elf.load_and_relocate(rt.memory());
    const std::filesystem::path output(argv[2]);
    std::ostringstream registry;
    registry << "#include \"psprecomp/runtime.hpp\"\nnamespace psprecomp {\n";
    const std::string preamble = "#include \"psprecomp/runtime.hpp\"\n#include <bit>\n#include <cmath>\n#include <cstdint>\n#include <limits>\nnamespace psprecomp {\n";
    unsigned total = 0;
    for (unsigned mode = 0; mode < 2; ++mode) {
        const unsigned entry = mode == 0 ? 0x08BDD380u : 0x08BDD2C0u;
        const unsigned global = mode == 0 ? 0x08B03830u : 0x08B03828u;
        std::map<std::vector<unsigned>, unsigned> unique;
        std::map<unsigned, unsigned> keys;
        std::string source = preamble;
        unsigned part = 0;
        auto save = [&] {
            write_text_if_changed(output / ("dynamic_" + std::to_string(mode) + "_" + std::to_string(part++) + ".cpp"), source + "}\n");
            source = preamble;
        };
        for (unsigned format = 0; format < 512; ++format) {
            const unsigned color = (format >> 2) & 7, position = (format >> 7) & 3;
            if (position == 0 || (color != 0 && color < 4)) continue;
            for (unsigned index = 0; index < 4; ++index) {
                rt.memory().store32(global, ~0u);
                rt.memory().store32(global + 4, ~0u);
                AllegrexContext ctx;
                ctx.gpr[29] = 0x09FF0000u;
                ctx.pc = mode == 0 ? 0x089F6F3Cu : 0x089F7224u;
                ctx.gpr[mode == 0 ? 4 : 5] = format;
                ctx.gpr[mode == 0 ? 5 : 6] = index;
                build_decoder(rt, ctx);
                std::vector<unsigned> words;
                bool ended = false;
                for (unsigned offset = 0; offset < 768; offset += 4) {
                    const auto word = rt.memory().load32(entry + offset);
                    words.push_back(word);
                    if (word == 0x03E00008u) {
                        words.push_back(rt.memory().load32(entry + offset + 4));
                        ended = true; break;
                    }
                }
                if (!ended) throw Error("Vertex decoder missing return");
                auto [it, inserted] = unique.emplace(words, unique.size());
                keys[format * 4 + index] = it->second;
                if (!inserted) continue;
                const std::string name = "cod_decoder_" + std::to_string(mode) + "_" + std::to_string(it->second);
                registry << "void " << name << "(Runtime &, AllegrexContext &);\n";
                GeneratedFunctionInput function{name, entry, {}, {}};
                for (unsigned offset = 0; offset < words.size() * 4;) {
                    function.instructions.insert(entry + offset);
                    function.entry_labels.insert(entry + offset);
                    const auto decoded = decode_allegrex(words[offset / 4]);
                    offset += decoded.has_delay_slot() ? 8 : 4;
                }
                source += lower_constant_fpr_accesses(lower_constant_gpr_writes(emit_function_source(function, rt.memory(), name)));
                if (unique.size() % 32 == 0) save();
            }
        }
        if (source != preamble) save();
        registry << "static void cod_dispatch_" << mode << "(Runtime &rt, AllegrexContext &ctx) {\n"
                 << "unsigned key = (rt.memory().load32(" << global << "u) & 511u) * 4u + (rt.memory().load32(" << global + 4 << "u) & 3u);\nswitch (key) {\n";
        for (const auto &[key, id] : keys)
            registry << "case " << key << "u: cod_decoder_" << mode << "_" << id << "(rt, ctx); return;\n";
        registry << "default: rt.unsupported(ctx.pc, rt.memory().load32(ctx.pc), \"Uncompiled vertex format\"); return;\n}}\n";
        std::cout << "RAM vertex decoder " << mode << ": " << keys.size() << " formats, " << unique.size() << " unique routines\n";
        total += unique.size();
    }
    registry << "void register_game_dynamic_functions(Runtime &rt) {\n"
             << "rt.register_function(0x08BDD380u, cod_dispatch_0, \"cod_vertex_decoder\");\n"
             << "rt.register_function(0x08BDD2C0u, cod_dispatch_1, \"cod_vertex_decoder_indexed\");\n}\n}\n";
    write_text_if_changed(output / "dynamic_registry.cpp", registry.str());
}
'''
source = work / "builder.cpp"
source.write_text(builder)
executable = work / "builder"
subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-O1", "-I", str(root / "PSPRecomp/include"),
                str(source), str(root / "build/tools/libpsprecomp_core.a"), "-o", str(executable)], check=True)
subprocess.run([str(executable), str(module.resolve()), str(output.resolve())], check=True, timeout=60)
registry = output / "generated_registry.cpp"
text = registry.read_text().replace("void register_game_dynamic_functions(Runtime &runtime);\n", "").replace("    register_game_dynamic_functions(runtime);\n", "")
text = text.replace("void register_generated_functions(Runtime &runtime) {",
    "void register_game_dynamic_functions(Runtime &runtime);\nvoid register_generated_functions(Runtime &runtime) {\n    register_game_dynamic_functions(runtime);")
if text != registry.read_text():
    registry.write_text(text)
