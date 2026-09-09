#pragma once

#include "SystemControl.hpp"
#include "core_peripherals/SCB.hpp"
#include "core_peripherals/SYSTICK.hpp"
#include "kvasir/Atomic/Atomic.hpp"
#include "kvasir/Common/Interrupt.hpp"
#include "kvasir/Register/Register.hpp"
#include "kvasir/Register/Utility.hpp"
#include "kvasir/StartUp/Resources.hpp"
#include "kvasir/Util/attributes.hpp"

#include <atomic>
#include <chrono>
#include <type_traits>

namespace Kvasir {
namespace Systick {
    using SystickRegs                       = Kvasir::Peripheral::SYSTICK::Registers<>;
    static constexpr auto useExternalClock  = SystickRegs::CSR::CLKSOURCEValC::external;
    static constexpr auto useProcessorClock = SystickRegs::CSR::CLKSOURCEValC::processor;
}   // namespace Systick

namespace Nvic {
    using SystickRegs = Kvasir::Peripheral::SYSTICK::Registers<>;

    template<>
    struct MakeAction<Action::Enable, Index<Kvasir::Interrupt::systick.index()>>
      : decltype(write(SystickRegs::CSR::TICKINTValC::interrupt_enabled)) {
        static_assert(Detail::interuptIndexValid(Interrupt::systick.index(),
                                                 std::begin(InterruptOffsetTraits<void>::noEnable),
                                                 std::end(InterruptOffsetTraits<void>::noEnable)),
                      "Unable to enable this interrupt, index is out of range");
    };

    template<>
    struct MakeAction<Action::Disable, Index<Kvasir::Interrupt::systick.index()>>
      : decltype(write(SystickRegs::CSR::TICKINTValC::interrupt_disabled)) {
        static_assert(Detail::interuptIndexValid(Interrupt::systick.index(),
                                                 std::begin(InterruptOffsetTraits<void>::noDisable),
                                                 std::end(InterruptOffsetTraits<void>::noDisable)),
                      "Unable to disable this interrupt, index is out of range");
    };
    template<>
    struct MakeAction<Action::Read, Index<Kvasir::Interrupt::systick.index()>>
      : decltype(read(SystickRegs::CSR::tickint)){};
}   // namespace Nvic

// What an init step enables (kvasir/StartUp/Resources.hpp): TICKINT written to one is the
// SysTick exception.
namespace Startup {
    template<unsigned Value>
        requires(Value != 0)
    struct InterruptOfAction<
      Register::Action<std::remove_cvref_t<decltype(Nvic::SystickRegs::CSR::tickint)>,
                       Register::WriteLiteralAction<Value>>> {
        using type = brigand::list<std::integral_constant<int, Kvasir::Interrupt::systick.index()>>;
    };
}   // namespace Startup

namespace Nvic {

    template<int Priority>
    struct MakeAction<Action::SetPriority<Priority>, Index<Kvasir::Interrupt::systick.index()>>
      : decltype(write(Kvasir::Peripheral::SCB::Registers<>::SHPR3::pri_15,
                       Register::value<Priority>())) {
        static_assert(15 >= Priority,
                      "priority on cortex_m33 can only be 0-15 (4 bits implemented)");
        static_assert(
          Detail::interuptIndexValid(Interrupt::systick.index(),
                                     std::begin(InterruptOffsetTraits<void>::noSetPriority),
                                     std::end(InterruptOffsetTraits<void>::noSetPriority)),
          "Unable to set priority on this interrupt, index is out of range");
    };
}   // namespace Nvic

namespace Systick {
    template<typename TConfig>
    struct SystickClockBase {
    private:
        // needed config
        // clockSpeed
        // clockBase
        // minOverrunTime
        using Config                              = TConfig;
        static constexpr std::uint64_t ClockSpeed = Config::clockSpeed;
        using Regs                                = Kvasir::Peripheral::SYSTICK::Registers<>;

    public:
        // Startup: on the processor clock this claims it at Config::clockSpeed, so a config
        // number that is not what the clock settings provide is a build error.
        using Claims
          = std::conditional_t<std::is_same_v<std::remove_cvref_t<decltype(Config::clockBase)>,
                                              std::remove_cvref_t<decltype(useProcessorClock)>>,
                               brigand::list<Startup::ProcessorClock<Config::clockSpeed>>,
                               brigand::list<>>;

        // chrono interface
        using duration
          = std::chrono::duration<std::int64_t,
                                  std::ratio<1, Config::clockSpeed>>;   // std::chrono::nanoseconds;
        using rep        = typename duration::rep;
        using period     = typename duration::period;
        using time_point = std::chrono::time_point<SystickClockBase, duration>;

        static constexpr bool is_steady = true;

        template<typename Rep,
                 typename Period>
        friend constexpr std::enable_if_t<!std::is_same_v<std::chrono::duration<Rep,
                                                                                Period>,
                                                          duration>,
                                          time_point>
        operator+(time_point                    t,
                  std::chrono::duration<Rep,
                                        Period> d) {
            return t + std::chrono::duration_cast<duration>(d);
        }

        template<typename Rep,
                 typename Period>
        friend constexpr std::enable_if_t<!std::is_same_v<std::chrono::duration<Rep,
                                                                                Period>,
                                                          duration>,
                                          time_point>
        operator-(time_point                    t,
                  std::chrono::duration<Rep,
                                        Period> d) {
            return t - std::chrono::duration_cast<duration>(d);
        }

    private:
        static_assert(Config::clockSpeed
                        < std::numeric_limits<std::uint64_t>::max() / 100'000'000ULL,
                      "ClockSpeed to high");

        template<std::uint64_t OverRunValue, typename = void>
        struct GetOverrunType {
            using type = std::uint64_t;
        };

        template<std::uint64_t OverRunValue>
        struct GetOverrunType<
          OverRunValue,
          std::enable_if_t<(OverRunValue <= std::numeric_limits<std::uint32_t>::max())>> {
            using type = std::uint32_t;
        };

        template<std::uint64_t OverRunValue>
        using GetOverrunTypeT = typename GetOverrunType<OverRunValue, void>::type;

        static constexpr std::uint32_t calcReloadValue(std::uint64_t clockSpeed) {
            (void)clockSpeed;
            return (1U << 24U) - 1U;
        }

        static constexpr std::uint64_t calcOverRunValue(std::uint64_t            clockSpeed,
                                                        std::chrono::nanoseconds overrunTime) {
            std::uint64_t NanoSecPerOverrun
              = ((std::uint64_t(calcReloadValue(clockSpeed)) + 1ULL) * 1'000'000'000ULL)
              / clockSpeed;
            return (std::uint64_t(overrunTime.count()) + NanoSecPerOverrun - 1ULL)
                 / NanoSecPerOverrun;
        }

        using overrunT = GetOverrunTypeT<calcOverRunValue(ClockSpeed, Config::minOverrunTime)>;
        static inline std::atomic<overrunT> overruns{};

        // A synchronised clock (Config::synchronised == true) carries an epoch offset so it
        // can read the same as a reference clock on another core: SysTick is per core, and
        // the two cores' counters start at different instants. Set once by syncTo() on the
        // owning core; a plain member, since after that it is only ever read.
        static constexpr bool Synchronised = [] {
            if constexpr(requires { Config::synchronised; }) {
                return static_cast<bool>(Config::synchronised);
            } else {
                return false;
            }
        }();

        static inline rep epoch_{};

        [[KVASIR_NO_SANITIZE_UNSIGNED_OVERFLOW]] static duration rawNow() {
            static constexpr auto reloadValue = calcReloadValue(ClockSpeed);

            std::uint32_t currentCount{};
            overrunT      localOverruns;

            while(true) {
                currentCount  = apply(read(Regs::CVR::current));
                localOverruns = overruns.load(std::memory_order_relaxed);
                if(!fieldEquals(Regs::CSR::COUNTFLAGValC::timer_has_counted_to_0)) { break; }
            }
            auto const cnd = duration{reloadValue - currentCount};
            auto const ovd = duration{static_cast<std::uint64_t>(localOverruns)
                                      * static_cast<std::uint64_t>(reloadValue + 1)};
            return cnd + ovd;
        }

        static void onIsr() {
            overrunT old = overruns.load(std::memory_order_relaxed);
            ++old;
            overruns.store(old, std::memory_order_relaxed);
        }

        static void delay_ticks(std::uint32_t ticksToWait) {
            std::uint32_t const countStart    = apply(read(Regs::CVR::current));
            overrunT const      overrunsStart = overruns.load(std::memory_order_relaxed);
            while(true) {
                std::uint32_t const countNow    = apply(read(Regs::CVR::current));
                overrunT const      overrunsNow = overruns.load(std::memory_order_relaxed);
                auto const          countsRaw   = std::int32_t(countStart - countNow);
                std::uint32_t const countsElapsed
                  = countsRaw >= 0 ? std::uint32_t(countsRaw)
                                   : countStart + (calcReloadValue(ClockSpeed) - countNow);
                if(countsElapsed >= ticksToWait || overrunsNow - overrunsStart >= 2
                   || (countNow < countStart && overrunsNow - overrunsStart == 1))
                {
                    break;
                }
            }
        }

    public:
        [[KVASIR_NO_SANITIZE_UNSIGNED_OVERFLOW]] static time_point now() {
            if constexpr(Synchronised) {
                return time_point{rawNow() + duration{epoch_}};
            } else {
                return time_point{rawNow()};
            }
        }

        // The counter without the epoch: what syncTo(referenceAt, rawAt) pairs a reference
        // reading with. Only meaningful on the core that owns this SysTick.
        static duration raw() { return rawNow(); }

        // Make now() read `referenceAt` where raw() read `rawAt`. Only on the core that
        // owns this SysTick, and only for a synchronised config. The epoch is 64 bits,
        // i.e. two stores on a 32-bit core, so it is written with interrupts masked: after
        // syncTo() returns, now() from an ISR on this core is safe.
        static void syncTo(duration referenceAt,
                           duration rawAt) {
            static_assert(Synchronised, "syncTo() needs Config::synchronised = true");
            bool const enabled = Nvic::disable_all_and_get_old_state();
            epoch_             = referenceAt.count() - rawAt.count();
            if(enabled) { Nvic::enable_all(); }
        }

        // Make now() read `referenceNow` from this instant on.
        static void syncTo(duration referenceNow) { syncTo(referenceNow, rawNow()); }

        template<typename Duration,
                 typename duration::rep value>
        static void delay() {
            static constexpr auto reloadValue = calcReloadValue(ClockSpeed);
            static constexpr auto ticksToWait
              = std::chrono::duration_cast<duration>(Duration{value}).count();
            if constexpr(ticksToWait >= reloadValue) {
                static constexpr auto count
                  = std::uint32_t(double(ticksToWait) / double(reloadValue));
                static constexpr auto last
                  = std::uint32_t(double(ticksToWait) - (double(count) * double(reloadValue)));
                std::uint32_t c = count;
                while(c != 0) {
                    delay_ticks(reloadValue);
                    --c;
                }
                delay_ticks(last);

            } else {
                delay_ticks(ticksToWait);
            }
        }

        // kvasir init
        static constexpr auto initStepPeripheryConfig
          = list(write(Config::clockBase),
                 write(Regs::RVR::reload, Register::value<calcReloadValue(ClockSpeed)>()),
                 write(Regs::CSR::ENABLEValC::counter_is_disabled),
                 makeDisable(Interrupt::systick),
                 write(Regs::CVR::current, Register::value<0>()));

        static constexpr auto initStepInterruptConfig
          = list(action(Nvic::Action::setPriority0, Interrupt::systick),
                 action(Nvic::Action::clearPending, Interrupt::systick));

        static constexpr auto initStepPeripheryEnable
          = list(write(Regs::CSR::ENABLEValC::counter_is_operating),
                 makeEnable(Interrupt::systick));

        static constexpr Nvic::Isr<std::addressof(onIsr),
                                   std::decay_t<decltype(Interrupt::systick)>>
          isr{};
    };
}   // namespace Systick
}   // namespace Kvasir
