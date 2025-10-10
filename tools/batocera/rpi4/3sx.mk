3SX_VERSION = main
3SX_SITE = https://github.com/daouid/3sx.git
3SX_SITE_METHOD = git
3SX_DEPENDENCIES = sdl3 zlib ffmpeg

# Configure CMake for cross-compilation
define 3SX_CONFIGURE_CMDS
	cmake -S $(@D) -B $(@D)/build \
		-DCMAKE_INSTALL_PREFIX=$(TARGET_DIR)/usr/games/3sx \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_C_COMPILER="$(TARGET_CC)" \
		-DCMAKE_CXX_COMPILER="$(TARGET_CXX)"
endef

# Build the project with CMake
define 3SX_BUILD_CMDS
	cmake --build $(@D)/build
endef

# Install the resulting binary to the Batocera target image
define 3SX_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/build/3sx \
		$(TARGET_DIR)/usr/games/3sx
endef

$(eval $(generic-package))
