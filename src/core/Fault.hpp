#pragma once
#include "core_peripherals/SCB.hpp"
#include "kvasir/Register/Register.hpp"
#include "kvasir/Util/StaticString.hpp"

#include <cstdint>
#include <string_view>
#include <utility>

using Kvasir::Register::apply;
using Kvasir::Register::read;
using Kvasir::Register::write;

namespace Kvasir::Core::Fault {

using SCB_R = Kvasir::Peripheral::SCB::Registers<>;

struct FaultContext {
    std::uint32_t        r0;
    std::uint32_t        r1;
    std::uint32_t        r2;
    std::uint32_t        r3;
    std::uint32_t        r12;
    std::uint32_t        lr;
    std::uint32_t        pc;
    std::uint32_t        xpsr;
    std::uint32_t        exc_return;
    std::uint32_t const* stack_pointer;
};

struct FaultInfo {
    Kvasir::StaticString<64>     type;
    std::string_view             description;
    std::optional<std::uint32_t> fault_address;
    std::uint32_t                status_bits;
};

namespace detail {

    static constexpr FaultInfo analyze_usage_fault(Kvasir::StaticString<64> context,
                                                   std::uint32_t            ufsr) {
        if(ufsr & (1U << 9)) {
            return {.type          = context + "UsageFault",
                    .description   = "Division by zero",
                    .fault_address = std::nullopt,
                    .status_bits   = ufsr};
        }
        if(ufsr & (1U << 8)) {
            return {.type          = context + "UsageFault",
                    .description   = "Unaligned access",
                    .fault_address = std::nullopt,
                    .status_bits   = ufsr};
        }
        if(ufsr & (1U << 3)) {
            return {.type          = context + "UsageFault",
                    .description   = "No coprocessor",
                    .fault_address = std::nullopt,
                    .status_bits   = ufsr};
        }
        if(ufsr & (1U << 2)) {
            return {.type          = context + "UsageFault",
                    .description   = "Invalid PC load",
                    .fault_address = std::nullopt,
                    .status_bits   = ufsr};
        }
        if(ufsr & (1U << 1)) {
            return {.type          = context + "UsageFault",
                    .description   = "Invalid state",
                    .fault_address = std::nullopt,
                    .status_bits   = ufsr};
        }
        if(ufsr & (1U << 0)) {
            return {.type          = context + "UsageFault",
                    .description   = "Undefined instruction",
                    .fault_address = std::nullopt,
                    .status_bits   = ufsr};
        }
        return {.type          = context + "UsageFault",
                .description   = "Unknown",
                .fault_address = std::nullopt,
                .status_bits   = ufsr};
    }

    static constexpr FaultInfo analyze_bus_fault(Kvasir::StaticString<64> context,
                                                 std::uint32_t            bfsr,
                                                 std::uint32_t            bfar) {
        bool const address_valid = bfsr & (1U << 7);
        auto const fault_address
          = address_valid ? std::optional<std::uint32_t>{bfar} : std::nullopt;

        if(bfsr & (1U << 5)) {
            return {.type          = context + "BusFault",
                    .description   = "Lazy state preservation error",
                    .fault_address = fault_address,
                    .status_bits   = bfsr};
        }
        if(bfsr & (1U << 4)) {
            return {.type          = context + "BusFault",
                    .description   = "Exception stacking error",
                    .fault_address = fault_address,
                    .status_bits   = bfsr};
        }
        if(bfsr & (1U << 3)) {
            return {.type          = context + "BusFault",
                    .description   = "Exception unstacking error",
                    .fault_address = fault_address,
                    .status_bits   = bfsr};
        }
        if(bfsr & (1U << 2)) {
            return {.type          = context + "BusFault",
                    .description   = "Imprecise data access error",
                    .fault_address = fault_address,
                    .status_bits   = bfsr};
        }
        if(bfsr & (1U << 1)) {
            return {.type          = context + "BusFault",
                    .description   = "Precise data access error",
                    .fault_address = fault_address,
                    .status_bits   = bfsr};
        }
        if(bfsr & (1U << 0)) {
            return {.type          = context + "BusFault",
                    .description   = "Instruction bus error",
                    .fault_address = fault_address,
                    .status_bits   = bfsr};
        }
        return {.type          = context + "BusFault",
                .description   = "Unknown",
                .fault_address = fault_address,
                .status_bits   = bfsr};
    }

    static constexpr FaultInfo analyze_memmanage_fault(Kvasir::StaticString<64> context,
                                                       std::uint32_t            mmfsr,
                                                       std::uint32_t            mmfar) {
        bool const address_valid = mmfsr & (1U << 7);
        auto const fault_address
          = address_valid ? std::optional<std::uint32_t>{mmfar} : std::nullopt;

        if(mmfsr & (1U << 5)) {
            return {.type          = context + "MemManage",
                    .description   = "Lazy state preservation error",
                    .fault_address = fault_address,
                    .status_bits   = mmfsr};
        }
        if(mmfsr & (1U << 4)) {
            return {.type          = context + "MemManage",
                    .description   = "Exception stacking error",
                    .fault_address = fault_address,
                    .status_bits   = mmfsr};
        }
        if(mmfsr & (1U << 3)) {
            return {.type          = context + "MemManage",
                    .description   = "Exception unstacking error",
                    .fault_address = fault_address,
                    .status_bits   = mmfsr};
        }
        if(mmfsr & (1U << 1)) {
            return {.type          = context + "MemManage",
                    .description   = "Data access violation",
                    .fault_address = fault_address,
                    .status_bits   = mmfsr};
        }
        if(mmfsr & (1U << 0)) {
            return {.type          = context + "MemManage",
                    .description   = "Instruction access violation",
                    .fault_address = fault_address,
                    .status_bits   = mmfsr};
        }
        return {.type          = context + "MemManage",
                .description   = "Unknown",
                .fault_address = fault_address,
                .status_bits   = mmfsr};
    }

}   // namespace detail

static FaultInfo GetFaultInfo() {
    auto fault_regs = apply(read(SCB_R::CFSR::ufsr),
                            read(SCB_R::CFSR::bfsr),
                            read(SCB_R::CFSR::mmfsr),
                            read(SCB_R::HFSR::debugevt),
                            read(SCB_R::HFSR::forced),
                            read(SCB_R::HFSR::vecttbl),
                            read(SCB_R::MMFAR::address),
                            read(SCB_R::BFAR::address));

    auto ufsr  = get<0>(fault_regs);
    auto bfsr  = get<1>(fault_regs);
    auto mmfsr = get<2>(fault_regs);

    auto hfsr_debugevt = static_cast<std::uint32_t>(get<3>(fault_regs));
    auto hfsr_forced   = static_cast<std::uint32_t>(get<4>(fault_regs));
    auto hfsr_vecttbl  = static_cast<std::uint32_t>(get<5>(fault_regs));

    auto mmfar = get<6>(fault_regs);
    auto bfar  = get<7>(fault_regs);

    if(hfsr_vecttbl) {
        return {.type          = "HardFault",
                .description   = "vector table",
                .fault_address = std::nullopt,
                .status_bits   = hfsr_vecttbl};
    }

    Kvasir::StaticString<64> context{};
    if(hfsr_forced) {
        context = "Forced HardFault -> ";
    }

    if(ufsr) {
        return detail::analyze_usage_fault(context, ufsr);
    }
    if(bfsr) {
        return detail::analyze_bus_fault(context, bfsr, bfar);
    }
    if(mmfsr) {
        return detail::analyze_memmanage_fault(context, mmfsr, mmfar);
    }

    if(hfsr_forced) {
        return {.type          = "HardFault",
                .description   = "unknown escalation",
                .fault_address = std::nullopt,
                .status_bits   = ufsr | bfsr | mmfsr};
    }

    return {.type          = "HardFault",
            .description   = "unknown",
            .fault_address = std::nullopt,
            .status_bits   = hfsr_debugevt | hfsr_forced | hfsr_vecttbl};
}

static FaultContext CaptureFaultContext(std::uint32_t const* stack_ptr,
                                        std::uint32_t        lr_value) {
    FaultContext ctx{};

    ctx.r0            = stack_ptr[0];
    ctx.r1            = stack_ptr[1];
    ctx.r2            = stack_ptr[2];
    ctx.r3            = stack_ptr[3];
    ctx.r12           = stack_ptr[4];
    ctx.lr            = stack_ptr[5];
    ctx.pc            = stack_ptr[6];
    ctx.xpsr          = stack_ptr[7];
    ctx.exc_return    = lr_value;
    ctx.stack_pointer = stack_ptr;

    return ctx;
}

using EarlyInitList = decltype(MPL::list(
  // Enable fault exceptions
  write(SCB_R::SHCSR::MEMFAULTENAValC::memmanage_exception_enabled_for_the_selected_security_state),
  write(SCB_R::SHCSR::BUSFAULTENAValC::busfault_exception_enabled),
  write(
    SCB_R::SHCSR::USGFAULTENAValC::usagefault_exception_enabled_for_the_selected_security_state),

  // Enable additional fault detection
  write(SCB_R::CCR::DIV_0_TRPValC::divbyzero_usagefault_generation_enabled),
  write(SCB_R::CCR::UNALIGN_TRPValC::any_unaligned_transaction_generates_an_unaligned_usagefault),
  write(
    SCB_R::CCR::STKALIGNValC::stack_automatically_aligned_to_8_byte_boundary_on_exception_entry)));

static inline void Log(std::uint32_t const* stack_ptr,
                       std::uint32_t        lr_value) {
    [[maybe_unused]] auto const ctx        = Core::Fault::CaptureFaultContext(stack_ptr, lr_value);
    auto                        fault_info = Core::Fault::GetFaultInfo();

    fault_info.fault_address = 33;

    UC_LOG_C(
      "COREFAULT type({}) info({}) flags({:#08x}) address({:#08x}) "
      "registers: PC={:#08x} R0={:#08x} R1={:#08x} R2={:#08x} R3={:#08x} R12={:#08x} LR={:#08x} "
      "xPSR={:#08x}",
      fault_info.type,
      fault_info.description,
      fault_info.status_bits,
      fault_info.fault_address,
      ctx.pc,
      ctx.r0,
      ctx.r1,
      ctx.r2,
      ctx.r3,
      ctx.r12,
      ctx.lr,
      ctx.xpsr);
}

}   // namespace Kvasir::Core::Fault
