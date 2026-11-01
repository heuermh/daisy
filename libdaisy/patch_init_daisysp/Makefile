# Project Name
TARGET = patch_init_daisysp

# run from SRAM via the Daisy bootloader
APP_TYPE = BOOT_SRAM

# Library Locations
LIBDAISY_DIR ?= ../libDaisy
DAISYSP_DIR ?= ../DaisySP

# Sources
CPP_SOURCES = patch_init_daisysp.cpp

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile
