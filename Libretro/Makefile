# Makefile for OpenEmu Libretro Bridge and Plugins

CC = xcrun -sdk macosx clang
CXX = xcrun -sdk macosx clang++
FRAMEWORKS = -framework Foundation -framework Metal -framework AppKit -framework OpenGL -F../build-dir/Build/Products/Debug -framework OpenEmuBase -framework OpenEmuSystem
CXXFLAGS = -Ofast -arch arm64 -DNDEBUG -ffast-math -flto -fmodules -fobjc-arc -I./Core -I./libretro-common/include -I../OpenEmu-SDK
LDFLAGS = -Ofast -flto -dynamiclib
PLUGIN_DIR = ./Plugins
SYSTEMS = Genesis PSX PSP DS N64 SNES SNES9x

APP_BUNDLE = /Applications/OpenEmuARM64.app

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
		sed -e "s/\$$(PRODUCT_NAME)/$$sys/g" -e "s/\$$(SYSTEM_IDENTIFIER)/$$identifier/g" Info.plist.template > $(PLUGIN_DIR)/$$sys.oecoreplugin/Contents/Info.plist; \
		if [ -f "Plugins/$$sys.oecoreplugin/Contents/Resources/libretro_core.dylib" ]; then \
			echo "Using seeded dylib for $$sys"; \
		elif [ -f "Cores/$$sys/libretro_core.dylib" ]; then \
			cp Cores/$$sys/libretro_core.dylib $(PLUGIN_DIR)/$$sys.oecoreplugin/Contents/Resources/; \
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
			cp -R $(PLUGIN_DIR)/$$sys.oecoreplugin "$(APP_BUNDLE)/Contents/Resources/"; \
		done; \
		echo "Deploying plugins to app bundle (PlugIns/Cores)..."; \
		mkdir -p "$(APP_BUNDLE)/Contents/PlugIns/Cores"; \
		for sys in $(SYSTEMS); do \
			cp -R $(PLUGIN_DIR)/$$sys.oecoreplugin "$(APP_BUNDLE)/Contents/PlugIns/Cores/"; \
		done; \
		echo "Deep signing app bundle..."; \
		codesign -s - --force --deep "$(APP_BUNDLE)"; \
	else \
		echo "Warning: APP_BUNDLE not found at $(APP_BUNDLE). Skipping internal deployment."; \
	fi
	@echo "Deploying plugins to user Cores dir..."
	@mkdir -p "$(HOME)/Library/Application Support/OpenEmu/Cores"
	@for sys in $(SYSTEMS); do \
		cp -R $(PLUGIN_DIR)/$$sys.oecoreplugin "$(HOME)/Library/Application Support/OpenEmu/Cores/"; \
	done
	@echo "Deploy done."

clean:
	rm -f OELibretroGameCore.dylib
	rm -rf $(PLUGIN_DIR)/*/Contents/MacOS/*
