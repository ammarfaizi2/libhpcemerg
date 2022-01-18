# SPDX-License-Identifier: GPL-2.0-only
#
# Copyright (C) 2022  Ammar Faizi <ammarfaizi2@gmail.com>
#

DEP_DIRS += $(BASE_DEP_DIR)/src/arch
DEP_DIRS += $(BASE_DEP_DIR)/src/arch/x86
DEP_DIRS += $(BASE_DEP_DIR)/src/arch/x86/entry

OBJ_PRE_CC += \
	$(BASE_DIR)/src/arch/x86/entry/entry_64.o

AS = $(CC)

$(BASE_DIR)/src/arch/x86/entry/entry_64.o:
	$(AS_PRINT)
	$(Q)$(AS) $(PIC_FLAGS) $(DEPFLAGS) $(CFLAGS) -c $(O_TO_S) -o $(@)
