#pragma once

// The Armv8-M barrier and event instructions, spelled once. Every one of them clobbers
// "memory" so the compiler cannot move an ordinary load or store across it, which is the
// whole point of a barrier: the instruction orders the hardware, the clobber orders the
// compiler.
namespace Kvasir::Core {

// Data memory barrier: every explicit memory access before it completes before any after it
// is observed. This is the ordering primitive for data shared between cores or with a DMA.
[[gnu::always_inline]] inline void dmb() { asm volatile("dmb" ::: "memory"); }

// Data synchronisation barrier: like dmb, and additionally waits for the accesses to finish.
// Needed after a register write whose side effect the next instruction depends on, such as
// enabling a coprocessor before using it.
[[gnu::always_inline]] inline void dsb() { asm volatile("dsb" ::: "memory"); }

// Instruction synchronisation barrier: flush the pipeline so instructions after it are
// fetched with the effect of whatever system register write preceded it.
[[gnu::always_inline]] inline void isb() { asm volatile("isb" ::: "memory"); }

// Send event: wakes every core sleeping in wfe(). Cheap, safe to issue spuriously.
[[gnu::always_inline]] inline void sev() { asm volatile("sev" ::: "memory"); }

// Wait for event: sleep until the event register is set, then clear it and return. Sources:
// another core's sev(), an interrupt (a pending one returns at once), an exception return
// on this core, and - on the RP2350 - any successful exclusive store on this core. The last
// one is the trap: Armv8-M raises an event whenever the global monitor leaves Exclusive
// (DDI0553 B9.3.1), and the RP2350 makes a core's own successful strex do that. Every
// std::atomic read-modify-write or compare_exchange, Atomic::Spinlock, CriticalSection and
// the 8-byte atomics (ShimLock) therefore leave the event set, and a wfe() right after them
// does not sleep. So the check in front of a wfe() loop must be a plain load or a register
// read; a check that takes a lock or does an RMW turns the loop into a busy-wait. (This is
// what pico-sdk 2.3.1 fixed under PICO_EXCLUSIVE_ACCESS_SETS_OWN_EVENT.) Always re-check the
// condition after waking: the event register can be set for any of the reasons above.
[[gnu::always_inline]] inline void wfe() { asm volatile("wfe" ::: "memory"); }

// Wait for interrupt.
[[gnu::always_inline]] inline void wfi() { asm volatile("wfi" ::: "memory"); }

}   // namespace Kvasir::Core
