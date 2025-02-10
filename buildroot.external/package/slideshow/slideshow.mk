################################################################################
#
# DRM Slide Show application
#
################################################################################

SLIDESHOW_SITE_METHOD = local
SLIDESHOW_SITE = $(TOPDIR)/../slideshow

define SLIDESHOW_INSTALL_INIT_SYSV
	$(INSTALL) -Dm0755 $(BR2_EXTERNAL)/package/slideshow/S20slideshow $(TARGET_DIR)/etc/init.d/S20slideshow
endef

$(eval $(meson-package))
