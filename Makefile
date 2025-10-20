# Main Makefile - delegates to target-specific Makefiles

# Default target
help:
	@echo "Silicon Labs Zigbee Smart Home Device Project"
	@echo "============================================="
	@echo ""
	@echo "This project supports both hardware (EFR32MG21) and simulation development."
	@echo ""
	@echo "Quick Start:"
	@echo "  make stub/build    - Build stub device for testing/simulation"
	@echo "  make stub/run      - Run stub device interactively"
	@echo "  make test          - Run automated tests (builds stub first)"
	@echo ""
	@echo "Silabs Development:"
	@echo "  make silabs/tools/all - Download and install Silicon Labs tools"
	@echo "  make silabs/trust     - Trust SDK signature (first time setup)"
	@echo "  make silabs/gen       - Generate project files from .slcp"
	@echo "  make silabs/build     - Build firmware for EFR32MG21"
	@echo "  make silabs/install   - Flash firmware to device"
	@echo ""
	@echo "Telink Development:"
	@echo "  make telink/tools/all - Download and install Telink tools"
	@echo "  make telink/build     - Build firmware for Telink TC32"
	@echo ""
	@echo "Stub/Simulation Development:"
	@echo "  make stub/help     - Show detailed stub commands"
	@echo "  make stub/build    - Build host-native simulation binary"
	@echo "  make stub/run      - Run stub device with REPL interface"
	@echo "  make stub/test     - Run basic stub functionality test"
	@echo "  make stub/clean    - Clean stub build and NVM data"
	@echo ""
	@echo "Testing:"
	@echo "  make test          - Run full pytest suite against stub device"
	@echo ""
	@echo "Device Management:"
	@echo "  make silabs/erase_dev   - Mass erase device flash"
	@echo "  make silabs/restart_dev - Reset device"
	@echo ""
	@echo "Bootloader:"
	@echo "  make silabs/bootloader_build   - Build bootloader"
	@echo "  make silabs/bootloader_install - Flash bootloader"
	@echo ""
	@echo "For detailed help on specific targets:"
	@echo "  make silabs/help       - Show Silicon Labs build system help"
	@echo "  make silabs/tools/help - Show Silicon Labs tools help"
	@echo "  make telink/help       - Show Telink build system help" 
	@echo "  make telink/tools/help - Show Telink tools help"
	@echo "  make stub/help         - Show stub device build system help"
	@echo ""
	@echo "Architecture Overview:"
	@echo "  • src/silabs/      - EFR32MG21 hardware build system"
	@echo "  • src/telink/      - Telink TC32 hardware build system"
	@echo "  • src/stub/        - Host simulation environment"
	@echo "  • tests/           - Python test suite for stub device"
	@echo ""

stub/%:
	$(MAKE) -C src/stub $*

silabs/%:
	$(MAKE) -C src/silabs $*

telink/%:
	$(MAKE) -C src/telink $*

# Run pytest tests (requires stub to be built)
test: stub/build
	python -m pytest tests/ -v

# Define available targets for help
.PHONY: help stub/% silabs/% telink/% test
