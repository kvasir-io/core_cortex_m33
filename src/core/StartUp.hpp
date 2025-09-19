#pragma once
extern "C" {
extern void _LINKER_stack_start_();
}

namespace Kvasir::Startup::Core {

static void startup() { asm("msr MSPLIM, %0" : : "r"(_LINKER_stack_start_)); }

}   // namespace Kvasir::Startup::Core
