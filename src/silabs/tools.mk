# Silicon Labs Tools Download Makefile
# This makefile provides targets to download Silicon Labs development tools

# Configuration
PROJECT_ROOT := ../..
TOOLS_DIR := $(PROJECT_ROOT)/silabs_tools
DOWNLOAD_DIR := $(TOOLS_DIR)/downloads

# Tool versions and URLs
SIMPLICITY_SDK_VERSION := 2025.6.2
SIMPLICITY_SDK_REPO := https://github.com/SiliconLabs/simplicity_sdk
SIMPLICITY_SDK_ARCHIVE := simplicity-sdk-$(SIMPLICITY_SDK_VERSION).zip
SIMPLICITY_SDK_URL := $(SIMPLICITY_SDK_REPO)/releases/download/v$(SIMPLICITY_SDK_VERSION)/simplicity-sdk.zip

# Silicon Labs download URLs
COMMANDER_URL := https://www.silabs.com/documents/public/software/SimplicityCommander-Linux.zip
SLC_CLI_URL := https://www.silabs.com/documents/public/software/slc_cli_linux.zip

# ZAP tool configuration
ZAP_VERSION := 2025.10.23
ZAP_ARCHIVE := zap-linux-x64.zip
ZAP_URL := https://github.com/project-chip/zap/releases/download/v$(ZAP_VERSION)/$(ZAP_ARCHIVE)

.PHONY: all clean clean-downloads help status verify trust
.PHONY: simplicity_sdk commander slc-cli zap
.PHONY: install-simplicity_sdk install-commander install-slc-cli install-zap

# Default target
all: simplicity_sdk commander slc-cli zap trust verify
	@echo "All Silicon Labs tools have been downloaded and installed to $(TOOLS_DIR)"

# Trust SDK signature and configure ZAP
trust:
	@echo "Trusting Silicon Labs SDK signature..."
	$(TOOLS_DIR)/slc-cli/slc signature trust --sdk $(TOOLS_DIR)/simplicity_sdk

# Help target
help:
	@echo "Silicon Labs Tools Download Makefile"
	@echo ""
	@echo "Available targets:"
	@echo "  all          - Download and install all tools"
	@echo "  simplicity_sdk - Download and install Simplicity SDK from GitHub"
	@echo "  commander    - Download and install Simplicity Commander"
	@echo "  slc-cli      - Download and install SLC CLI tool"
	@echo "  zap          - Download and install ZAP (Zigbee Application Processor)"
	@echo "  trust        - Trust SDK signature and configure ZAP (run after installation)"
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

# Simplicity SDK from GitHub
simplicity_sdk: $(TOOLS_DIR)/simplicity_sdk
	@echo "Simplicity SDK installed successfully"

$(TOOLS_DIR)/simplicity_sdk: | $(DOWNLOAD_DIR)
	@echo "Downloading Simplicity SDK v$(SIMPLICITY_SDK_VERSION) from GitHub..."
	@if [ ! -f "$(DOWNLOAD_DIR)/$(SIMPLICITY_SDK_ARCHIVE)" ]; then \
		echo "Downloading $(SIMPLICITY_SDK_URL)"; \
		curl -L "$(SIMPLICITY_SDK_URL)" \
			-o "$(DOWNLOAD_DIR)/$(SIMPLICITY_SDK_ARCHIVE)" \
			--fail --show-error; \
	fi
	@echo "Extracting Simplicity SDK..."
	@rm -rf $(TOOLS_DIR)/simplicity_sdk
	@mkdir -p $(TOOLS_DIR)/simplicity_sdk
	@unzip -q "$(DOWNLOAD_DIR)/$(SIMPLICITY_SDK_ARCHIVE)" -d $(TOOLS_DIR)/simplicity_sdk
	@# Move files from subdirectory if needed
	@if [ -d "$(TOOLS_DIR)/simplicity_sdk/simplicity_sdk-$(SIMPLICITY_SDK_VERSION)" ]; then \
		mv $(TOOLS_DIR)/simplicity_sdk/simplicity_sdk-$(SIMPLICITY_SDK_VERSION)/* $(TOOLS_DIR)/simplicity_sdk/; \
		rmdir $(TOOLS_DIR)/simplicity_sdk/simplicity_sdk-$(SIMPLICITY_SDK_VERSION); \
	fi
	@echo "Simplicity SDK v$(SIMPLICITY_SDK_VERSION) installed to $(TOOLS_DIR)/simplicity_sdk"

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
		COMMANDER_FILE=$$(find $(TOOLS_DIR)/commander_temp -name "*cli*linux_x86_64*.tar.bz" | head -1); \
	elif [ "$$ARCH" = "aarch64" ]; then \
		COMMANDER_FILE=$$(find $(TOOLS_DIR)/commander_temp -name "*cli*linux_aarch64*.tar.bz" | head -1); \
	elif [ "$$ARCH" = "armv7l" ]; then \
		COMMANDER_FILE=$$(find $(TOOLS_DIR)/commander_temp -name "*cli*linux_aarch32*.tar.bz" | head -1); \
	else \
		COMMANDER_FILE=$$(find $(TOOLS_DIR)/commander_temp -name "*cli*linux_x86_64*.tar.bz" | head -1); \
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

# ZAP (Zigbee Application Processor)
zap: $(TOOLS_DIR)/zap
	@echo "ZAP installed successfully"

$(TOOLS_DIR)/zap: | $(DOWNLOAD_DIR)
	@echo "Downloading ZAP v$(ZAP_VERSION)..."
	@if [ ! -f "$(DOWNLOAD_DIR)/$(ZAP_ARCHIVE)" ]; then \
		echo "Downloading $(ZAP_URL)"; \
		curl -L "$(ZAP_URL)" \
			-o "$(DOWNLOAD_DIR)/$(ZAP_ARCHIVE)" \
			--fail --show-error; \
	fi
	@echo "Extracting ZAP..."
	@rm -rf $(TOOLS_DIR)/zap
	@mkdir -p $(TOOLS_DIR)/zap
	@unzip -q "$(DOWNLOAD_DIR)/$(ZAP_ARCHIVE)" -d $(TOOLS_DIR)/zap
	@# Make zap executable
	@chmod +x $(TOOLS_DIR)/zap/zap* 2>/dev/null || true
	@echo "ZAP v$(ZAP_VERSION) installed to $(TOOLS_DIR)/zap"
	@echo ""

# Install targets (for manual installation from downloaded archives)
install-simplicity_sdk: $(TOOLS_DIR)/simplicity_sdk

install-commander: $(TOOLS_DIR)/commander

install-slc-cli: $(TOOLS_DIR)/slc-cli

install-zap: $(TOOLS_DIR)/zap

# Clean targets
clean:
	@echo "Removing installed tools from $(TOOLS_DIR)..."
	@rm -rf $(TOOLS_DIR)/simplicity_sdk $(TOOLS_DIR)/commander $(TOOLS_DIR)/slc-cli $(TOOLS_DIR)/zap
	@echo "Tools removed (downloads preserved)"

clean-downloads:
	@echo "Removing downloaded archives from $(DOWNLOAD_DIR)..."
	@rm -rf $(DOWNLOAD_DIR)
	@echo "Downloads removed"

# Verification targets
verify:
	@echo "Verifying installed tools..."
	@if [ -d "$(TOOLS_DIR)/simplicity_sdk" ]; then \
		echo "✓ Simplicity SDK: $(TOOLS_DIR)/simplicity_sdk"; \
		echo "  Version: $$(grep '^sdk_version:' $(TOOLS_DIR)/simplicity_sdk/simplicity_sdk.slcs 2>/dev/null | sed 's/sdk_version: "\(.*\)"/\1/' || echo 'Unknown')"; \
	else \
		echo "✗ Simplicity SDK: Not installed"; \
		exit 1; \
	fi
	@if [ -f "$(TOOLS_DIR)/commander/commander-cli" ]; then \
		echo "✓ Simplicity Commander: $(TOOLS_DIR)/commander/commander-cli"; \
		echo "  Version: $$($(TOOLS_DIR)/commander/commander-cli --version 2>/dev/null | grep "Simplicity Commander" | head -1 || echo 'Unknown')"; \
	else \
		echo "✗ Simplicity Commander: Not installed"; \
		exit 1; \
	fi
	@if [ -f "$(TOOLS_DIR)/slc-cli/slc" ]; then \
		echo "✓ SLC CLI: $(TOOLS_DIR)/slc-cli/slc"; \
		echo "  Version: $$($(TOOLS_DIR)/slc-cli/slc --version 2>/dev/null || echo 'Unknown')"; \
	else \
		echo "✗ SLC CLI: Not installed"; \
		exit 1; \
	fi
	@ZAP_EXEC=$$(find $(TOOLS_DIR)/zap -name "zap" -type f -executable 2>/dev/null | head -1); \
	if [ -n "$$ZAP_EXEC" ]; then \
		echo "✓ ZAP: $$ZAP_EXEC"; \
		echo "  Version: $$($$ZAP_EXEC --version 2>/dev/null | head -1 || echo 'Unknown')"; \
	else \
		echo "✗ ZAP: Not installed"; \
		exit 1; \
	fi
	@echo ""
	@echo "System Requirements:"
	@GCC_PATH=$$(command -v arm-none-eabi-gcc 2>/dev/null); \
	if [ -n "$$GCC_PATH" ]; then \
		echo "✓ ARM GCC Toolchain: $$GCC_PATH"; \
		echo "  Version: $$(arm-none-eabi-gcc --version 2>/dev/null | head -1 || echo 'Unknown')"; \
	else \
		echo "✗ ARM GCC Toolchain: Not found (install arm-none-eabi-gcc)"; \
		exit 1; \
	fi
	@JAVA_PATH=$$(command -v java 2>/dev/null); \
	if [ -n "$$JAVA_PATH" ]; then \
		JAVA_VERSION=$$(java -version 2>&1 | head -1 | sed -n 's/.*"\([0-9]*\)\..*".*/\1/p' || echo "0"); \
		if [ "$$JAVA_VERSION" -lt 21 ]; then \
			echo "✗ Java: Version $$JAVA_VERSION found, but version >= 21 is required"; \
			exit 1; \
		else \
			echo "✓ Java: $$JAVA_PATH (Version >= 21)"; \
		fi; \
	else \
		echo "✗ Java: Not found (install Java >= 21)"; \
		exit 1; \
	fi

# Show current status
status:
	@echo "Silicon Labs Tools Status:"
	@echo "Tools directory: $(TOOLS_DIR)"
	@echo "Download directory: $(DOWNLOAD_DIR)"
	@echo ""
	@$(MAKE) -f $(lastword $(MAKEFILE_LIST)) verify