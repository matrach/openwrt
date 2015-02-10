#
# Copyright (C) 2014 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=luci-app-relay
PKG_RELEASE:=20141013

PKG_LICENSE:=Apache-2.0

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/luci-app-relay
  SECTION:=luci
  CATEGORY:=LuCI
  SUBMENU:=3. Applications
  DEPENDS:=lua luci-base
 PKGARCH:=all
   TITLE:=Realy manager for RT5350f-OLinuXino
  MAINTAINER:=HeHoPMaJIeH <hehopmajieh@debian.bg>
endef

define Package/luci-app-relay/description
 Manage relay outputs
endef

define Build/Prepare
endef

define Build/Configure
endef

define Build/Compile
endef


define Package/luci-app-relay/install
	$(INSTALL_DIR) $(1)/usr/lib/lua/luci/controller/
	$(INSTALL_BIN) \
		./files/controller/relay.lua \
		$(1)/usr/lib/lua/luci/controller/

	$(INSTALL_DIR) $(1)/usr/lib/lua/luci/view/
	$(INSTALL_DATA) \
		./files/view/switch.htm \
		$(1)/usr/lib/lua/luci/view/

	$(INSTALL_DIR) $(1)/etc/config/
	$(INSTALL_DATA) \
                ./files/relay.config \
                $(1)/etc/config/relay
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/relay_gpio $(1)/etc/init.d/relay

endef
	
define Package/luci-app-relay/postinst
	#!/bin/sh
	mkdir $(1)/usr/lib/lua/luci/controller/relay/
	 mv $(1)/usr/lib/lua/luci/controller/relay.lua $(1)/usr/lib/lua/luci/controller/relay/relay.lua
	cp $(1)/usr/lib/lua/luci/view/switch.htm $(1)/usr/lib/lua/luci/controller/relay/switch.htm
	mkdir $(1)/usr/lib/lua/luci/view/relay/
	mv $(1)/usr/lib/lua/luci/view/switch.htm $(1)/usr/lib/lua/luci/view/relay/switch.htm
endef

$(eval $(call BuildPackage,luci-app-relay))
