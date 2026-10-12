#pragma once
#include "core_peripherals/FP.hpp"
#include "core_peripherals/SCB.hpp"
#include "core_peripherals/SYSTICK.hpp"
#include "kvasir/Register/Register.hpp"
#include "kvasir/StartUp/LinkerSymbols.hpp"

#include <cstdint>

namespace Kvasir::Startup::Core {

static void startup() { asm("msr MSPLIM, %0" : : "r"(_LINKER_stack_start_)); }

// What a warm reset would have done and the SDK's naked reset entry does not (kvasir/StartUp/StartUp.hpp,
// KVASIR_RESET_ENTRY_ASM), for an image not entered from a reset: SysTick stopped, PendSV/SysTick no longer pending,
// VTOR on this image's table, FAULTMASK clear, the fault status of an image before cleared, no lazy FP context
// pending. Before .data and .bss: the register layer only, no static.
[[gnu::always_inline]] inline void cleanCore() {
    using SCB_R     = Kvasir::Peripheral::SCB::Registers<>;
    using SYSTICK_R = Kvasir::Peripheral::SYSTICK::Registers<>;
    using Kvasir::Register::apply;
    using Kvasir::Register::clear;
    using Kvasir::Register::write;
    // SYST_CSR (DDI0553B.y D1.2.241): ENABLE and TICKINT off; CLKSOURCE kept, its reset value is IMPLEMENTATION
    // DEFINED and SystickClock writes it anyway
    apply(clear(SYSTICK_R::CSR::enable), clear(SYSTICK_R::CSR::tickint));
    // ICSR (D1.2.126): PENDSVCLR | PENDSTCLR as one whole-register write - never with a SET bit, which a field
    // write's read-modify-write could write back as read (UNPREDICTABLE together with its CLR bit)
    apply(
      write(SCB_R::ICSR::FULLREGISTER, SCB_R::ICSR::pendsvclr.Mask | SCB_R::ICSR::pendstclr.Mask));
    // VTOR (D1.2.272)
    apply(
      write(SCB_R::VTOR::FULLREGISTER,
            static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&_LINKER_vectors_start_))));
    // FAULTMASK = 0, a special register; PRIMASK stays set (the entry stub)
    asm volatile("cpsie f" ::: "memory");
    // CFSR, HFSR (D1.2.11, D1.2.123): write one to clear, every bit an image before left
    apply(write(SCB_R::CFSR::FULLREGISTER, 0xFFFF'FFFFU));
    apply(write(SCB_R::HFSR::FULLREGISTER, 0xFFFF'FFFFU));
#if defined(__ARM_FP)
    // FPCCR (D1.2.100): no lazy FP context pending; CONTROL.FPCA is 0 already (the entry stub)
    using FP_R = Kvasir::Peripheral::FP::Registers<>;
    apply(write(FP_R::FPCCR::LSPACTValC::lazy_state_preservation_is_not_active));
#endif
    asm volatile("dsb\n isb" ::: "memory");
}

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
