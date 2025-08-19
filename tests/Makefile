# --- Compiler and flags ---
CC       := gcc          # host tests; use your cross-CC if you must
DEPFLAGS := -MMD -MP
CFLAGS   := -Wall -Wextra -O0 -g -std=gnu99 $(DEPFLAGS)
UNITY    := tests/unity/unity.c
INCLUDES := -Itests -Isrc -Itests/sdk_headers_stubs

# --- Directories ---
BUILD_DIR := tests/build

# --- Find all test suites (manifests) ---
ALL_MANIFESTS := $(shell find . -name manifest.mk)
ifeq ($(TEST),)
  MANIFESTS := $(ALL_MANIFESTS)
else
  MANIFESTS := $(filter %/$(TEST)/manifest.mk,$(ALL_MANIFESTS))
endif
ifneq ($(TESTS),)
  MANIFESTS := $(foreach t,$(TESTS),$(filter %/$(t)/manifest.mk,$(ALL_MANIFESTS)))
endif

# --- Globals ---
OBJS :=
DEPS :=
TEST_BINS :=

define build_one_suite
include $(1)

$$(TEST_NAME)_SRCS := $$(TEST_SOURCES) $$(UNDER_TEST) $$(REAL_DEPS) $$(STUBS) $(UNITY)

$$(TEST_NAME)_OBJS := $$(addprefix $(BUILD_DIR)/$$(TEST_NAME)/,$$(patsubst %.c,%.o,$$($$(TEST_NAME)_SRCS)))
$(BUILD_DIR)/$$(TEST_NAME)/%.o: %.c
	@mkdir -p $$(@D)
	$$(CC) $$(CFLAGS) -c $$< -o $$@

$(BUILD_DIR)/$$(TEST_NAME).test: $$($$(TEST_NAME)_OBJS)
	$$(CC) $$^ -o $$@ $(LDFLAGS) $$(LDFLAGS_$$(TEST_NAME)) $(LDFLAGS_EXTRA)

CFLAGS_ALL_$$(TEST_NAME) := $(CFLAGS) $(CFLAGS_EXTRA) $(INCLUDES) $$(INCLUDES_$$(TEST_NAME))
$$($$(TEST_NAME)_OBJS): CFLAGS := $$(CFLAGS_ALL_$$(TEST_NAME))

OBJS     += $$($$(TEST_NAME)_OBJS)
DEPS     += $$(patsubst %.o,%.d,$$($$(TEST_NAME)_OBJS))
TEST_BINS += $(BUILD_DIR)/$$(TEST_NAME).test
endef

# --- Expand the template for each manifest ---
$(foreach m,$(MANIFESTS),$(eval $(call build_one_suite,$(m))))

.PHONY: all test list clean build
.DEFAULT_GOAL := all

all: test

# Build all selected suites' binaries (but don't run)
build: $(TEST_BINS)

# Build & run
test: $(TEST_BINS)
	@set -e; \
	for t in $(TEST_BINS); do \
	  echo "== Running $$t =="; \
	  ./$$t; \
	done

# Show which manifests/suites were detected after filtering
list:
	@echo "Manifests:"; printf " - %s\n" $(MANIFESTS)
	@echo "Suites:";    printf " - %s\n" $(TEST_BINS)

clean:
	@rm -rf $(BUILD_DIR)

-include $(DEPS)