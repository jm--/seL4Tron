#
# Copyright (c) 2015, Josef Mihalits
#
# This software may be distributed and modified according to the terms of
# the BSD 2-Clause license. Note that NO WARRANTY is provided.
# See "LICENSE_BSD2.txt" for details.
#

TARGETS := $(notdir $(SOURCE_DIR)).bin

ENTRY_POINT := _sel4_start

# required source files
CFILES   := $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/*.c))

# CPIO archive
OFILES := archive.o

# required libraries
LIBS = c cpio sel4 sel4muslcsys sel4vka sel4allocman \
       platsupport sel4platsupport sel4vspace \
       sel4simple sel4simple-stable sel4utils utils

# extra flags
CFLAGS += -Werror -ggdb -g3
ifdef CONFIG_X86_64
CFLAGS += -mno-sse
endif

include $(SEL4_COMMON)/common.mk

CPIO_FILES := sel4.ppm
CPIO_FILES_FULL := $(addprefix $(SOURCE_DIR)/, $(CPIO_FILES))

archive.o: $(CPIO_FILES_FULL)
	$(Q)mkdir -p $(dir $@)
	${COMMON_PATH}/files_to_obj.sh $@ _cpio_archive $^
