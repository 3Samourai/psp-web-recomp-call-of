#include "kernel.hpp"

#include <cassert>
#include <iostream>

using Runtime = psprecomp::Runtime;
using Context = psprecomp::AllegrexContext;
constexpr std::uint32_t base = 0x08804000u, worker = base + 0x100u, flag = 0x08800200u;

static void create(Runtime &rt, Context &ctx) {
    rt.memory().store32(flag, 0u);
    rt.memory().store8(flag + 16u, 'i');
    rt.memory().store8(flag + 17u, 'o');
    rt.memory().store8(flag + 18u, 0u);
    ctx.gpr[4] = flag + 16u;
    ctx.gpr[5] = worker;
    ctx.gpr[6] = 32u; // same priority as the boot thread
    ctx.gpr[7] = 0x1000u;
    ctx.gpr[8] = ctx.gpr[9] = 0u;
    ctx.gpr[31] = base + 0x10u;
    rt.invoke_import("ThreadManForUser", 0x446D8DE6u, ctx);
}

static void start(Runtime &rt, Context &ctx) {
    ctx.gpr[4] = ctx.gpr[2];
    ctx.gpr[5] = ctx.gpr[6] = 0u;
    ctx.gpr[16] = 0x12345678u;
    ctx.gpr[31] = base + 0x20u;
    rt.invoke_import("ThreadManForUser", 0xF475845Du, ctx);
}

static void spin(Runtime &rt, Context &ctx) {
    if (rt.memory().load32(flag) == 0u) return;
    assert(ctx.gpr[16] == 0x12345678u);
    rt.invoke_import("LoadExecForUser", 0x05572A5Fu, ctx);
}

static void complete(Runtime &rt, Context &ctx) {
    rt.memory().store32(flag, 1u);
    ctx.gpr[16] = 0u;
    ctx.pc = ctx.gpr[31];
}

int main() {
    Runtime rt;
    pspweb::Kernel kernel(rt);
    kernel.install({});
    rt.register_function(base, create, "timeslice_create");
    rt.register_function(base + 0x10u, start, "timeslice_start");
    rt.register_function(base + 0x20u, spin, "timeslice_spin");
    rt.register_function(worker, complete, "timeslice_io_complete");
    kernel.boot(base, 0u, "timeslice-test");
    for (int frame = 0; frame < 10 && kernel.frame(1.0); ++frame) {}
    assert(kernel.halt_reason() == "exit" && rt.memory().load32(flag) == 1u);
    std::cout << "Time slices let equal-priority I/O run and preserve thread context: passed\n";
}
