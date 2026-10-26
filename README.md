# core_cortex_m33

The Cortex-M33 (Armv8-M Mainline, with the Security Extension) core layer: `cmake/core.cmake`, `src/core/*.hpp`
(`Fpu.hpp`), and core_cortex_common as the submodule `src/cortex_common` (its `Debug.hpp` and `CleanCore.hpp` serve
every core, included as `cortex_common/X.hpp`).

- **Priority:** 4 bits: levels 0-15.
- **Exceptions:** `CoreInterrupts` adds MemManage, BusFault, UsageFault, SecureFault; `Core::InterruptRules` =
  Armv8-M Mainline (8-10, 13 reserved, 12 DebugMonitor not served; DDI0553B.y B3.9).
- **Features** (`core/Features.hpp`): all 1 - DWT cycle counter (DDI0553B.y D1.2.59), MSPLIM/PSPLIM (D1.2.177),
  fault status registers, DHCSR readable from software (Cortex-M33 TRM 100230).
- **`Core::haltingDebug()`** (`cortex_common/Debug.hpp`): `on`/`off` from DHCSR.C_DEBUGEN (DDI0553B.y D1.2.39).
- **`Startup::Core::startup()`** (`core/StartUp.hpp`): FPU access (CPACR.CP10/CP11 full access, DDI0553B.y D1.2.14),
  then MSPLIM = the stack's lowest address (D1.2.177). The FPU is the core's; a chip opens only its own coprocessors.
- **`Startup::Core::SecondaryCoreTraits::cpacrEnable`:** CP10/CP11 full access (the FPU); a chip ORs its own
  coprocessors in.
- **`Core::Mpu`** (`core/Mpu.hpp`, the PMSAv8 MPU block of `core.svd`: MPU_TYPE/CTRL/RNR/RBAR/RLAR, the RBAR/RLAR
  aliases, MAIR0/1; DDI0553B.y D1.2.168-176): the mechanism - `Region {begin, end, readOnly, execute, attr}`,
  `encode()` (the RBAR/RLAR words), `valid()`, `program<Implemented>(regions)`, `enable(on)` / `disable()` with the
  barriers (DAI0321A 4.17), `setAttributes()`, `regionCount()` (MPU_TYPE.DREGION), `enabled()`. Which regions an image
  wants is the chip's policy (chip_rp2350 `chip/Mpu.hpp`, `Kvasir::Mpu::ImageProtection`). Host test on the register
  mock: core_cortex_common `tests/decoders/mpu_test.cpp`.
- **`core/Delay.hpp`**: `Core::spinCycles` (core_cortex_common `Delay.hpp`).
- Conformance tree: `tests/conformance` (paired with chip_rp2350).
