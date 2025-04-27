# keep first
board_runner_args(stm32cubeprogrammer "--port=swd" "--reset-mode=hw")
board_runner_args(jlink "--device=STM32F407IG" "--speed=4000")
board_runner_args(openocd
        "--cmd-pre-init=source [find interface/cmsis-dap.cfg]"
        "--cmd-pre-init=transport select swd"
        "--cmd-pre-init=source [find target/stm32f4x.cfg]"
)

# keep first
include(${ZEPHYR_BASE}/boards/common/stm32cubeprogrammer.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)