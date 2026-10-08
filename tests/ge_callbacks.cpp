#include "kernel.hpp"

#include <cassert>
#include <iostream>

using Runtime = psprecomp::Runtime;
using Context = psprecomp::AllegrexContext;
constexpr std::uint32_t base = 0x08804000u, callback = base + 0x100u;
constexpr std::uint32_t data = 0x08800200u, list = data + 0x40u, output = data + 0x80u;

static void setup(Runtime &rt, Context &ctx) {
    rt.memory().store32(data, 0u);
    rt.memory().store32(data + 4u, 0u);
    rt.memory().store32(data + 8u, callback);
    rt.memory().store32(data + 12u, output);
    rt.memory().store32(list, 0x0E080000u); // SIGNAL SYNC
    rt.memory().store32(list + 4u, 0x0C000000u);
    rt.memory().store32(list + 8u, 0x0F00DEADu); // barrier FINISH must not invoke the callback
    rt.memory().store32(list + 12u, 0x0C000000u);
    rt.memory().store32(list + 16u, 0x0F001234u); // final FINISH with a nonzero value
    rt.memory().store32(list + 20u, 0x0C000000u);
    ctx.gpr[28] = 0x12345678u;
    ctx.gpr[4] = data;
    ctx.gpr[31] = base + 0x10u;
    rt.invoke_import("sceGe_user", 0xA4FC06A4u, ctx);
}

static void enqueue(Runtime &rt, Context &ctx) {
    ctx.gpr[6] = ctx.gpr[2]; // callback ID returned by sceGeSetCallback
    ctx.gpr[4] = list;
    ctx.gpr[5] = 0u;
    ctx.gpr[7] = 0u;
    ctx.gpr[31] = base + 0x20u;
    rt.invoke_import("sceGe_user", 0xAB49E76Au, ctx);
}

static void start_wait(Runtime &, Context &ctx) {
    ctx.gpr[16] = 0xABCDEF01u;
    ctx.gpr[17] = 0u;
    ctx.pc = base + 0x30u;
}

static void wait_for_finish(Runtime &rt, Context &ctx) {
    ++ctx.gpr[17];
    if (rt.memory().load32(output + 4u) == 0u) return;
    // Interrupt execution must preserve the interrupted guest's registers.
    assert(ctx.gpr[16] == 0xABCDEF01u && ctx.gpr[17] != 0u);
    assert(ctx.gpr[28] == 0x12345678u);
    assert(rt.memory().load32(output) == 0x1234u);
    rt.invoke_import("LoadExecForUser", 0x05572A5Fu, ctx);
}

static void finish(Runtime &rt, Context &ctx) {
    assert(ctx.gpr[4] == 0x1234u && ctx.gpr[5] == output);
    assert(ctx.gpr[28] == 0x12345678u);
    rt.memory().store32(ctx.gpr[5], ctx.gpr[4]);
    rt.memory().store32(ctx.gpr[5] + 4u, 1u);
    ctx.gpr[16] = 0u; // would corrupt the main thread without context isolation
    ctx.pc = ctx.gpr[31];
}

int main() {
    Runtime rt;
    pspweb::Kernel kernel(rt);
    kernel.install({});
    rt.register_function(base, setup, "callback_test_setup");
    rt.register_function(base + 0x10u, enqueue, "callback_test_enqueue");
    rt.register_function(base + 0x20u, start_wait, "callback_test_start_wait");
    rt.register_function(base + 0x30u, wait_for_finish, "callback_test_wait");
    rt.register_function(callback, finish, "callback_test_finish");
    kernel.boot(base, 0u, "callback-test");
    for (int frame = 0; frame < 10 && kernel.frame(1.0); ++frame) {}
    assert(kernel.halt_reason() == "exit");
    assert(rt.memory().load32(output + 4u) == 1u);
    std::cout << "GE finish callback value, argument and interrupted context: passed\n";
}
