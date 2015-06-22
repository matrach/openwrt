#
# Copyright (C) 2008-2012 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=display
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define KernelPackage/display
  SUBMENU:=LED modules
  TITLE:=7 segment display driver
  DEPENDS:=
  FILES:=$(PKG_BUILD_DIR)/display.ko
  AUTOLOAD:=$(call AutoLoad,31,display)
  KCONFIG:=
endef

define KernelPackage/display/description
 Kernel module for 7 segment display on OLinuXino-RT5350F-DIN.
endef

EXTRA_KCONFIG:= \
	CONFIG_DISPLAY=m

EXTRA_CFLAGS:= \
	$(patsubst CONFIG_%, -DCONFIG_%=1, $(patsubst %=m,%,$(filter %=m,$(EXTRA_KCONFIG)))) \
	$(patsubst CONFIG_%, -DCONFIG_%=1, $(patsubst %=y,%,$(filter %=y,$(EXTRA_KCONFIG)))) \

MAKE_OPTS:= \
	ARCH="$(LINUX_KARCH)" \
	CROSS_COMPILE="$(TARGET_CROSS)" \
	SUBDIRS="$(PKG_BUILD_DIR)" \
	EXTRA_CFLAGS="$(EXTRA_CFLAGS)" \
	$(EXTRA_KCONFIG)

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
	$(MAKE) -C "$(LINUX_DIR)" \
		$(MAKE_OPTS) \
		modules
endef

$(eval $(call KernelPackage,display))
