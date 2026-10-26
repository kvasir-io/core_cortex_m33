#pragma once

// The Armv8-M PMSAv8 MPU, the mechanism: a region as bounds and permissions, its two register words, programming a set
// of regions and switching the MPU on and off with the barriers the architecture asks for. Which regions an image wants
// is a policy of the chip or the firmware (chip_rp2350 chip/Mpu.hpp: Kvasir::Mpu::ImageProtection).
//
// The registers are core.svd's MPU block (Armv8-M ARM DDI0553B.y D1.2.168-176). Privileged code only; the MPU of the
// Security state this runs in (Secure on the RP2350, the reset state).

#include "core/Barrier.hpp"
#include "core_peripherals/MPU.hpp"
#include "kvasir/Register/Register.hpp"

#include <cstddef>
#include <cstdint>

namespace Kvasir::Core::Mpu {
using Regs = Kvasir::Peripheral::MPU::Registers<>;

/// [begin, end): both 32-byte aligned (an MPU region's granule: RBAR.BASE and RLAR.LIMIT are bits 31:5); begin == end
/// is no region. `attr`: the MPU_MAIR0/1 attribute index (RLAR.AttrIndx).
struct Region {
    std::uint32_t begin;
    std::uint32_t end;
    bool          readOnly;
    bool          execute;
    std::uint8_t  attr{0};
};

struct Words {
    std::uint32_t rbar;
    std::uint32_t rlar;
};

/// MPU_RBAR: BASE = begin >> 5, SH 0b00 (non-shareable), AP 0b10 (read-only) or 0b00 (read/write), privileged only,
/// XN = !execute (D1.2.171, md l.86191). MPU_RLAR: LIMIT = (end - 1) >> 5 (the last 32-byte block), AttrIndx, EN 1
/// (D1.2.173, md l.86401).
constexpr Words encode(Region const& r) {
    using Kvasir::Register::deposit;
    return {.rbar = deposit(Regs::RBAR::base, r.begin >> 5U) | deposit(Regs::RBAR::sh, 0U)
                  | deposit(Regs::RBAR::ap, r.readOnly ? 0b10U : 0U)
                  | deposit(Regs::RBAR::xn, r.execute ? 0U : 1U),
            .rlar = deposit(Regs::RLAR::limit, (r.end - 1U) >> 5U)
                  | deposit(Regs::RLAR::attrindx, r.attr) | deposit(Regs::RLAR::en, 1U)};
}

constexpr bool valid(Region const& r) {
    return (r.begin & 0x1FU) == 0 && (r.end & 0x1FU) == 0 && r.begin <= r.end && r.attr < 8U;
}

/// Normal memory, write-back non-transient, read and write allocate, inner and outer (0b1111 each half: MAIR_ATTR,
/// DDI0553B.y D1.2.165) - what the system address map gives SRAM; unaligned accesses stay legal.
inline constexpr std::uint32_t normalWriteBack = 0xFFU;

/// The regions this MPU has (MPU_TYPE.DREGION, IMPLEMENTATION DEFINED; D1.2.176, md l.86690).
[[nodiscard]] inline unsigned regionCount() {
    return static_cast<unsigned>(get<0>(apply(read(Regs::TYPE::dregion))));
}

[[nodiscard]] inline bool enabled() { return apply(read(Regs::CTRL::enable)) != 0U; }

/// ENABLE 0, and HFNMIENA and PRIVDEFENA with it (PRIVDEFENA is ignored while ENABLE is 0, D1.2.168 md l.85950); a
/// dmb first, so the accesses before it complete under the old setting (DAI0321A 4.17, md l.1818).
inline void disable() {
    Kvasir::Core::dmb();
    apply(clear(Regs::CTRL::enable, Regs::CTRL::hfnmiena, Regs::CTRL::privdefena));
}

/// Attribute 0 = `attr0`, attributes 1..3 0 (Device-nGnRnE: unused by `program`'s regions unless named).
inline void setAttributes(std::uint32_t attr0) {
    apply(write(Regs::MAIR0::attr0, attr0),
          write(Regs::MAIR0::attr1, Kvasir::Register::value<std::uint32_t, 0U>()),
          write(Regs::MAIR0::attr2, Kvasir::Register::value<std::uint32_t, 0U>()),
          write(Regs::MAIR0::attr3, Kvasir::Register::value<std::uint32_t, 0U>()));
}

struct Programmed {
    std::uint8_t regions;   // regions written and enabled, from 0
    std::uint8_t invalid;   // regions refused (misaligned bounds): left out
};

/// Writes `regions` into region numbers 0, 1, ... (an empty one is skipped, an invalid one counted and left out) and
/// disables every other of the `Implemented` regions (RLAR.EN 0). Call with the MPU disabled.
template<unsigned Implemented,
         typename Regions>
Programmed program(Regions const& regions) {
    std::uint32_t n       = 0;
    std::uint8_t  invalid = 0;
    for(Region const& r : regions) {
        if(!valid(r)) {
            ++invalid;
            continue;
        }
        if(r.begin == r.end) { continue; }
        auto const w = encode(r);   // every field of both: whole-word stores
        apply(write(Regs::RNR::region, n));
        apply(write(Regs::RBAR::FULLREGISTER, w.rbar));
        apply(write(Regs::RLAR::FULLREGISTER, w.rlar));
        ++n;
    }
    auto const used = static_cast<std::uint8_t>(n);
    for(; n < Implemented; ++n) {
        apply(write(Regs::RNR::region, n));
        apply(clear(Regs::RLAR::en));
    }
    return {.regions = used, .invalid = invalid};
}

/// ENABLE 1 with the system address map behind the regions for privileged code (PRIVDEFENA) and the MPU off for
/// HardFault, NMI and FAULTMASK (HFNMIENA 0); or ENABLE 0. A dmb before, a dsb and an isb after: the accesses before it
/// complete under the old setting, the new one holds for the next instruction fetch (Arm AN DAI0321A 4.17 "MPU
/// programming", md l.1818-1820).
inline void enable(bool on) {
    Kvasir::Core::dmb();
    apply(set(Regs::CTRL::privdefena),
          clear(Regs::CTRL::hfnmiena),
          write(Regs::CTRL::enable, on ? 1U : 0U));
    Kvasir::Core::dsb();
    Kvasir::Core::isb();
}
}   // namespace Kvasir::Core::Mpu
