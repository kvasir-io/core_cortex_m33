#pragma once
// What this core package provides, checked against the SDK's derivation (kvasir/Port/Capabilities.hpp, the macros
// `#if` tests above the core use): a core whose facts the compiler macros do not give stops the build here.
#include "kvasir/Port/Capabilities.hpp"

static_assert(KVASIR_CORE_ARMV6M == 0);
static_assert(KVASIR_CORE_ARMV7EM == 0);
static_assert(KVASIR_CORE_ARMV8M_MAIN == 1);
static_assert(KVASIR_CORE_HAS_CYCLE_COUNTER == 1);   // DWT_CYCCNT, Armv8-M ARM DDI0553B.y D1.2.59
static_assert(
  KVASIR_CORE_HAS_DEBUGEN_READ
  == 1);   // DHCSR from software: Cortex-M33 TRM 100230 (DHCSR RW, Secure and Non-secure)
static_assert(KVASIR_CORE_HAS_STACK_LIMITS == 1);   // MSPLIM/PSPLIM, DDI0553B.y D1.2.177
static_assert(KVASIR_CORE_HAS_FAULT_STATUS == 1);   // CFSR/HFSR/MMFAR/BFAR
