#include "psprecomp/allegrex_context.hpp"
#include "psprecomp/decoder.hpp"

#include <array>
#include <bit>
#include <cassert>
#include <iostream>

using Context = psprecomp::AllegrexContext;

static Context input(std::array<std::uint32_t, 4> values) {
    Context ctx{};
    ctx.vfpu_ctrl[0] = ctx.vfpu_ctrl[1] = 0xE4u;
    for (unsigned i = 0; i < 4; ++i)
        ctx.vfpu[Context::vfpu_vector_lane_index(0, 4, i)] = std::bit_cast<float>(values[i]);
    return ctx;
}

static std::uint32_t lane(const Context &ctx, unsigned reg, unsigned size, unsigned index = 0) {
    return std::bit_cast<std::uint32_t>(ctx.vfpu[Context::vfpu_vector_lane_index(reg, size, index)]);
}

int main() {
    const auto instruction = psprecomp::decode_allegrex(0xD03CAC8Cu);
    assert(instruction.kind == psprecomp::OpcodeKind::Vi2x && instruction.mnemonic == "vi2uc");
    for (unsigned op = 0; op < 4; ++op)
        assert(psprecomp::decode_allegrex(0xD03C8080u | (op << 16)).kind == psprecomp::OpcodeKind::Vi2x);

    auto ctx = input({0x00000000u, 0x3FFFFFFFu, 0x7FFFFFFFu, 0xFFFFFFFFu});
    ctx.execute_vfpu_vi2x(4, 0, 4, 0);
    assert(lane(ctx, 4, 1) == 0x00FF7F00u); // unsigned: negatives clamp to zero
    assert(ctx.vfpu_ctrl[0] == 0xE4u && ctx.vfpu_ctrl[2] == 0u);

    ctx = input({0x12345678u, 0x80000000u, 0xFFFFFFFFu, 0x7FFFFFFFu});
    ctx.execute_vfpu_vi2x(4, 0, 4, 1);
    assert(lane(ctx, 4, 1) == 0x7FFF8012u); // signed: retain the high byte

    ctx = input({0x00000000u, 0x7FFFFFFFu, 0x80000000u, 0x40000000u});
    ctx.execute_vfpu_vi2x(4, 0, 4, 2);
    assert(lane(ctx, 4, 2, 0) == 0xFFFF0000u);
    assert(lane(ctx, 4, 2, 1) == 0x80000000u);

    ctx = input({0x12345678u, 0x80000000u, 0xFFFFFFFFu, 0x7FFFFFFFu});
    ctx.execute_vfpu_vi2x(4, 0, 4, 3);
    assert(lane(ctx, 4, 2, 0) == 0x80001234u);
    assert(lane(ctx, 4, 2, 1) == 0x7FFFFFFFu);

    ctx = input({0x00000000u, 0x3FFFFFFFu, 0x7FFFFFFFu, 0xFFFFFFFFu});
    ctx.vfpu_ctrl[0] = 0x1Bu; // reverse source lanes
    ctx.execute_vfpu_vi2x(0, 0, 4, 0); // overlapping source and destination
    assert(lane(ctx, 0, 1) == 0x007FFF00u);

    ctx = input({0x00000000u, 0x3FFFFFFFu, 0x7FFFFFFFu, 0xFFFFFFFFu});
    ctx.vfpu[Context::vfpu_vector_lane_index(4, 1, 0)] = std::bit_cast<float>(0x12345678u);
    ctx.vfpu_ctrl[2] = 1u << 8; // masked destination must remain unchanged
    ctx.execute_vfpu_vi2x(4, 0, 4, 0);
    assert(lane(ctx, 4, 1) == 0x12345678u);
    assert(ctx.vfpu_ctrl[2] == 0u);

    std::cout << "VFPU integer packing: passed\n";
}
