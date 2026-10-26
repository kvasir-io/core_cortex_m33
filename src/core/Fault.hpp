#pragma once
// The core's part of the fault module: its decode table (from the SVD's field masks), the snapshot of its fault
// registers, the fault status clear and the fault enables. The shared v7-M/v8-M body is cortex_common/FaultV7m.hpp,
// the decode and the COREFAULT line are core_cortex_common's (FaultDecode.hpp, FaultLog.hpp); here only what is this
// core's.
#include "core_peripherals/SAU.hpp"
#include "core_peripherals/SCB.hpp"
#include "cortex_common/FaultDecode.hpp"
#include "cortex_common/FaultLog.hpp"
#include "cortex_common/FaultSnapshot.hpp"
#include "cortex_common/FaultV7m.hpp"
#include "kvasir/Mpl/Types.hpp"
#include "kvasir/Register/Register.hpp"

#include <array>
#include <cstdint>

// Kvasir::Fault::Core, next to the SDK's Kvasir::Fault (FaultHandler.hpp): its log module is
// "fault.core", derived from the scope.
namespace Kvasir::Fault::Core {
using ScbR = Kvasir::Peripheral::SCB::Registers<>;

// The decode table (cortex_common/FaultV7m.hpp) with the Armv8-M row first: STKOF, the stack pointer crossed
// MSPLIM/PSPLIM. First because an overflow usually drags a second flag along with it, and the overflow is the one to
// report.
inline constexpr std::array<FaultRow, 1> stackLimitRows{
  {{ScbR::UFSR::stkof.mask, FaultDescription::stackOverflow}}};
inline constexpr auto faultTable = V7m::faultTable<ScbR>(stackLimitRows);
static_assert(faultTable.usage.size() == 7);

// The SVD against the architecture (Armv8-M ARM DDI0553B.y: UFSR D1.2.270, BFSR D1.2.7, MMFSR D1.2.167, HFSR
// D1.2.123; in CFSR coordinates: MMFSR [7:0], BFSR [15:8], UFSR [31:16]): a wrong mask in the SVD stops the build here.
static_assert(ScbR::CFSR::ufsr.mask == 0xFFFF'0000U && ScbR::CFSR::bfsr.mask == 0x0000'FF00U
              && ScbR::CFSR::mmfsr.mask == 0x0000'00FFU);
static_assert(ScbR::UFSR::stkof.mask == 1U << (16 + 4));
static_assert(ScbR::UFSR::divbyzero.mask == 1U << (16 + 9)
              && ScbR::UFSR::unaligned.mask == 1U << (16 + 8)
              && ScbR::UFSR::nocp.mask == 1U << (16 + 3) && ScbR::UFSR::invpc.mask == 1U << (16 + 2)
              && ScbR::UFSR::invstate.mask == 1U << (16 + 1)
              && ScbR::UFSR::undefinstr.mask == 1U << 16);
static_assert(ScbR::BFSR::bfarvalid.mask == 1U << (8 + 7)
              && ScbR::BFSR::lsperr.mask == 1U << (8 + 5)
              && ScbR::BFSR::stkerr.mask == 1U << (8 + 4)
              && ScbR::BFSR::unstkerr.mask == 1U << (8 + 3)
              && ScbR::BFSR::impreciserr.mask == 1U << (8 + 2)
              && ScbR::BFSR::preciserr.mask == 1U << (8 + 1)
              && ScbR::BFSR::ibuserr.mask == 1U << 8);
static_assert(ScbR::MMFSR::mmarvalid.mask == 1U << 7 && ScbR::MMFSR::mlsperr.mask == 1U << 5
              && ScbR::MMFSR::mstkerr.mask == 1U << 4 && ScbR::MMFSR::munstkerr.mask == 1U << 3
              && ScbR::MMFSR::daccviol.mask == 1U << 1 && ScbR::MMFSR::iaccviol.mask == 1U << 0);
static_assert(ScbR::HFSR::debugevt.mask == 1U << 31 && ScbR::HFSR::forced.mask == 1U << 30
              && ScbR::HFSR::vecttbl.mask == 1U << 1);

// The fault registers for the SDK's full fault record (Kvasir::CrashRecord::Full, FaultHandler.hpp) and the
// COREFAULT line: no analysis, no log, nothing that can fault.
// Addresses from the Armv8-M ARM DDI0553B.y SCB and SAU tables (md l.67166-67263).
// The SAU's two words (DDI0553B.y SAU table): RAZ when the code runs Non-secure.
struct SauRegs {
    static constexpr bool present = true;

    static std::uint32_t sfsr() {
        return Kvasir::Register::readWhole(
          Kvasir::Peripheral::SAU::Registers<>::SFSR::FULLREGISTER);
    }

    static std::uint32_t sfar() {
        return Kvasir::Register::readWhole(
          Kvasir::Peripheral::SAU::Registers<>::SFAR::FULLREGISTER);
    }
};

inline Snapshot snapshot() { return V7m::snapshotOf<ScbR, SauRegs>(); }

// CFSR and HFSR are write-one-to-clear (DDI0553B.y D1.2.11 CFSR, D1.2.7 BFSR, D1.2.167 MMFSR, D1.2.123 HFSR):
// writing back what was read clears this fault's bits, MMFARVALID (the SVD's MMARVALID)/BFARVALID with them, and
// keeps a bit a nested fault set meanwhile. Whole-register writes: a field write would read the register again
// first. For a fault handler that returns, and a start without a core reset, where the next fault would otherwise
// find the old bits OR'ed into its own.
inline void clearFaultStatus(std::uint32_t cfsr,
                             std::uint32_t hfsr) {
    V7m::clearFaultStatus<ScbR>(cfsr, hfsr);
}

inline void clearFaultStatus() { V7m::clearFaultStatus<ScbR>(); }

using EarlyInitList = decltype(MPL::list(
  // Enable fault exceptions
  write(ScbR::SHCSR::MEMFAULTENAValC::memmanage_exception_enabled_for_the_selected_security_state),
  write(ScbR::SHCSR::BUSFAULTENAValC::busfault_exception_enabled),
  write(ScbR::SHCSR::SECUREFAULTENAValC::securefault_exception_enabled),
  write(ScbR::SHCSR::USGFAULTENAValC::usagefault_exception_enabled_for_the_selected_security_state),
  // Enable additional fault detection
  write(ScbR::CCR::DIV_0_TRPValC::divbyzero_usagefault_generation_enabled)));

// The COREFAULT line of one fault: the decode of the snapshot the fault handler took (no second register read).
inline void log(std::uint32_t const* frame,
                std::uint32_t        excReturn,
                Snapshot const&      s) {
    logCoreFault(frame, excReturn, decode(s, faultTable));
}

}   // namespace Kvasir::Fault::Core
