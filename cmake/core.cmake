set(TARGET_FLOAT_ABI hard)
set(TARGET_FPU fpv5-sp-d16)
set(TARGET_CPU cortex-m33)
set(TARGET_ARCH v8-m.main+fp+dsp)
set(TARGET_TRIPLE thumbv8-m.main-none-eabihf)
set(TARGET_ARM_INSTRUCTION_MODE thumb)
set(TARGET_ENDIAN little-endian)

# the write-only guard: every write-only field of the SVD is classified (oneToSet, a key, or <!-- Kvasir: write-only
# accepted -->), so a new one stops the build; registers with no readable field are never read
svd_convert(
    core_peripherals
    SVD_FILE
    ${CMAKE_CURRENT_LIST_DIR}/../core.svd
    OUTPUT_DIRECTORY
    core_peripherals
    WRITE_ONLY_GUARD
    error
    WRITE_ONLY_REGISTERS
    derived
    WRITE_ONLY_MASK
    ON)
