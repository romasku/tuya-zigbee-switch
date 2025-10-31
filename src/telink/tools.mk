# Telink Tools Download Makefile
# This makefile provides targets to download Telink development tools

# Configuration
PROJECT_ROOT := ../..
TOOLS_DIR := $(PROJECT_ROOT)/telink_tools
DOWNLOAD_DIR := $(TOOLS_DIR)/downloads

# Tool versions and URLs
SDK_VERSION := 3.7.2.0
SDK_REPO := https://github.com/telink-semi/telink_zigbee_sdk
SDK_ARCHIVE := V$(SDK_VERSION).zip
SDK_URL := $(SDK_REPO)/archive/refs/tags/$(SDK_ARCHIVE)

# Telink toolchain URL
TOOLCHAIN_URL := https://shyboy.oss-cn-shenzhen.aliyuncs.com/readonly/tc32_gcc_v2.0.tar.bz2
TOOLCHAIN_CHECKSUM := 33b854be3e3db3dba4b4dacdda2cd4ea1c94dfd4d562864a095956de7991b430
TOOLCHAIN_ARCHIVE := tc32_gcc_v2.0.tar.bz2

# TlsrPgm programmer tool URL
TLSRPGM_URL := https://raw.githubusercontent.com/pvvx/TLSRPGM/refs/heads/main/TlsrPgm.py
TLSRPGM_FILE := TlsrPgm.py

.PHONY: all clean clean-downloads help status verify
.PHONY: sdk toolchain tlsrpgm install
.PHONY: install-sdk install-toolchain install-tlsrpgm clean-sdk clean-toolchain clean-tlsrpgm

# Default target
all: sdk toolchain tlsrpgm
	@echo "All Telink tools have been downloaded and installed to $(TOOLS_DIR)"

# Help target
help:
	@echo "Telink Tools Download Makefile"
	@echo ""
	@echo "Available targets:"
	@echo "  all          - Download and install all tools"
	@echo "  sdk          - Download and install Telink Zigbee SDK"
	@echo "  toolchain    - Download and install TC32 GCC toolchain"
	@echo "  tlsrpgm      - Download TlsrPgm flashing tool"
	@echo "  install      - Alias for 'all'"
	@echo "  status       - Show installation status and verify tools"
	@echo "  verify       - Verify installed tools"
	@echo "  clean        - Remove installed tools (keeps downloads)"
	@echo "  clean-downloads - Remove downloaded archives"
	@echo "  clean-sdk    - Remove only SDK"
	@echo "  clean-toolchain - Remove only toolchain"
	@echo "  clean-tlsrpgm - Remove only TlsrPgm tool"
	@echo "  help         - Show this help message"
	@echo ""
	@echo "Tools will be installed to: $(TOOLS_DIR)"
	@echo ""
	@echo "Quick start:"
	@echo "  make -f tools.mk all      # Download all tools"

# Create directories
$(TOOLS_DIR):
	mkdir -p $(TOOLS_DIR)

$(DOWNLOAD_DIR): | $(TOOLS_DIR)
	mkdir -p $(DOWNLOAD_DIR)

# Telink Zigbee SDK
sdk: $(TOOLS_DIR)/sdk
	@echo "Telink Zigbee SDK installed successfully"

$(TOOLS_DIR)/sdk: | $(DOWNLOAD_DIR)
	@echo "Downloading Telink Zigbee SDK v$(SDK_VERSION)..."
	@if [ ! -f "$(DOWNLOAD_DIR)/$(SDK_ARCHIVE)" ]; then \
		echo "Downloading $(SDK_URL)"; \
		if ! curl -L "$(SDK_URL)" \
			-o "$(DOWNLOAD_DIR)/$(SDK_ARCHIVE)" \
			--fail --show-error --connect-timeout 30; then \
			echo "Curl failed, trying wget..."; \
			wget -P $(DOWNLOAD_DIR) "$(SDK_URL)" || { \
				echo "Download failed. Please manually download $(SDK_ARCHIVE)"; \
				echo "and save it as: $(DOWNLOAD_DIR)/$(SDK_ARCHIVE)"; \
				echo "Then run 'make sdk' again"; \
				exit 1; \
			}; \
		fi; \
	fi
	@echo "Extracting Telink Zigbee SDK..."
	@rm -rf $(TOOLS_DIR)/sdk
	@mkdir -p $(TOOLS_DIR)/sdk_temp
	@unzip -q "$(DOWNLOAD_DIR)/$(SDK_ARCHIVE)" -d $(TOOLS_DIR)/sdk_temp
	@# Extract the tl_zigbee_sdk directory from the archive
	@SDK_DIR=$$(find $(TOOLS_DIR)/sdk_temp -name "tl_zigbee_sdk" -type d | head -1); \
	if [ -n "$$SDK_DIR" ]; then \
		mv "$$SDK_DIR" $(TOOLS_DIR)/sdk; \
		echo "Telink Zigbee SDK v$(SDK_VERSION) installed to $(TOOLS_DIR)/sdk"; \
	else \
		echo "Could not find tl_zigbee_sdk directory in archive"; \
		exit 1; \
	fi
	@rm -rf $(TOOLS_DIR)/sdk_temp

# TC32 GCC Toolchain
toolchain: $(TOOLS_DIR)/toolchain
	@echo "TC32 GCC toolchain installed successfully"

$(TOOLS_DIR)/toolchain: | $(DOWNLOAD_DIR)
	@echo "Downloading TC32 GCC toolchain..."
	@if [ ! -f "$(DOWNLOAD_DIR)/$(TOOLCHAIN_ARCHIVE)" ]; then \
		echo "Downloading $(TOOLCHAIN_URL)"; \
		if ! curl -L "$(TOOLCHAIN_URL)" \
			-o "$(DOWNLOAD_DIR)/$(TOOLCHAIN_ARCHIVE)" \
			--fail --show-error --connect-timeout 30; then \
			echo "Curl failed, trying wget..."; \
			wget -P $(DOWNLOAD_DIR) "$(TOOLCHAIN_URL)" || { \
				echo "Download failed. Please manually download $(TOOLCHAIN_ARCHIVE)"; \
				echo "and save it as: $(DOWNLOAD_DIR)/$(TOOLCHAIN_ARCHIVE)"; \
				echo "Then run 'make toolchain' again"; \
				exit 1; \
			}; \
		fi; \
	fi
	@echo "Verifying TC32 GCC toolchain checksum..."
	@echo "$(TOOLCHAIN_CHECKSUM) $(DOWNLOAD_DIR)/$(TOOLCHAIN_ARCHIVE)" | sha256sum -c - \
	   || (rm -f $(DOWNLOAD_DIR)/$(TOOLCHAIN_ARCHIVE) && false)
	@echo "Extracting TC32 GCC toolchain..."
	@rm -rf $(TOOLS_DIR)/toolchain
	@mkdir -p $(TOOLS_DIR)/toolchain
	@tar -xjf "$(DOWNLOAD_DIR)/$(TOOLCHAIN_ARCHIVE)" -C $(TOOLS_DIR)/toolchain
	@# Make binaries executable
	@find $(TOOLS_DIR)/toolchain -name "tc32-elf-*" -type f -exec chmod +x {} \; 2>/dev/null || true
	@echo "TC32 GCC toolchain installed to $(TOOLS_DIR)/toolchain"

# TlsrPgm programmer tool
tlsrpgm: $(TOOLS_DIR)/tlsrpgm
	@echo "TlsrPgm programmer tool installed successfully"

$(TOOLS_DIR)/tlsrpgm: | $(DOWNLOAD_DIR)
	@echo "Downloading TlsrPgm programmer tool..."
	@mkdir -p $(TOOLS_DIR)/tlsrpgm
	@if ! curl -L "$(TLSRPGM_URL)" \
		-o "$(TOOLS_DIR)/tlsrpgm/$(TLSRPGM_FILE)" \
		--fail --show-error --connect-timeout 30; then \
		echo "Curl failed, trying wget..."; \
		wget -O "$(TOOLS_DIR)/tlsrpgm/$(TLSRPGM_FILE)" "$(TLSRPGM_URL)" || { \
			echo "Download failed. Please manually download $(TLSRPGM_FILE)"; \
			echo "and save it as: $(TOOLS_DIR)/tlsrpgm/$(TLSRPGM_FILE)"; \
			echo "Then run 'make tlsrpgm' again"; \
			exit 1; \
		}; \
	fi
	@chmod +x "$(TOOLS_DIR)/tlsrpgm/$(TLSRPGM_FILE)"
	@echo "TlsrPgm programmer tool installed to $(TOOLS_DIR)/tlsrpgm/$(TLSRPGM_FILE)"

# Install targets (aliases)
install: all

install-sdk: $(TOOLS_DIR)/sdk

install-toolchain: $(TOOLS_DIR)/toolchain

install-tlsrpgm: $(TOOLS_DIR)/tlsrpgm

# Clean targets
clean:
	@echo "Removing installed tools from $(TOOLS_DIR)..."
	@rm -rf $(TOOLS_DIR)/sdk $(TOOLS_DIR)/toolchain $(TOOLS_DIR)/tlsrpgm
	@echo "Tools removed (downloads preserved)"

clean-downloads:
	@echo "Removing downloaded archives from $(DOWNLOAD_DIR)..."
	@rm -rf $(DOWNLOAD_DIR)
	@echo "Downloads removed"

clean-sdk:
	@echo "Removing Telink SDK..."
	@rm -rf $(TOOLS_DIR)/sdk
	@echo "SDK removed"

clean-toolchain:
	@echo "Removing TC32 toolchain..."
	@rm -rf $(TOOLS_DIR)/toolchain
	@echo "Toolchain removed"

clean-tlsrpgm:
	@echo "Removing TlsrPgm tool..."
	@rm -rf $(TOOLS_DIR)/tlsrpgm
	@echo "TlsrPgm tool removed"

# Legacy aliases for backward compatibility
clean_sdk: clean-sdk
clean_toolchain: clean-toolchain
clean_tlsrpgm: clean-tlsrpgm
clean_install: clean

# Verification targets
verify:
	@echo "Verifying installed tools..."
	@if [ -d "$(TOOLS_DIR)/sdk" ]; then \
		echo "✓ Telink Zigbee SDK: $(TOOLS_DIR)/sdk"; \
		echo "  Target version: $(SDK_VERSION)"; \
		VERSION_FILE="$(TOOLS_DIR)/sdk/zigbee/common/includes/zb_version.h"; \
		if [ -f "$$VERSION_FILE" ]; then \
			ACTUAL_VERSION=$$(grep "SDK_VERSION_ID" "$$VERSION_FILE" 2>/dev/null | sed 's/.*SDK_VERSION_ID[[:space:]]*//; s/[[:space:]]*$$//'); \
			echo "  Installed version: $$ACTUAL_VERSION"; \
		else \
			echo "  Installed version: Unable to determine"; \
		fi; \
	else \
		echo "✗ Telink Zigbee SDK: Not installed"; \
	fi
	@if [ -d "$(TOOLS_DIR)/toolchain" ]; then \
		echo "✓ TC32 GCC Toolchain: $(TOOLS_DIR)/toolchain"; \
		GCC_PATH=$$(find $(TOOLS_DIR)/toolchain -name "tc32-elf-gcc" -type f | head -1); \
		if [ -n "$$GCC_PATH" ]; then \
			echo "  GCC path: $$GCC_PATH"; \
			VERSION=$$($$GCC_PATH --version 2>/dev/null | head -1 || echo "Version unknown"); \
			echo "  Version: $$VERSION"; \
		fi; \
	else \
		echo "✗ TC32 GCC Toolchain: Not installed"; \
	fi
	@if [ -f "$(TOOLS_DIR)/tlsrpgm/$(TLSRPGM_FILE)" ]; then \
		echo "✓ TlsrPgm Tool: $(TOOLS_DIR)/tlsrpgm/$(TLSRPGM_FILE)"; \
		VERSION=$$(python3 "$(TOOLS_DIR)/tlsrpgm/$(TLSRPGM_FILE)" --help 2>/dev/null | head -1 || echo "Python tool - run with --help for version"); \
		echo "  Status: $$VERSION"; \
	else \
		echo "✗ TlsrPgm Tool: Not installed"; \
	fi

# Show current status
status:
	@echo "Telink Tools Status:"
	@echo "Tools directory: $(TOOLS_DIR)"
	@echo "Download directory: $(DOWNLOAD_DIR)"
	@echo ""
	@$(MAKE) -f tools.mk verify

# Test targets (for future use)
test:
	@echo "Test functionality not yet implemented for Telink tools"

clean-test:
	@echo "Test cleanup not yet implemented for Telink tools"