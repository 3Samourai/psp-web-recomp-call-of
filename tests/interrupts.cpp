#include "kernel.hpp"
#include <cassert>
#include <iostream>

using Runtime = psprecomp::Runtime;
using Context = psprecomp::AllegrexContext;
constexpr std::uint32_t base = 0x08804000u, handler = base + 0x100u;
constexpr std::uint32_t count = 0x08800200u;

static void idle(Runtime &, Context &ctx) {
    assert(ctx.gpr[16] == 0xABCDEF01u);
    assert(ctx.gpr[28] == 0x12345678u);
}
static void start(Runtime &, Context &ctx) {
    ctx.gpr[16] = 0xABCDEF01u;
    ctx.pc = base + 0x10u;
}
static void interrupt(Runtime &rt, Context &ctx) {
    assert(ctx.gpr[4] == 3u && ctx.gpr[5] == count);
    assert(ctx.gpr[28] == 0x12345678u);
    rt.memory().store32(count, rt.memory().load32(count) + 1u);
    ctx.gpr[16] = ctx.gpr[28] = 0u;
    ctx.pc = ctx.gpr[31];
}
int main() {
    Runtime rt;
    pspweb::Kernel kernel(rt);
    kernel.install({});
    rt.register_function(base, start, "interrupt_start");
    rt.register_function(base + 0x10u, idle, "interrupt_idle");
    rt.register_function(handler, interrupt, "interrupt_handler");
    Context call;
    call.gpr[4] = 30u; call.gpr[5] = 3u;
    call.gpr[6] = handler; call.gpr[7] = count;
    call.gpr[28] = 0x12345678u;
    rt.invoke_import("InterruptManager", 0xCA04A2B9u, call);
    assert(call.gpr[2] == 0u);
    rt.invoke_import("InterruptManager", 0xFB8E22ECu, call);
    kernel.boot(base, 0x12345678u, "interrupt-test");
    rt.invoke_import("Kernel_Library", 0x092968F4u, call);
    assert(call.gpr[2] == 1u);
    rt.invoke_import("Kernel_Library", 0x092968F4u, call);
    assert(call.gpr[2] == 0u);
    for (int i = 0; i < 3; ++i) assert(kernel.frame(1.0));
    assert(rt.memory().load32(count) == 0u);
    call.gpr[4] = 0u;
    rt.invoke_import("Kernel_Library", 0x5F10D406u, call);
    assert(kernel.frame(1.0) && rt.memory().load32(count) == 0u);
    call.gpr[4] = 1u;
    rt.invoke_import("Kernel_Library", 0x3B84732Du, call);
    assert(kernel.frame(1.0) && rt.memory().load32(count) == 1u);
    assert(kernel.frame(1.0) && rt.memory().load32(count) == 2u);
    call.gpr[4] = 30u; call.gpr[5] = 3u;
    rt.invoke_import("InterruptManager", 0xD61E6961u, call);
    assert(kernel.frame(1.0) && rt.memory().load32(count) == 2u);
    std::cout << "Vblank delivery, nested masking, disabling and register isolation: passed\n";
}
