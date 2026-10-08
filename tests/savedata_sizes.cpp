#include "kernel.hpp"

#include <cassert>
#include <iostream>

int main() {
    psprecomp::Runtime rt;
    pspweb::Kernel kernel(rt);
    kernel.install({});
    constexpr std::uint32_t params = 0x08800800u, free_info = 0x08801000u, needed_info = free_info + 32u;
    auto &m = rt.memory();
    for (unsigned i = 0; i < 0x600u; i += 4u) m.store32(params + i, 0u);
    m.store32(params, 0x600u);
    m.store32(params + 0x30u, 8u);
    m.store32(params + 0x7Cu, 768u * 1024u);
    m.store32(params + 0x5D0u, free_info);
    m.store32(params + 0x5D8u, needed_info);
    psprecomp::AllegrexContext ctx{};
    ctx.gpr[4] = params;
    rt.invoke_import("sceUtility", 0x50C4CD57u, ctx);
    assert(ctx.gpr[2] == 0u && m.load32(params + 0x1Cu) == 0u);
    assert(m.load32(free_info + 8u) > 768u);
    assert(m.load32(free_info) / 1024u * m.load32(free_info + 4u) == m.load32(free_info + 8u));
    assert(m.load32(needed_info + 4u) >= 768u);
    assert(m.load32(needed_info + 4u) < m.load32(free_info + 8u));
    assert(m.load8(free_info + 19u) == 0u && m.load8(needed_info + 15u) == 0u);
    std::cout << "Savedata size query reports capacity and required storage: passed\n";
}
