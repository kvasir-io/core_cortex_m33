#pragma once

// The common exceptions come from core_cortex_common; these are the Armv8-M fault handlers
// only the Cortex-M33 has.
#include "cortex_common/CoreInterrupts.hpp"
#include "cortex_common/InterruptRules.hpp"
#include "kvasir/Mpl/Types.hpp"

namespace Kvasir {
struct CoreInterrupts : CommonCoreInterrupts {
    static constexpr Type<-12> memoryManagement{};
    static constexpr Type<-11> busFault{};
    static constexpr Type<-10> usageFault{};
    static constexpr Type<-9>  secureFault{};
};

namespace Core {
    // Armv8-M Mainline's exception rules (with the Security Extension: SecureFault is served).
    struct InterruptRules : CoreInterruptRules<ArmProfile::v8mMain> {
        using FaultInterruptIndexs = brigand::list<decltype(CoreInterrupts::hardFault),
                                                   decltype(CoreInterrupts::memoryManagement),
                                                   decltype(CoreInterrupts::busFault),
                                                   decltype(CoreInterrupts::usageFault),
                                                   decltype(CoreInterrupts::secureFault)>;
    };
}   // namespace Core
}   // namespace Kvasir
