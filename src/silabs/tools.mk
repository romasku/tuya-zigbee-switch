# Silicon Labs Tools Download Makefile
# This makefile provides targets to download Silicon Labs development tools

# Configuration
PROJECT_ROOT := ../..
TOOLS_DIR := $(PROJECT_ROOT)/silabs_tools
DOWNLOAD_DIR := $(TOOLS_DIR)/downloads

# Tool versions and URLs
GECKO_SDK_VERSION := 4.4.6
GECKO_SDK_REPO := https://github.com/SiliconLabs/gecko_sdk
GECKO_SDK_ARCHIVE := gecko-sdk.zip
GECKO_SDK_URL := $(GECKO_SDK_REPO)/releases/download/v$(GECKO_SDK_VERSION)/$(GECKO_SDK_ARCHIVE)

# Silicon Labs download URLs
COMMANDER_URL := https://www.silabs.com/documents/public/software/SimplicityCommander-Linux.zip
SLC_CLI_URL := https://www.silabs.com/documents/public/software/slc_cli_linux.zip

.PHONY: all clean clean-downloads help status verify trust
.PHONY: gecko_sdk commander slc-cli
.PHONY: install-gecko_sdk install-commander install-slc-cli

# Default target
all: gecko_sdk commander slc-cli
	@echo "All Silicon Labs tools have been downloaded and installed to $(TOOLS_DIR)"

# Trust SDK signature
trust:
	@echo "Trusting Silicon Labs SDK signature..."
	$(TOOLS_DIR)/slc-cli/slc signature trust --sdk $(TOOLS_DIR)/gecko_sdk

# Help target
help:
	@echo "Silicon Labs Tools Download Makefile"
	@echo ""
	@echo "Available targets:"
	@echo "  all          - Download and install all tools"
	@echo "  gecko_sdk    - Download and install Gecko SDK from GitHub"
	@echo "  commander    - Download and install Simplicity Commander"
	@echo "  slc-cli      - Download and install SLC CLI tool"
	@echo "  trust        - Trust SDK signature (run after SDK installation)"
	@echo "  status       - Show installation status and verify tools"
	@echo "  verify       - Verify installed tools and system requirements"
	@echo "  clean        - Remove installed tools (keeps downloads)"
	@echo "  clean-downloads - Remove downloaded archives"
	@echo "  help         - Show this help message"
	@echo ""
	@echo "Tools will be installed to: $(TOOLS_DIR)"
	@echo ""
	@echo "System requirements:"
	@echo "  - ARM GCC toolchain (arm-none-eabi-gcc)"
	@echo ""
	@echo "Note: Some downloads may require Silicon Labs account registration"
	@echo ""
	@echo "Quick start:"
	@echo "  make -f tools.mk all      # Download all tools"

# Create directories
$(TOOLS_DIR):
	mkdir -p $(TOOLS_DIR)

$(DOWNLOAD_DIR): | $(TOOLS_DIR)
	mkdir -p $(DOWNLOAD_DIR)

# Gecko SDK from GitHub
gecko_sdk: $(TOOLS_DIR)/gecko_sdk
	@echo "Gecko SDK installed successfully"

$(TOOLS_DIR)/gecko_sdk: | $(DOWNLOAD_DIR)
	@echo "Downloading Gecko SDK v$(GECKO_SDK_VERSION) from GitHub..."
	@if [ ! -f "$(DOWNLOAD_DIR)/$(GECKO_SDK_ARCHIVE)" ]; then \
		echo "Downloading $(GECKO_SDK_URL)"; \
		curl -L "$(GECKO_SDK_URL)" \
			-o "$(DOWNLOAD_DIR)/$(GECKO_SDK_ARCHIVE)" \
			--fail --show-error; \
	fi
	@echo "Extracting Gecko SDK..."
	@rm -rf $(TOOLS_DIR)/gecko_sdk
	@mkdir -p $(TOOLS_DIR)/gecko_sdk
	@unzip -q "$(DOWNLOAD_DIR)/$(GECKO_SDK_ARCHIVE)" -d $(TOOLS_DIR)/gecko_sdk
	@# Move files from subdirectory if needed
	@if [ -d "$(TOOLS_DIR)/gecko_sdk/gecko_sdk-$(GECKO_SDK_VERSION)" ]; then \
		mv $(TOOLS_DIR)/gecko_sdk/gecko_sdk-$(GECKO_SDK_VERSION)/* $(TOOLS_DIR)/gecko_sdk/; \
		rmdir $(TOOLS_DIR)/gecko_sdk/gecko_sdk-$(GECKO_SDK_VERSION); \
	fi
	@echo "Gecko SDK v$(GECKO_SDK_VERSION) installed to $(TOOLS_DIR)/gecko_sdk"

# Simplicity Commander
commander: $(TOOLS_DIR)/commander
	@echo "Simplicity Commander installed successfully"

$(TOOLS_DIR)/commander: | $(DOWNLOAD_DIR)
	@echo "Downloading Simplicity Commander..."
	@if [ ! -f "$(DOWNLOAD_DIR)/SimplicityCommander-Linux.zip" ]; then \
		echo "Attempting to download from: $(COMMANDER_URL)"; \
		if ! curl -L "$(COMMANDER_URL)" \
			-o "$(DOWNLOAD_DIR)/SimplicityCommander-Linux.zip" \
			--fail --show-error --connect-timeout 30; then \
			echo "Download failed. Please manually download SimplicityCommander-Linux.zip"; \
			echo "and save it as: $(DOWNLOAD_DIR)/SimplicityCommander-Linux.zip"; \
			echo "Then run 'make commander' again"; \
			exit 1; \
		fi; \
	fi
	@echo "Extracting Simplicity Commander..."
	@rm -rf $(TOOLS_DIR)/commander
	@mkdir -p $(TOOLS_DIR)/commander_temp
	@unzip -q "$(DOWNLOAD_DIR)/SimplicityCommander-Linux.zip" -d $(TOOLS_DIR)/commander_temp
	@# Detect architecture and extract appropriate tar.bz file
	@ARCH=$$(uname -m); \
	if [ "$$ARCH" = "x86_64" ]; then \
		COMMANDER_FILE=$$(find $(TOOLS_DIR)/commander_temp -name "*linux_x86_64*.tar.bz" | head -1); \
	elif [ "$$ARCH" = "aarch64" ]; then \
		COMMANDER_FILE=$$(find $(TOOLS_DIR)/commander_temp -name "*linux_aarch64*.tar.bz" | head -1); \
	elif [ "$$ARCH" = "armv7l" ]; then \
		COMMANDER_FILE=$$(find $(TOOLS_DIR)/commander_temp -name "*linux_aarch32*.tar.bz" | head -1); \
	else \
		COMMANDER_FILE=$$(find $(TOOLS_DIR)/commander_temp -name "*linux_x86_64*.tar.bz" | head -1); \
	fi; \
	if [ -n "$$COMMANDER_FILE" ]; then \
		echo "Extracting $$COMMANDER_FILE for $$ARCH"; \
		mkdir -p $(TOOLS_DIR)/commander; \
		tar -xjf "$$COMMANDER_FILE" -C $(TOOLS_DIR)/commander --strip-components=1; \
	else \
		echo "No suitable Commander archive found for architecture $$ARCH"; \
		exit 1; \
	fi
	@rm -rf $(TOOLS_DIR)/commander_temp
	@chmod +x $(TOOLS_DIR)/commander/commander* 2>/dev/null || true
	@echo "Simplicity Commander installed to $(TOOLS_DIR)/commander"

# SLC CLI
slc-cli: $(TOOLS_DIR)/slc-cli
	@echo "SLC CLI installed successfully"

$(TOOLS_DIR)/slc-cli: | $(DOWNLOAD_DIR)
	@echo "Downloading SLC CLI..."
	@if [ ! -f "$(DOWNLOAD_DIR)/slc_cli_linux.zip" ]; then \
		echo "Attempting to download from: $(SLC_CLI_URL)"; \
		if ! curl -L "$(SLC_CLI_URL)" \
			-o "$(DOWNLOAD_DIR)/slc_cli_linux.zip" \
			--fail --show-error --connect-timeout 30; then \
			echo "Download failed. Please manually download slc_cli_linux.zip"; \
			echo "and save it as: $(DOWNLOAD_DIR)/slc_cli_linux.zip"; \
			echo "Then run 'make slc-cli' again"; \
			exit 1; \
		fi; \
	fi
	@echo "Extracting SLC CLI..."
	@rm -rf $(TOOLS_DIR)/slc-cli
	@mkdir -p $(TOOLS_DIR)/slc-cli
	@unzip -q "$(DOWNLOAD_DIR)/slc_cli_linux.zip" -d $(TOOLS_DIR)/slc-cli
	@# Find and move contents from subdirectory if needed
	@if [ -d "$(TOOLS_DIR)/slc-cli/slc_cli" ]; then \
		mv $(TOOLS_DIR)/slc-cli/slc_cli/* $(TOOLS_DIR)/slc-cli/; \
		rmdir $(TOOLS_DIR)/slc-cli/slc_cli; \
	fi
	@chmod +x $(TOOLS_DIR)/slc-cli/slc $(TOOLS_DIR)/slc-cli/bin/slc-cli 2>/dev/null || true
	@echo "SLC CLI installed to $(TOOLS_DIR)/slc-cli"

# Install targets (for manual installation from downloaded archives)
install-gecko_sdk: $(TOOLS_DIR)/gecko_sdk

install-commander: $(TOOLS_DIR)/commander

install-slc-cli: $(TOOLS_DIR)/slc-cli

# Clean targets
clean:
	@echo "Removing installed tools from $(TOOLS_DIR)..."
	@rm -rf $(TOOLS_DIR)/gecko_sdk $(TOOLS_DIR)/commander $(TOOLS_DIR)/slc-cli
	@echo "Tools removed (downloads preserved)"

clean-downloads:
	@echo "Removing downloaded archives from $(DOWNLOAD_DIR)..."
	@rm -rf $(DOWNLOAD_DIR)
	@echo "Downloads removed"

# Verification targets
verify:
	@echo "Verifying installed tools..."
	@if [ -d "$(TOOLS_DIR)/gecko_sdk" ]; then \
		echo "✓ Gecko SDK: $(TOOLS_DIR)/gecko_sdk"; \
		echo "  Version info: $$(head -1 $(TOOLS_DIR)/gecko_sdk/platform/release-highlights.txt 2>/dev/null || echo 'Unknown')"; \
	else \
		echo "✗ Gecko SDK: Not installed"; \
	fi
	@if [ -f "$(TOOLS_DIR)/commander/commander-cli" ]; then \
		echo "✓ Simplicity Commander: $(TOOLS_DIR)/commander/commander-cli"; \
		echo "  Version: $$($(TOOLS_DIR)/commander/commander-cli --version 2>/dev/null | grep "Simplicity Commander" | head -1 || echo 'Unknown')"; \
	else \
		echo "✗ Simplicity Commander: Not installed"; \
	fi
	@if [ -f "$(TOOLS_DIR)/slc-cli/slc" ]; then \
		echo "✓ SLC CLI: $(TOOLS_DIR)/slc-cli/slc"; \
		echo "  Version: $$($(TOOLS_DIR)/slc-cli/slc --version 2>/dev/null || echo 'Unknown')"; \
	else \
		echo "✗ SLC CLI: Not installed"; \
	fi
	@echo ""
	@echo "System Requirements:"
	@GCC_PATH=$$(command -v arm-none-eabi-gcc 2>/dev/null); \
	if [ -n "$$GCC_PATH" ]; then \
		echo "✓ ARM GCC Toolchain: $$GCC_PATH"; \
		echo "  Version: $$(arm-none-eabi-gcc --version 2>/dev/null | head -1 || echo 'Unknown')"; \
	else \
		echo "✗ ARM GCC Toolchain: Not found (install arm-none-eabi-gcc)"; \
	fi

# Show current status
status:
	@echo "Silicon Labs Tools Status:"
	@echo "Tools directory: $(TOOLS_DIR)"
	@echo "Download directory: $(DOWNLOAD_DIR)"
	@echo ""
	@$(MAKE) -f tools.mk verify