#pragma once

#include "CoreInterrupts.hpp"
#include "chip/Interrupt.hpp"
#include "core_peripherals/NVIC.hpp"
#include "kvasir/Common/Interrupt.hpp"
#include "kvasir/Mpl/Utility.hpp"
#include "kvasir/Register/Register.hpp"

#include <algorithm>

namespace Kvasir { namespace Nvic {
    namespace Detail {
        using namespace Register;
        using NvicRegs = Kvasir::Peripheral::NVIC::Registers<>;

        template<typename InputIt>
        constexpr bool interuptIndexValid(int     Interrupt,
                                          InputIt f,
                                          InputIt l) {
            constexpr auto bd = std::begin(InterruptOffsetTraits<void>::disabled);
            constexpr auto ed = std::end(InterruptOffsetTraits<void>::disabled);
            return Interrupt < InterruptOffsetTraits<void>::end
                && Interrupt >= InterruptOffsetTraits<void>::begin
                && std::find(bd, ed, Interrupt) == ed && std::find(f, l, Interrupt) == l;
        }

        template<int Interrupt>
        constexpr auto get_enable_action() {
            using Reg   = NvicRegs::ISER<Interrupt / 32>;
            using Field = Reg::template SETENA<Interrupt % 32>;

            return write(Field::SETENAValC::enable_interrupt);
        }

        template<int Interrupt>
        constexpr auto get_read_enable_action() {
            using Reg   = NvicRegs::ISER<Interrupt / 32>;
            using Field = Reg::template SETENA<Interrupt % 32>;

            return read(Field::setena);
        }

        template<int Interrupt>
        constexpr auto get_disable_action() {
            using Reg   = NvicRegs::ICER<Interrupt / 32>;
            using Field = Reg::template CLRENA<Interrupt % 32>;

            return write(Field::CLRENAValC::disable_interrupt);
        }

        template<int Interrupt>
        constexpr auto get_set_pending_action() {
            using Reg   = NvicRegs::ISPR<Interrupt / 32>;
            using Field = Reg::template SETPEND<Interrupt % 32>;

            return write(Field::SETPENDValC::pend_interrupt);
        }

        template<int Interrupt>
        constexpr auto get_clear_pending_action() {
            using Reg   = NvicRegs::ICPR<Interrupt / 32>;
            using Field = Reg::template CLRPEND<Interrupt % 32>;

            return write(Field::CLRPENDValC::clear_pending_state_of_interrupt);
        }

        template<int Priority,
                 int Interrupt>
        constexpr auto get_set_priority_action() {
            using Reg   = NvicRegs::IPR<Interrupt / 4>;
            using Field = Reg::template PRI<Interrupt % 4>;

            return write(Field::pri, Register::value<Priority>());
        }

    }   // namespace Detail

    // Enable interrupt
    template<int Interrupt>
        requires(Interrupt >= 0)
    struct MakeAction<Action::Enable, Index<Interrupt>>
      : decltype(MPL::list(Detail::get_enable_action<Interrupt>())) {
        static_assert(Detail::interuptIndexValid(Interrupt,
                                                 std::begin(InterruptOffsetTraits<void>::noEnable),
                                                 std::end(InterruptOffsetTraits<void>::noEnable)),
                      "Unable to enable this interrupt, index is out of range");
    };

    // Read interrupt enabled state
    template<int Interrupt>
        requires(Interrupt >= 0)
    struct MakeAction<Action::Read, Index<Interrupt>>
      : decltype(MPL::list(Detail::get_read_enable_action<Interrupt>())) {
        static_assert(Detail::interuptIndexValid(Interrupt,
                                                 std::begin(InterruptOffsetTraits<void>::noEnable),
                                                 std::end(InterruptOffsetTraits<void>::noEnable)),
                      "Unable to read this interrupt, index is out of range");
    };

    // Disable interrupt
    template<int Interrupt>
        requires(Interrupt >= 0)
    struct MakeAction<Action::Disable, Index<Interrupt>>
      : decltype(MPL::list(Detail::get_disable_action<Interrupt>())) {
        static_assert(Detail::interuptIndexValid(Interrupt,
                                                 std::begin(InterruptOffsetTraits<void>::noDisable),
                                                 std::end(InterruptOffsetTraits<void>::noDisable)),
                      "Unable to disable this interrupt, index is out of range");
    };

    // Set interrupt pending
    template<int Interrupt>
        requires(Interrupt >= 0)
    struct MakeAction<Action::SetPending, Index<Interrupt>>
      : decltype(MPL::list(Detail::get_set_pending_action<Interrupt>())) {
        static_assert(
          Detail::interuptIndexValid(Interrupt,
                                     std::begin(InterruptOffsetTraits<void>::noSetPending),
                                     std::end(InterruptOffsetTraits<void>::noSetPending)),
          "Unable to set pending on this interrupt, index is out of range");
    };

    // Clear interrupt pending
    template<int Interrupt>
        requires(Interrupt >= 0)
    struct MakeAction<Action::ClearPending, Index<Interrupt>>
      : decltype(MPL::list(Detail::get_clear_pending_action<Interrupt>())) {
        static_assert(
          Detail::interuptIndexValid(Interrupt,
                                     std::begin(InterruptOffsetTraits<void>::noClearPending),
                                     std::end(InterruptOffsetTraits<void>::noClearPending)),
          "Unable to clear pending on this interrupt, index is out of range");
    };

    // Set Priority
    template<int Priority, int Interrupt>
        requires(Interrupt >= 0)
    struct MakeAction<Action::SetPriority<Priority>, Index<Interrupt>>
      : decltype(MPL::list(Detail::get_set_priority_action<Priority, Interrupt>())) {
        static_assert(15 >= Priority,
                      "priority on cortex_m33 can only be 0-15 (4 bits implemented)");
        static_assert(
          Detail::interuptIndexValid(Interrupt,
                                     std::begin(InterruptOffsetTraits<void>::noSetPriority),
                                     std::end(InterruptOffsetTraits<void>::noSetPriority)),
          "Unable to set priority on this interrupt, index is out of range");
    };
}}   // namespace Kvasir::Nvic

