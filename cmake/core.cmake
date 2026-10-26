# Kvasir's write semantics (svd_convert's defaults): a new unclassified write-only field stops the build
kvasir_chip_svd(core_peripherals ${CMAKE_CURRENT_LIST_DIR}/../core.svd)

# the core's part of the contract (Kvasir_SDK cmake/kvasir_chip.cmake): the TARGET_* variables the toolchain files read,
# and kvasir::core (these headers, the registers above)
kvasir_core(
    CPU
    cortex-m33
    ARCH
    v8-m.main+fp+dsp
    FPU
    fpv5-sp-d16
    FLOAT_ABI
    hard
    TRIPLE
    thumbv8-m.main-none-eabihf
    INSTRUCTION_MODE
    thumb
    ENDIAN
    little-endian
    INCLUDE_DIR
    ${CMAKE_CURRENT_LIST_DIR}/../src
    REGISTERS
    core_peripherals)
