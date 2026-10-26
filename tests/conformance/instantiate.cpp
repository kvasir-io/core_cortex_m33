// The core's register writes, instantiated once each: every dependent apply() of the core layer is checked against its
// register's write-only rules (Register::Semantics: AIRCR's key, reads with side effects) as the conformance tree
// converts the core SVD (guard error, registers derived, mask on). Nothing here is called.

// the config header first: the chip below needs its chip variant, log backend and log clock (uc_log.hpp would read it
// too, but the chip headers before it need the same)
#include "kvasir_config.hpp"

// then the paired chip and this core
#include "chip/chip.hpp"
#include "core/Fault.hpp"
#include "core/Mpu.hpp"
#include "core/StartUp.hpp"
#include "core/core.hpp"
#include "cortex_common/Debug.hpp"

#include <array>

namespace Conformance {
[[gnu::used]] static void coreWrites() {
    using Kvasir::Register::apply;
    apply(Kvasir::Nvic::makeEnable(Kvasir::Interrupt::systick));
    apply(Kvasir::Nvic::makeDisable(Kvasir::Interrupt::systick));
    apply(Kvasir::Nvic::makeSetPending(Kvasir::Interrupt::systick));
    apply(Kvasir::Nvic::makeClearPending(Kvasir::Interrupt::systick));
    apply(Kvasir::Nvic::makeSetPriority<1>(Kvasir::Interrupt::systick));
    apply(Kvasir::Nvic::makeEnable(Kvasir::Interrupt::dma_0));
    apply(Kvasir::Nvic::makeSetPriority<2>(Kvasir::Interrupt::dma_0));
    apply(Kvasir::SystemControl::SystemReset{});
    Kvasir::Core::Debug::enableCycleCounter();
    static_cast<void>(SystickClock::now());
    // the MPU mechanism (core/Mpu.hpp): every register write it makes
    Kvasir::Core::Mpu::disable();
    Kvasir::Core::Mpu::setAttributes(Kvasir::Core::Mpu::normalWriteBack);
    std::array<Kvasir::Core::Mpu::Region, 1> const regions{
      {{.begin = 0x2000'0000U, .end = 0x2000'1000U, .readOnly = false, .execute = false}}};
    static_cast<void>(Kvasir::Core::Mpu::program<8>(regions));
    Kvasir::Core::Mpu::enable(true);
    static_cast<void>(Kvasir::Core::Mpu::regionCount());
}
}   // namespace Conformance
