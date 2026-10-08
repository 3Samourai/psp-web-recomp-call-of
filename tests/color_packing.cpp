#include "psprecomp/runtime.hpp"
#include <bit>
#include <cassert>
#include <iostream>
int main() {
    using C = psprecomp::AllegrexContext;
    const std::uint32_t expected[] = {0x70F08F0Fu, 0x03E0FC1Fu, 0x07E0F81Fu};
    psprecomp::Runtime rt;
    psprecomp::register_generated_functions(rt);
    for (unsigned op = 0; op < 3; ++op) {
        rt.cpu() = C{};
        auto &ctx = rt.cpu();
        ctx.pc = 0x08804000u + op * 16;
        ctx.gpr[31] = 0x08805000u;
        ctx.vfpu_ctrl[0] = ctx.vfpu_ctrl[1] = 0xE4u;
        const std::uint32_t input[] = {0xFFFFFFFFu, 0u, 0x80FF00FFu, 0x7F00FF00u};
        for (unsigned i = 0; i < 4; ++i)
            ctx.vfpu[C::vfpu_vector_lane_index(0, 4, i)] = std::bit_cast<float>(input[i]);
        rt.run(ctx.pc, 1u);
        assert(ctx.pc == ctx.gpr[31]);
        assert(std::bit_cast<std::uint32_t>(ctx.vfpu[C::vfpu_vector_lane_index(4, 2, 0)]) == 0x0000FFFFu);
        assert(std::bit_cast<std::uint32_t>(ctx.vfpu[C::vfpu_vector_lane_index(4, 2, 1)]) == expected[op]);
        ctx.pc = 0x08804000u + op * 16;
        ctx.vfpu_ctrl[0] = 0x1Bu;
        ctx.vfpu_ctrl[2] = 1u << 8;
        ctx.vfpu[C::vfpu_vector_lane_index(4, 2, 0)] = std::bit_cast<float>(0x12345678u);
        rt.run(ctx.pc, 1u);
        assert(std::bit_cast<std::uint32_t>(ctx.vfpu[C::vfpu_vector_lane_index(4, 2, 0)]) == 0x12345678u);
        assert(std::bit_cast<std::uint32_t>(ctx.vfpu[C::vfpu_vector_lane_index(4, 2, 1)]) == 0xFFFF0000u);
    }
    std::cout << "Compiled VFPU color packing, source swizzle and destination masks: passed\n";
}
