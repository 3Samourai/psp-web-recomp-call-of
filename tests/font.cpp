#include "kernel.hpp"

#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>

static std::uint32_t word(const std::uint8_t *p) {
    return p[0] | (std::uint32_t(p[1]) << 8) | (std::uint32_t(p[2]) << 16) | (std::uint32_t(p[3]) << 24);
}

int main() {
    psprecomp::Runtime rt;
    pspweb::Kernel kernel(rt);
    kernel.install({});
    auto &m = rt.memory();
    psprecomp::AllegrexContext ctx{};
    constexpr std::uint32_t error = 0x08800180u, styles = 0x08800200u;
    ctx.gpr[4] = 0x08800100u;
    ctx.gpr[5] = error;
    rt.invoke_import("sceLibFont", 0x67F17ED7u, ctx);
    const auto library = ctx.gpr[2];
    assert(library != 0u && m.load32(error) == 0u);
    ctx.gpr[4] = library;
    rt.invoke_import("sceLibFont", 0x27F6E642u, ctx);
    const auto count = ctx.gpr[2];
    assert(count == 17u);
    ctx.gpr[5] = styles;
    ctx.gpr[6] = count;
    rt.invoke_import("sceLibFont", 0xBC75D85Bu, ctx);
    assert(ctx.gpr[2] == 0u);
    std::uint32_t index = count;
    for (std::uint32_t i = 0; i < count; ++i) {
        std::string name;
        for (std::uint32_t c = 0; c < 64u && m.load8(styles + i * 168u + 96u + c); ++c)
            name.push_back(static_cast<char>(m.load8(styles + i * 168u + 96u + c)));
        if (name == "ltn8.pgf") index = i;
    }
    assert(index < count && std::bit_cast<float>(m.load32(styles + index * 168u + 4u)) == 7.0f);
    ctx.gpr[5] = index;
    ctx.gpr[6] = 0u;
    ctx.gpr[7] = error;
    rt.invoke_import("sceLibFont", 0xA834319Du, ctx);
    const auto font = ctx.gpr[2];
    assert(font != 0u && m.load32(error) == 0u);

    // Compare the HLE output to the glyph decoded from the original disc font.
    std::ifstream input("/game/fonts/ltn8.pwf", std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(input)), {});
    assert(bytes.size() >= 272u);
    std::size_t cursor = 272u;
    for (std::uint32_t i = 0; i < word(bytes.data() + 4u); ++i) {
        const auto size = word(bytes.data() + cursor + 64u);
        if (word(bytes.data() + cursor) == 'A') break;
        cursor += 68u + size;
    }
    assert(cursor + 68u <= bytes.size() && word(bytes.data() + cursor) == 'A');
    const auto *info = bytes.data() + cursor + 4u;
    const int width = word(info), height = word(info + 4u);
    assert(width > 0 && height > 0 && width < 64 && height < 64);
    ctx.gpr[4] = font;
    ctx.gpr[5] = 'A';
    ctx.gpr[6] = 0x08802000u;
    rt.invoke_import("sceLibFont", 0xDCC80C2Fu, ctx);
    assert(ctx.gpr[2] == 0u);
    for (unsigned i = 0; i < 60; ++i) assert(m.load8(ctx.gpr[6] + i) == info[i]);

    constexpr std::uint32_t image = 0x08802100u, buffer = 0x08804000u;
    for (unsigned i = 0; i < 64u * 64u; ++i) m.store8(buffer + i, 7u);
    m.store32(image, 2u); // 8-bit grayscale
    m.store32(image + 4u, 64u);
    m.store32(image + 8u, 128u);
    m.store16(image + 12u, 64u);
    m.store16(image + 14u, 64u);
    m.store16(image + 16u, 64u);
    m.store32(image + 20u, buffer);
    ctx.gpr[6] = image;
    rt.invoke_import("sceLibFont", 0x980F4895u, ctx);
    assert(ctx.gpr[2] == 0u);
    const auto *pixels = bytes.data() + cursor + 68u;
    bool ink = false;
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x) {
            const int expected = std::min(255, 7 + int(pixels[y * width + x]) * 17);
            assert(m.load8(buffer + (y + 2) * 64 + x + 1) == expected);
            ink |= expected > 7;
        }
    assert(ink && m.load8(buffer) == 7u && m.load8(buffer + 64) == 7u);
    ctx.gpr[4] = font;
    rt.invoke_import("sceLibFont", 0x3AEA8CB6u, ctx);
    ctx.gpr[5] = 0x08803000u;
    rt.invoke_import("sceLibFont", 0x0DA7535Eu, ctx);
    assert(ctx.gpr[2] == 0x80460003u);
    std::cout << "Original NewRodin font listing, glyph metrics and bitmap rendering: passed\n";
}
