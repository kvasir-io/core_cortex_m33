#pragma once
#include "kvasir/StartUp/LinkerSymbols.hpp"

#include <cstdint>

namespace Kvasir::Startup::Core {

static void startup() { asm("msr MSPLIM, %0" : : "r"(_LINKER_stack_start_)); }

// What a secondary core has to do for itself before any compiled code runs on it. The
// bootrom of a multicore chip hands the core over with the FPU disabled; a hard-float ABI
// will use it in the very first frame, so CPACR is written by the entry trampoline in
// assembly. The chip layer ORs its own coprocessors into this mask.
struct SecondaryCoreTraits {
    static constexpr std::uint32_t cpacrFpu
      = (3U << 20U) | (3U << 22U);   // cp10, cp11: full access
    static constexpr std::uint32_t cpacrEnable = cpacrFpu;
};

}   // namespace Kvasir::Startup::Core
