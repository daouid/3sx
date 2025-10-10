3SX_VERSION = main
3SX_SITE = 3SX_SITE = https://github.com/daouid/3sx.git
3SX_SITE_METHOD = git
3SX_DEPENDENCIES = sdl3 zlib ffmpeg

define 3SX_FIX_MAKEFILE
	sed -i 's/^\tclang /\t$$(CC) /g' $(@D)/Makefile
	sed -i 's/^clang /\$$(CC) /g' $(@D)/Makefile
	sed -i 's/CLANG_FLAGS/CFLAGS/g' $(@D)/Makefile
	sed -i 's/CLANG_LINKER_FLAGS/LDFLAGS/g' $(@D)/Makefile
	sed -i 's/CLANG_/COMMON_/g' $(@D)/Makefile
	sed -i 's/$$(COMMON_FLAGS)/$$(CFLAGS)/g' $(@D)/Makefile
	sed -i 's/$$(COMMON_LINKER_FLAGS)/$$(LDFLAGS)/g' $(@D)/Makefile
	sed -i 's/endifa/endif/g' $(@D)/Makefile
	sed -i '/#define NULL/d' $(@D)/include/common.h
	sed -i '/^\/\//s/\\$$//' $(@D)/include/sdk/eestruct.h
	sed -i 's/-DXPT_TGT_EE//g' $(@D)/Makefile
	sed -i 's/-Wno-tautological[^ ]*//g' $(@D)/Makefile
	sed -i 's/-Wno-gcc-compat//g' $(@D)/Makefile
	sed -i 's/-Wno-c2x-extensions//g' $(@D)/Makefile
	sed -i 's/-Wno-self-assign//g' $(@D)/Makefile
	sed -i 's/-Wno-deprecated-non-prototype//g' $(@D)/Makefile
	sed -i 's/-Wno-macro-redefined//g' $(@D)/Makefile
	sed -i 's/-Wno-unused-comparison//g' $(@D)/Makefile
	sed -i 's/-Wno-incompatible-function-pointer-types//g' $(@D)/Makefile
	sed -i 's/-Werror/-Wno-error/g' $(@D)/Makefile
	sed -i '/^COMMON_DEFINES :=/a COMMON_DEFINES += -DSint64=int64_t' $(@D)/Makefile
	sed -i 's/^void fatal_error/__attribute__((noreturn)) void fatal_error/' $(@D)/src/port/utils.c
	sed -i 's/) __dead2 {/) {/g' $(@D)/src/port/utils.c
	sed -i 's/^void not_implemented/__attribute__((noreturn)) void not_implemented/' $(@D)/src/port/utils.c
	sed -i 's/LoadExecPS2(const char\* filename, int num_args, char\* args\[\]) __attribute__((noreturn))/__attribute__((noreturn)) LoadExecPS2(const char* filename, int num_args, char* args[])/' $(@D)/src/port/sdk/sdk_stubs.c
endef

3SX_POST_PATCH_HOOKS += 3SX_FIX_MAKEFILE

define 3SX_BUILD_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) -C $(@D) \
		CC="$(TARGET_CC)" \
		PLATFORM=linux \
		USE_CLANG=0
endef

define 3SX_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/build/linux/3sx \
		$(TARGET_DIR)/usr/games/3sx
endef

$(eval $(generic-package))
