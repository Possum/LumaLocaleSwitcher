# TARGET #

TARGET := 3DS
LIBRARY := 0

ifeq ($(TARGET),3DS)
    ifeq ($(strip $(DEVKITPRO)),)
        $(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>devkitPro")
    endif

    ifeq ($(strip $(DEVKITARM)),)
        $(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
    endif
endif

# COMMON CONFIGURATION #

NAME := Luma Locale Switcher

BUILD_DIR := build
OUTPUT_DIR := output
INCLUDE_DIRS := include
SOURCE_DIRS := source

BUILD_FILTER := source/svchax/test/test.c

EXTRA_OUTPUT_FILES :=

LIBRARY_DIRS := $(DEVKITPRO)/libctru
LIBRARIES := citro3d ctru m

VERSION := $(shell git describe --tags --abbrev=0)
BUILD_FLAGS := -DLIBKHAX_AS_LIB -DVERSION_STRING="\"$(VERSION)\""
RUN_FLAGS :=

ifeq ($(LUMA_NIGHTLY),1)
    BUILD_FLAGS += -DLUMA_NIGHTLY
endif

# 3DS CONFIGURATION #

TITLE := $(NAME)
DESCRIPTION := Locale Switcher
AUTHOR := Possum
PRODUCT_CODE := LumaLocale
UNIQUE_ID := 0xA0CA1

SYSTEM_MODE := 64MB
SYSTEM_MODE_EXT := Legacy

ICON_FLAGS :=

ROMFS_DIR := romfs
BANNER_AUDIO := meta/audio.wav
BANNER_IMAGE := meta/banner.png
ICON := meta/icon.png

# INTERNAL #

include buildtools/make_base

3DS_IP     := CHANGEME
3DS_PORT   := 5000
SERVEFILES := servefiles.py

network_install: output/LumaLocaleSwitcher-$(VERSION).cia
	$(SERVEFILES) $(3DS_IP) output/LumaLocaleSwitcher-$(VERSION).cia

QRENCODE     := qrencode
PROJECT_NAME := LumaLocaleSwitcher
qrencode:
	$(QRENCODE) -o qr/$(VERSION).png https://github.com/$(AUTHOR)/$(PROJECT_NAME)/releases/download/$(VERSION)/LumaLocaleSwitcher-$(VERSION).cia
	rm qr/latest.png
	ln -s $(VERSION).png qr/latest.png
