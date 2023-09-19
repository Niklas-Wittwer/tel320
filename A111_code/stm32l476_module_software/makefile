# Copyright (c) Acconeer AB, 2020
# All rights reserved

.DEFAULT_GOAL := all

# Variable to modify output messages
ifneq ($(V),)
SUPPRESS :=
else
SUPPRESS := @
endif

OPENOCD        := openocd
OUT_DIR        := out
OUT_OBJ_DIR    := $(OUT_DIR)/obj
ALL_TARGETS    :=
OUT_LIB_DIR    := $(OUT_DIR)/lib

IDIR := -I. -IInc

vpath %.c Src
vpath %.ss Src

LDLIBS := \
			-lacc_detector_distance \
			-lacc_detector_obstacle \
			-lacc_detector_presence \
			-lacc_module_software \
			-lacconeer \
			-lacc_rf_certification_test_a111

STM32_CUBE_GENERATED_FILES := \
			main.c \
			stm32l4xx_hal_msp.c \
			stm32l4xx_it.c \
			system_stm32l4xx.c \
			startup_stm32l476xx.s

STM32_CUBE_INTEGRATION_FILES := \
			acc_integration_cortex.c \
			acc_integration_log.c \
			acc_integration_stm32.c \
			acc_hal_integration_stm32cube_sparkfun_a111.c \
			acc_ms_system_stm32cube.c \
			acc_ms_system_cortex.c \
			syscalls.c \
			sysmem.c

include $(sort $(wildcard rule/makefile_target_*.inc))
include $(sort $(wildcard rule/makefile_define_*.inc))
include $(sort $(wildcard rule/makefile_build_*.inc))

LDLIBS := -Wl,--start-group $(LDLIBS) -Wl,--end-group

CFLAGS-$(OUT_OBJ_DIR)/acc_integration_stm32.o = -Wno-cast-qual
CFLAGS-$(OUT_OBJ_DIR)/syscalls.o = -Wno-missing-prototypes
CFLAGS-$(OUT_OBJ_DIR)/sysmem.o = -std=gnu11 -Wno-strict-prototypes -Wno-pedantic -Wno-missing-prototypes

SOURCES := $(STM32_CUBE_INTEGRATION_FILES) $(STM32_CUBE_GENERATED_FILES)

OBJECTS += $(addprefix $(OUT_OBJ_DIR)/, $(notdir $(patsubst %.c,%.o,$(patsubst %.s,%.o,$(SOURCES)))))

ALL_TARGETS += $(OUT_DIR)/acc_module_server.elf

all: $(ALL_TARGETS)

$(BUILD_LIBS) : | $(OUT_LIB_DIR)

$(OUT_OBJ_DIR)/%.o: %.c | $(OUT_OBJ_DIR)
	@echo "Compiling $(notdir $<)"
	$(SUPPRESS)$(TOOLS_CC) $^ -c $(CFLAGS) $(CFLAGS-$@) $(IDIR) -o $@

$(OUT_OBJ_DIR)/%.o: %.s | $(OUT_OBJ_DIR)
	@echo "Assembling $(notdir $<)"
	$(SUPPRESS)$(TOOLS_AS) $^ $(ASFLAGS) -o $@

$(OUT_LIB_DIR):
	$(SUPPRESS)mkdir -p $@

$(OUT_OBJ_DIR):
	$(SUPPRESS)mkdir -p $@

$(OUT_DIR):
	$(SUPPRESS)mkdir -p $@

$(OUT_DIR)/acc_module_server.elf: $(OBJECTS) $(BUILD_LIBS) | $(OUT_DIR)
	@echo "Linking $@"
	$(SUPPRESS)$(TOOLS_LD) $(LDFLAGS) $^ $(LDLIBS) -o $@
	$(SUPPRESS)$(TOOLS_OBJDUMP) -h -S $@ > $(basename $@).lss
	$(SUPPRESS)$(TOOLS_SIZE) -t $@ > $(basename $@)_size.txt
	$(SUPPRESS)$(TOOLS_OBJCOPY) -O ihex $@ $(basename $@).hex

.PHONY : clean
clean:
	$(SUPPRESS)rm -rf out obj

flash_acc_module_server:
	$(OPENOCD) $(OPENOCD_CONFIG) -c "program $(OUT_DIR)/acc_module_server.elf verify exit"
