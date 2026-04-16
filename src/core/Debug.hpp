#pragma once
#include "core_peripherals/DCB.hpp"
#include "kvasir/Register/Register.hpp"

namespace Kvasir::Core::Debug {

using DCB_R = Kvasir::Peripheral::DCB::Registers<>;

// Returns true if a debugger has enabled halting debug (DHCSR.C_DEBUGEN).
// This bit is set by the debugger over SWD when it attaches; software can only read it.
[[nodiscard]] static inline bool isDebuggerConnected() {
    using Kvasir::Register::apply;
    using Kvasir::Register::read;
    return get<0>(apply(read(DCB_R::DHCSR::c_debugen))) != 0u;
}

}   // namespace Kvasir::Core::Debug
