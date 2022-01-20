# SPDX-License-Identifier: GPL-2.0-only
#
# Copyright (C) 2022  Ammar Faizi <ammarfaizi2@gmail.com>
#

DEP_DIRS += $(BASE_DEP_DIR)/tests

TEST_SOURCES := \
	bug.c \
	reg_dump.c \
	sigill.c \
	warn.c \
	# EOF

TEST_SOURCES := $(TEST_SOURCES:%=$(BASE_DIR)/tests/%)
TEST_EXECUTABLES := $(patsubst %.c,%,$(patsubst %.cc,%,$(TEST_SOURCES)))

test: $(TEST_EXECUTABLES)

%: %.c $(TARGET_BIN)
	$(CC_PRINT)
	$(Q)$(CC) $(PIC_FLAGS) $(INCLUDE_DIR) $(DEPFLAGS) \
	$(CFLAGS) $(^) -o $(@) -L$(BASE_DIR) -lhpcemerg -Wl,-rpath="$(BASE_DIR)"

clean_test:
	$(Q)$(RM) -vf $(TEST_EXECUTABLES)

.PHONY: test clean_test
