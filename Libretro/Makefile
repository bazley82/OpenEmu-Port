# Makefile for OpenEmu Libretro Bridge and Plugins

CC = xcrun -sdk macosx clang
CXX = xcrun -sdk macosx clang++
FRAMEWORKS = -framework Foundation -framework Metal -framework AppKit -F../build-dir/Build/Products/Debug/OpenEmuARM64.app/Contents/Frameworks -framework OpenEmuBase -framework OpenEmuSystem
OPTS = -O3 -arch arm64 -DNDEBUG -ffast-math -flto -fmodules -fobjc-arc -I./Core -I./libretro-common/include -I../OpenEmu-SDK

PLUGIN_DIR = ./Plugins
SYSTEMS = Genesis PSX PSP DS N64

all: bridge plugins

bridge:
	$(CXX) $(OPTS) -dynamiclib Core/OELibretroGameCore.mm -o OELibretroGameCore.dylib $(FRAMEWORKS)
	codesign -s - OELibretroGameCore.dylib

plugins: bridge
	@for sys in $(SYSTEMS); do \
		echo "Packaging $$sys.oecoreplugin..."; \
		mkdir -p $(PLUGIN_DIR)/$$sys.oecoreplugin/Contents/MacOS; \
		cp OELibretroGameCore.dylib $(PLUGIN_DIR)/$$sys.oecoreplugin/Contents/MacOS/$$sys; \
		install_name_tool -id @executable_path/../MacOS/$$sys $(PLUGIN_DIR)/$$sys.oecoreplugin/Contents/MacOS/$$sys; \
		codesign -s - -f $(PLUGIN_DIR)/$$sys.oecoreplugin/Contents/MacOS/$$sys; \
	done

clean:
	rm -f OELibretroGameCore.dylib
	rm -rf $(PLUGIN_DIR)/*/Contents/MacOS/*
