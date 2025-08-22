# One Framework - Copilot Instructions

## Repository Overview

**One Framework** is a RoboMaster embedded framework developed by Dalian Minzu University C·One team. It's a "one-click" solution built on top of Zephyr RTOS that provides hardware abstraction, modular architecture, and test-driven development for robotics competitions.

**Key Technologies:**
- **Language**: C++20 with Zephyr C APIs
- **RTOS**: Zephyr (real-time operating system)
- **Build System**: West (Zephyr meta-tool) + CMake
- **Testing**: ZTest framework with QEMU simulation
- **Hardware**: STM32-based boards (DJI Board C: ARM Cortex, 192KB RAM, 1MB Flash)
- **Dependencies**: tl::expected library, OneMotor module

**Project Size**: ~31 C++/header files, modular architecture with applications, drivers, and utilities.

## Build Instructions

**CRITICAL**: This project requires Zephyr toolchain and West tool. Always follow these exact steps:

### Prerequisites
1. **Zephyr Environment**: Install following [Zephyr Getting Started Guide](https://docs.zephyrproject.org/latest/develop/getting_started/index.html)
2. **Python 3.11+** with virtual environment
3. **ARM Zephyr EABI toolchain**

### Setup Workspace (One-time)
```bash
# Activate Zephyr environment first (example):
# source ~/zephyrproject/.venv/bin/activate  # Linux/macOS
# .\zephyrproject\.venv\Scripts\Activate.ps1  # Windows

west init -m https://github.com/RoboMaster-DLMU-CONE/one-framework --mr main zephyr_workspace
cd zephyr_workspace
west update
```

### Build Commands
```bash
# Navigate to framework directory
cd one-framework

# Build for DJI Board C with infantry configuration
west build -b dji_board_c app -- -DDTC_OVERLAY_FILE=boards/cone/infantry.overlay

# Build with debug configuration (adds debug.conf)
west build -b dji_board_c app -- -DDTC_OVERLAY_FILE=boards/cone/infantry.overlay -DOVERLAY_CONFIG=debug.conf

# Clean build (if needed)
west build -t clean

# Flash to hardware
west flash

# Flash with OpenOCD + DAP-Link
west flash --openocd

# Debug
west debug
```

### Test Execution
```bash
# Run all tests with Twister
west twister -T tests -v --inline-logs --integration

# Run specific test
west twister -T tests/lib/applications/Unit -v

# Run tests on emulator (faster)
west twister -T tests --platform qemu_cortex_m3 -v

# Build app tests (validates build configuration)
west twister -T app -v --inline-logs --integration
```

**Integration Platforms**: 
- Primary: `qemu_cortex_m3` (for unit tests)
- Hardware: `dji_board_c` (for integration tests)

**IMPORTANT**: Tests run on `qemu_cortex_m3` virtual platform and take ~10 seconds setup time.

## Codebase Layout

### Architecture Overview
```
one-framework/
├── app/                    # Main application entry point
│   ├── src/main.cpp       # Application main function
│   ├── prj.conf          # Project configuration
│   └── boards/cone/       # Board-specific overlays
├── lib/                   # Framework core libraries
│   ├── applications/      # High-level application modules
│   │   ├── Unit/         # Unit system (threading, lifecycle)
│   │   └── PRTS/         # Print/Debug system
│   ├── modules/          # Functional modules
│   └── utilities/        # Utility libraries
├── drivers/              # Hardware drivers
│   ├── input/           # Input drivers (DBUS, etc.)
│   ├── output/          # Output drivers (Buzzer, Motors)
│   ├── sensor/          # Sensor drivers (PWM heater, etc.)
│   └── utils/           # Utility drivers (Status LEDs)
├── tests/               # Unit tests (mirrors lib/ structure)
├── include/OF/          # Public header files
├── boards/              # Board definitions
├── dts/                # Device tree includes
└── .github/workflows/   # CI/CD pipelines
```

### Key Configuration Files
- **west.yml**: West manifest defining dependencies (Zephyr, OneMotor)
- **Kconfig**: Framework configuration options
- **prj.conf**: Project-level Zephyr configuration
- **CMakeLists.txt**: Build configuration
- **.clang-format**: Code formatting (LLVM-based, 120 char limit)

### Critical Dependencies
- **Device Tree System**: Hardware described in `.dts` and `.overlay` files
  - `app/boards/cone/infantry.overlay`: Infantry robot configuration
  - `boards/dji/dji_board_c/`: DJI Board C definition (CAN, PWM, GPIO, USART support)
- **Kconfig System**: Feature selection via `CONFIG_*` options
  - `CONFIG_ONE_FRAMEWORK=y`: Enables framework core
  - `CONFIG_UNIT=y`, `CONFIG_PRTS=y`: Application modules
- **Linker Sections**: Custom sections for unit registry in `lib/applications/Unit/linker/`
- **West Workspace**: Must be initialized as Zephyr workspace, not standalone git repo

## Validation Steps

### Pre-commit Checks
The CI pipeline runs these steps - replicate locally:
1. **Build Test**: `west twister -T app -v --inline-logs --integration`
2. **Unit Tests**: `west twister -T tests -v --inline-logs --integration`  
3. **Multi-platform**: Tests run on Ubuntu, macOS, Windows

### Code Quality
- **Format**: Use `.clang-format` (LLVM style, 4-space indent)
- **Standards**: C++20 features enabled, constexpr optimization encouraged
- **Testing**: Write ZTest unit tests for new functionality

### Common Issues
- **West not found**: Ensure Zephyr environment is activated before any commands
- **Build timeouts**: Use `timeout: 300` for build commands, tests may take 5-10 minutes
- **Missing dependencies**: Run `west update` if OneMotor or Zephyr modules missing
- **Test failures**: Check `qemu_cortex_m3` platform availability, ensure CONFIG_ZTEST=y
- **Wrong directory**: Commands must run in `one-framework/` directory within west workspace
- **Device tree errors**: Verify `.overlay` files reference correct board peripherals
- **Windows builds**: May require `--short-build-path -O/tmp/twister-out` flags

## Framework Concepts

### Unit System
- **Unit.hpp**: Base class for threaded application components
- **UnitRegistry.hpp**: Global unit management and lifecycle
- **Tests**: `tests/lib/applications/Unit/` demonstrates usage

### Device Abstraction
- **DTS-based**: Hardware configuration through device tree
- **Zephyr Drivers**: Standard driver API (device_is_ready, sensor_sample_get)
- **Status LEDs**: Example driver in `drivers/utils/status-leds/`

### Configuration Philosophy
- **Modular**: Use `CONFIG_*` flags to enable/disable features
- **Board-specific**: Override configs in board files
- **Test-driven**: Write tests first, implement minimal code

Trust these instructions for efficient development. Only search for additional information if instructions are incomplete or outdated.