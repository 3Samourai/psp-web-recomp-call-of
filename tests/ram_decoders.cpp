#include "psprecomp/runtime.hpp"
#include <bit>
#include <cassert>
#include <iostream>
namespace psprecomp { void register_game_dynamic_functions(Runtime &); }
int main() {
    psprecomp::Runtime rt;
    psprecomp::register_game_dynamic_functions(rt);
    constexpr unsigned source = 0x08810000u, destination = source + 0x100u;
    const float vertex[] = {1.25f, -2.5f, 0.25f, -0.5f, 0.75f, 4.f, 5.f, 6.f};
    for (unsigned i = 0; i < 8; ++i) rt.memory().store32(source + i * 4, std::bit_cast<unsigned>(vertex[i]));
    auto &ctx = rt.cpu();
    ctx.vfpu_ctrl[0] = ctx.vfpu_ctrl[1] = 0xE4u;
    rt.memory().store32(0x08B03830u, 483u); // float UV, normal and position
    rt.memory().store32(0x08B03834u, 0u);
    ctx.gpr[4] = source;
    ctx.gpr[31] = 0x08805000u;
    rt.run(0x08BDD380u, 1u);
    assert(ctx.pc == ctx.gpr[31]);
    float values[4]{};
    ctx.read_vfpu_vector(values, 61u, 2u);
    assert(values[0] == vertex[0] && values[1] == vertex[1]);
    ctx.read_vfpu_vector(values, 62u, 3u);
    for (unsigned i = 0; i < 3; ++i) assert(values[i] == vertex[i + 2]);
    ctx.read_vfpu_vector(values, 60u, 3u);
    for (unsigned i = 0; i < 3; ++i) assert(values[i] == vertex[i + 5]);
    rt.memory().store32(0x08B03828u, 483u);
    rt.memory().store32(0x08B0382Cu, 0u);
    ctx.gpr[4] = destination;
    rt.run(0x08BDD2C0u, 1u);
    assert(ctx.pc == ctx.gpr[31]);
    for (unsigned i = 0; i < 8; ++i)
        assert(rt.memory().load32(destination + i * 4) == std::bit_cast<unsigned>(vertex[i]));

    // The game's compact vertex format must retain signed normals/positions.
    rt.memory().store16(source, 4096u); rt.memory().store16(source + 2, 8192u);
    rt.memory().store8(source + 4, 192u); rt.memory().store8(source + 5, 32u); rt.memory().store8(source + 6, 64u);
    rt.memory().store16(source + 8, 61440u); rt.memory().store16(source + 10, 2048u); rt.memory().store16(source + 12, 8192u);
    rt.memory().store32(0x08B03830u, 290u);
    ctx.gpr[4] = source;
    rt.run(0x08BDD380u, 1u);
    ctx.read_vfpu_vector(values, 61u, 2u);
    assert(values[0] == 1.f && values[1] == 2.f);
    ctx.read_vfpu_vector(values, 62u, 3u);
    assert(values[0] == -0.5f && values[1] == 0.25f && values[2] == 0.5f);
    ctx.read_vfpu_vector(values, 60u, 3u);
    assert(values[0] == -0.125f && values[1] == 0.0625f && values[2] == 0.25f);
    std::cout << "Compiled RAM vertex decoders preserve float attributes and signed compact formats: passed\n";
}
