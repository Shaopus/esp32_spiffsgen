#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := spiffsgen

include $(IDF_PATH)/make/project.mk

MKSPIFFS=$(PROJECT_PATH)/mkspiffs/mkspiffs
SPIFFS_IMAGE=$(BUILD_DIR_BASE)/spiffs.bin

# Note: The $(shell ...) hack is to start a clean make session
# Clean mkspiffs
clean: clean_spiffs

clean_spiffs:
	echo $(shell make -C $(PROJECT_PATH)/mkspiffs clean)

# Build mkspiffs utility
$(MKSPIFFS):
	echo $(shell make -C $(PROJECT_PATH)/mkspiffs)

SPIFFS_PARTITION=$(shell grep "^fs_0" partitions.csv | sed 's/,//g')
SPIFFS_OFFSET=$(word 4, $(SPIFFS_PARTITION))
SPIFFS_SIZE=$(word 5, $(SPIFFS_PARTITION))

# Build SPIFFS image
$(SPIFFS_IMAGE): $(PROJECT_PATH)/data $(MKSPIFFS) partitions.csv 
	$(MKSPIFFS) -c $< -b 4096 -p 256 -s $(SPIFFS_SIZE) $@

# Need to generate SPIFFS image before flashing
flash: $(SPIFFS_IMAGE)

# Include SPIFFS offset + image in the flash command
ESPTOOL_ALL_FLASH_ARGS += $(SPIFFS_OFFSET) $(SPIFFS_IMAGE) \
  $$(($(SPIFFS_OFFSET) + $(SPIFFS_SIZE))) $(SPIFFS_IMAGE)
