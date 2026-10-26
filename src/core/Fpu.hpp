#pragma once

// The FPU's cumulative exception flags (FPSCR IOC, DZC, OFC), taken once per main-loop turn.
// The Armv8-M FPU cannot trap on them, so a NaN or an infinity that reaches a control loop is
// otherwise silent. What sets them:
//   invalid    a NaN made from numbers (0 / 0, inf - inf, sqrt(-1)), or a float converted to
//              an integer it does not fit (a negative value to unsigned, say). A quiet NaN
//              passed on through arithmetic raises nothing, and a compare with one raises it
//              only as vcmpe: gcc emits that for <, >, <=, >=, clang 23 the quiet vcmp - under
//              clang a NaN that is only compared stays silent here.
//   divByZero  x / 0 with x finite and non-zero
//   overflow   a result too large for the type
// Underflow, inexact and input-denormal are ordinary arithmetic and not counted.
//
// The thread context's FPSCR only: an exception's entry and return save and restore the
// interrupted context's FPSCR, so what an ISR raises is gone when it returns. Counts are turns
// in which a flag was seen, not operations.
//
//     using Startup = Kvasir::Startup::Startup<..., Kvasir::Fpu::ExceptionMonitor>;
//
// FPSCR IOC bit 0, DZC bit 1, OFC bit 2: Armv8-M ARM DDI0553B.y D1.2.103 (md l.79783); no
// traps: Cortex-M33 TRM 100230 B5.2.4 "Exceptions" ("The FPU does not support exception
// traps", md l.2377).

#include "kvasir/Port/Extends.hpp"

#include <cstdint>

#ifndef __ARM_FP
    #error "Kvasir::Fpu: this build has no FPU (-mfloat-abi=soft): there is no FPSCR to read"
#endif

namespace Kvasir { namespace Fpu {

    struct Exceptions {
        std::uint32_t invalid;
        std::uint32_t divByZero;
        std::uint32_t overflow;
    };

    inline constexpr std::uint32_t kIoc = 1U << 0U;
    inline constexpr std::uint32_t kDzc = 1U << 1U;
    inline constexpr std::uint32_t kOfc = 1U << 2U;

    struct ExceptionMonitor {
        static Turn turn() {
            std::uint32_t fpscr{};
            asm volatile("vmrs %0, fpscr" : "=r"(fpscr));
            auto const raised = fpscr & (kIoc | kDzc | kOfc);
            if(raised != 0U) {
                fpscr &= ~raised;
                asm volatile("vmsr fpscr, %0" : : "r"(fpscr));
                if((raised & kIoc) != 0U) { ++counts_.invalid; }
                if((raised & kDzc) != 0U) { ++counts_.divByZero; }
                if((raised & kOfc) != 0U) { ++counts_.overflow; }
            }
            return Turn::idle();
        }

        using Extends = Startup::Extend<Hook::MainLoop, &ExceptionMonitor::turn>;

        /// Turns that raised each flag, this run.
        [[nodiscard]] static Exceptions counts() { return counts_; }

    private:
        inline static Exceptions counts_{};
    };
}}   // namespace Kvasir::Fpu
