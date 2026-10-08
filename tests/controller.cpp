#include "kernel.hpp"
#include <cassert>
#include <iostream>
using Runtime = psprecomp::Runtime;
using Context = psprecomp::AllegrexContext;
constexpr unsigned base=0x08804000u, sample=0x08800200u, reads=sample+32;
static void read(Runtime &rt, Context &ctx) {
    ctx.gpr[4]=sample; ctx.gpr[5]=1; ctx.gpr[31]=base+16;
    rt.invoke_import("sceCtrl",0x1F803938u,ctx);
}
static void count(Runtime &rt, Context &ctx) {
    assert(ctx.gpr[2]==1);
    unsigned n=rt.memory().load32(reads)+1;
    assert(rt.memory().load32(sample+4)==(n==1 ? 0x4000u : 0u));
    assert(rt.memory().load8(sample+8)==128 && rt.memory().load8(sample+9)==128);
    rt.memory().store32(reads,n);
    if(n==3) rt.invoke_import("LoadExecForUser",0x05572A5Fu,ctx);
    else ctx.pc=base;
}
int main() {
    Runtime rt; pspweb::Kernel kernel(rt); kernel.install({});
    rt.register_function(base,read,"read_pad"); rt.register_function(base+16,count,"count_pad");
    kernel.boot(base,0,"controller-test"); kernel.set_pad(0x4000u);
    assert(kernel.frame(1.0) && rt.memory().load32(reads)==1);
    kernel.set_pad(0);
    assert(kernel.frame(1.0) && rt.memory().load32(reads)==2);
    assert(!kernel.frame(1.0) && rt.memory().load32(reads)==3);
    assert(kernel.halt_reason()=="exit");
    std::cout << "Blocking controller reads wait for fresh samples and observe button release: passed\n";
}
