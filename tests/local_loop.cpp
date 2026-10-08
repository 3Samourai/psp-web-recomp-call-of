#include "psprecomp/runtime.hpp"

#include <cassert>
#include <iostream>

int main() {
    psprecomp::Runtime runtime;
    psprecomp::register_generated_functions(runtime);
    // The fixture increments v0, then branches back with a v1 increment in
    // its delay slot. It must return even though the guest loop never ends.
    runtime.run(0x08804000u, 1);
    assert(runtime.cpu().pc == 0x08804000u);
    assert(runtime.cpu().gpr[2] == 2048u && runtime.cpu().gpr[3] == 2048u);
    runtime.run(runtime.cpu().pc, 1);
    assert(runtime.cpu().gpr[2] == 4096u && runtime.cpu().gpr[3] == 4096u);
    std::cout << "local loop yields and resumes with its delay slot intact\n";
}
