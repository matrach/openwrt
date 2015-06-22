#
# Copyright (C) 2015 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

define Profile/OLINUXINO-MT7620N
	NAME:=OLinuXino MT7620N
	PACKAGES:=swconfig
endef

define Profile/OLINUXINO-MT7620N/Description
	Package set compatible with the OLinuXino-MT7620N
endef
$(eval $(call Profile,OLINUXINO-MT7620N))
