# Makefile for OpenEmu Libretro Bridge and Plugins

CC = xcrun -sdk macosx clang
CXX = xcrun -sdk macosx clang++
FRAMEWORKS = -framework Foundation -framework Metal -framework AppKit -framework OpenGL -framework IOSurface -F/Users/devinhughes/AntiGravity/OpenEmuARM64/build-dir/Build/Products/Debug -framework OpenEmuBase -framework OpenEmuSystem
CXXFLAGS = -std=gnu++2b -D__LIBRETRO__ -D_M_ARM_64=1 -D_ARCH_64=1 -Ofast -arch arm64 -DNDEBUG -ffast-math -flto -fmodules -fobjc-arc -I ./Core -I ./libretro-common/include -I /Users/devinhughes/AntiGravity/OpenEmuARM64/OpenEmu-SDK -I /Users/devinhughes/AntiGravity/OpenEmuARM64/OpenEmu-SDK/OpenEmuBase -I /Users/devinhughes/AntiGravity/OpenEmuARM64/OpenEmu-SDK/OpenEmuSystem -I ./Cores/dolphin-libretro/Source/Core -I ./Cores/dolphin-libretro/Source/Core/VideoBackends/Metal -I ./Cores/dolphin-libretro/Externals/fmt/fmt/include
LDFLAGS = -Ofast -flto -dynamiclib
PLUGIN_DIR = ./Plugins
SYSTEMS = Genesis PSX PSP DS N64 SNES SNES9x GameCube Wii

APP_BUNDLE = /Users/devinhughes/AntiGravity/OpenEmuARM64/build-dir/Build/Products/Debug/OpenEmuARM64.app

all: bridge plugins deploy

release: bridge plugins deploy

bridge:
	$(CXX) $(CXXFLAGS) $(LDFLAGS) Core/OELibretroGameCore.mm -o OELibretroGameCore.dylib $(FRAMEWORKS)
	codesign -s - OELibretroGameCore.dylib

plugins: bridge
	@for sys in $(SYSTEMS); do \
		echo "Packaging $$sys.oecoreplugin..."; \
		mkdir -p $(PLUGIN_DIR)/$$sys.oecoreplugin/Contents/MacOS; \
		mkdir -p $(PLUGIN_DIR)/$$sys.oecoreplugin/Contents/Resources; \
		cp OELibretroGameCore.dylib $(PLUGIN_DIR)/$$sys.oecoreplugin/Contents/MacOS/$$sys; \
		install_name_tool -id $$sys $(PLUGIN_DIR)/$$sys.oecoreplugin/Contents/MacOS/$$sys; \
		identifier="openemu.system.$$(echo $$sys | tr '[:upper:]' '[:lower:]')"; \
		if [ "$$sys" = "SNES9x" ]; then identifier="openemu.system.snes"; fi; \
		if [ "$$sys" = "Genesis" ]; then identifier="openemu.system.sg"; fi; \
		if [ "$$sys" = "DS" ]; then identifier="openemu.system.nds"; fi; \
		if [ "$$sys" = "GameCube" ]; then identifier="openemu.system.gc"; fi; \
		if [ "$$sys" = "Wii" ]; then identifier="openemu.system.wii"; fi; \
		sed -e "s/\$$(PRODUCT_NAME)/$$sys/g" -e "s/\$$(SYSTEM_IDENTIFIER)/$$identifier/g" Info.plist.template > $(PLUGIN_DIR)/$$sys.oecoreplugin/Contents/Info.plist; \
		if [ -f "Plugins/$$sys.oecoreplugin/Contents/Resources/libretro_core.dylib" ]; then \
			echo "Using seeded dylib for $$sys"; \
		elif [ -f "Cores/$$sys/libretro_core.dylib" ]; then \
			cp Cores/$$sys/libretro_core.dylib $(PLUGIN_DIR)/$$sys.oecoreplugin/Contents/Resources/; \
		fi; \
		if [ -d "Cores/$$sys/Sys" ]; then \
			echo "Bundling Sys folder for $$sys"; \
			mkdir -p $(PLUGIN_DIR)/$$sys.oecoreplugin/Contents/Resources/dolphin-emu; \
			cp -R Cores/$$sys/Sys $(PLUGIN_DIR)/$$sys.oecoreplugin/Contents/Resources/dolphin-emu/; \
		fi; \
		codesign -s - --force --deep $(PLUGIN_DIR)/$$sys.oecoreplugin/Contents/MacOS/$$sys; \
		if [ -f "$(PLUGIN_DIR)/$$sys.oecoreplugin/Contents/Resources/libretro_core.dylib" ]; then \
			codesign -s - --force --deep $(PLUGIN_DIR)/$$sys.oecoreplugin/Contents/Resources/libretro_core.dylib; \
		fi; \
		codesign -s - --force --deep $(PLUGIN_DIR)/$$sys.oecoreplugin; \
	done

deploy: plugins
	@if [ -d "$(APP_BUNDLE)" ]; then \
		echo "Deploying plugins to app bundle (Resources)..."; \
		mkdir -p "$(APP_BUNDLE)/Contents/Resources"; \
		for sys in $(SYSTEMS); do \
			rm -rf "$(APP_BUNDLE)/Contents/Resources/$$sys.oecoreplugin"; \
			cp -R $(PLUGIN_DIR)/$$sys.oecoreplugin "$(APP_BUNDLE)/Contents/Resources/"; \
		done; \
		echo "Deploying plugins to app bundle (PlugIns/Cores)..."; \
		mkdir -p "$(APP_BUNDLE)/Contents/PlugIns/Cores"; \
		for sys in $(SYSTEMS); do \
			rm -rf "$(APP_BUNDLE)/Contents/PlugIns/Cores/$$sys.oecoreplugin"; \
			cp -R $(PLUGIN_DIR)/$$sys.oecoreplugin "$(APP_BUNDLE)/Contents/PlugIns/Cores/"; \
		done; \
		echo "Deep signing app bundle..."; \
		codesign -s - --force --deep "$(APP_BUNDLE)"; \
	else \
		echo "Warning: APP_BUNDLE not found at $(APP_BUNDLE). Skipping internal deployment."; \
	fi
	@echo "Deploy done."

clean:
	rm -f OELibretroGameCore.dylib
	rm -rf $(PLUGIN_DIR)/*/Contents/MacOS/*
