#pragma once

// The common exceptions come from core_cortex_common; these are the Armv8-M fault handlers
// only the Cortex-M33 has.
#include "cortex_common/CoreInterrupts.hpp"

namespace Kvasir {
struct CoreInterrupts : CommonCoreInterrupts {
    static constexpr Type<-12> memoryManagement{};
    static constexpr Type<-11> busFault{};
    static constexpr Type<-10> usageFault{};
    static constexpr Type<-9>  secureFault{};
};
}   // namespace Kvasir
