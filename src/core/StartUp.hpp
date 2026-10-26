#pragma once
// The core's start: startup() here, cleanCore() and SecondaryCoreTraits from core_cortex_common (one body for every
// core, chosen by the capability macros).
#include "core/Barrier.hpp"
#include "core_peripherals/SCB.hpp"
#include "cortex_common/CleanCore.hpp"
#include "cortex_common/SecondaryCoreTraits.hpp"
#include "kvasir/Port/LinkerSymbols.hpp"
#include "kvasir/Register/Register.hpp"

#include <cstdint>

namespace Kvasir::Startup::Core {

// First the FPU: CPACR.CP10/CP11 reset to an UNKNOWN value on a warm reset (DDI0553B.y D1.2.14, md l.69515-69521), and
// this core is built for the hard-float ABI, so the first frame that spills an FP register would fault with NOCP. The
// FPU is the core's: a chip opens only its own coprocessors (the RP2350's cp4, cp7) and leaves these two as they are;
// this write is a read-modify-write of CP10/CP11 only, so it never closes a coprocessor the boot ROM or the chip opened.
// CP11 gets CP10's value ("If the value of this field is not programmed to the same value as the CP10 field, then the
// value is UNKNOWN", md l.69517). The write is complete after the DSB and in effect after the ISB (B7.2.16 RSJQJ,
// RNPDJ, md l.10750-10754).
// Then the main stack's limit: MSPLIM = the stack's lowest address (DDI0553B.y D1.2.177), so an overflow is a STKOF
// UsageFault instead of a write below the stack.
inline void startup() {
    using CPACR = Kvasir::Peripheral::SCB::Registers<>::CPACR;
    apply(write(CPACR::CP10ValC::full_access_to_the_fp_extension),
          write(CPACR::cp11, Kvasir::Register::value<std::uint32_t, 3U>()));
    Kvasir::Core::dsb();
    Kvasir::Core::isb();
    asm("msr MSPLIM, %0" : : "r"(_LINKER_stack_start_));
}

}   // namespace Kvasir::Startup::Core
