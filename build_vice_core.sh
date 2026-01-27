#!/bin/bash
set -e

PROJECT_DIR="/Users/barriesanders/.gemini/antigravity/scratch/OpenEmu_Port"
VICE_DIR="$PROJECT_DIR/vice-3.10"
CORE_DIR="$PROJECT_DIR/VICE-Core"
BUILD_DIR="$PROJECT_DIR/Build/OpenEmu.app/Contents/PlugIns/Cores"
FRAMEWORK_PATH="$PROJECT_DIR/Build/OpenEmu.app/Contents/Frameworks"

mkdir -p "$BUILD_DIR/VICE.oecoreplugin/Contents/MacOS"
mkdir -p "$BUILD_DIR/VICE.oecoreplugin/Contents/Resources"

cp "$CORE_DIR/Info.plist" "$BUILD_DIR/VICE.oecoreplugin/Contents/"
mkdir -p "$BUILD_DIR/VICE.oecoreplugin/Contents/Resources"
cp -R "$VICE_DIR/data/" "$BUILD_DIR/VICE.oecoreplugin/Contents/Resources/data"

# FIX SYSTEM PLUGIN
# FIX SYSTEM PLUGIN
# The pre-installed system plugin is broken/missing Info.plist. We repair it here.
SYS_PLUGIN_DIR="$PROJECT_DIR/Build/OpenEmu.app/Contents/PlugIns/Systems/Commodore 64.oesystemplugin/Contents"
mkdir -p "$SYS_PLUGIN_DIR"
if [ -f "$PROJECT_DIR/OpenEmu/SystemPlugins/Commodore 64/Commodore 64-Info.plist" ]; then
    echo "Repairing C64 System Plugin..."
    cp "$PROJECT_DIR/OpenEmu/SystemPlugins/Commodore 64/Commodore 64-Info.plist" "$SYS_PLUGIN_DIR/Info.plist"
    
    # Expand variables in Info.plist
    sed -i '' 's/$(EXECUTABLE_NAME)/Commodore 64/g' "$SYS_PLUGIN_DIR/Info.plist"
    sed -i '' 's/$(PRODUCT_BUNDLE_IDENTIFIER)/org.openemu.Commodore-64/g' "$SYS_PLUGIN_DIR/Info.plist"
    sed -i '' 's/$(PRODUCT_NAME)/Commodore 64/g' "$SYS_PLUGIN_DIR/Info.plist"
    sed -i '' 's/$(PRODUCT_BUNDLE_PACKAGE_TYPE)/BNDL/g' "$SYS_PLUGIN_DIR/Info.plist"
    sed -i '' 's/$(DEVELOPMENT_LANGUAGE)/en/g' "$SYS_PLUGIN_DIR/Info.plist"
    sed -i '' 's/OESystemController/OEC64SystemController/g' "$SYS_PLUGIN_DIR/Info.plist"
    
    # Compile the System Plugin binary
    echo "Compiling C64 System Plugin..."
    mkdir -p "$SYS_PLUGIN_DIR/MacOS"
    mkdir -p "$SYS_PLUGIN_DIR/Resources"
    
    # Copy Resources
    cp "$PROJECT_DIR/OpenEmu/SystemPlugins/Commodore 64/"*.plist "$SYS_PLUGIN_DIR/Resources/"
    # Remove the Info.plist from Resources if it was copied by the wildcard (it should be in Contents)
    rm -f "$SYS_PLUGIN_DIR/Resources/Commodore 64-Info.plist"
    
    # Compile Asset Catalog
    echo "Compiling Asset Catalog..."
    /Applications/Xcode.app/Contents/Developer/usr/bin/actool "$PROJECT_DIR/OpenEmu/SystemPlugins/Commodore 64/Images.xcassets" \
        --compile "$SYS_PLUGIN_DIR/Resources" \
        --output-format human-readable-text \
        --notices --warnings \
        --platform macosx \
        --target-device mac \
        --minimum-deployment-target 10.14.4
    
    clang -bundle -o "$SYS_PLUGIN_DIR/MacOS/Commodore 64" \
        -isysroot $(xcrun --show-sdk-path) \
        -arch arm64 \
        -fobjc-arc \
        -fmodules \
        -Wl,-rpath,@loader_path/../../../../../Frameworks \
        -F"$FRAMEWORK_PATH" \
        -I"$FRAMEWORK_PATH/OpenEmuSystem.framework/Headers" \
        -I"$PROJECT_DIR/OpenEmu-SDK" \
        -I"$PROJECT_DIR/OpenEmu-SDK/OpenEmuSystem" \
        -framework OpenEmuSystem \
        -framework OpenEmuBase \
        -framework Cocoa \
        -I"$PROJECT_DIR/OpenEmu/SystemPlugins/Commodore 64" \
        "$PROJECT_DIR/OpenEmu/SystemPlugins/Commodore 64/OEC64SystemResponder.m" \
        "$PROJECT_DIR/OpenEmu/SystemPlugins/Commodore 64/OEC64SystemController.m"
    
    echo "Signing C64 System Plugin..."
    codesign --force --sign - --deep "$SYS_PLUGIN_DIR/.."
        
else
    echo "Error: C64 System Plugin source plist not found at $PROJECT_DIR/OpenEmu/SystemPlugins/Commodore 64/Commodore 64-Info.plist"
    exit 1
fi

# Collect all .a files, excluding GTK/SDL and non-C64 machines and stubs
# Collect all .a files, excluding GTK/SDL and non-relevant stubs
# Collect all .a files, excluding GTK/SDL, non-relevant machines, and stubs
# Collect all .a files, excluding GTK/SDL and non-C64 machines and stubs
# We merge the grep exclusions to be safe
# We merge the grep exclusions to be safe
LIBS=$(find "$VICE_DIR/src" -name "*.a" | grep -v "gtk3" | grep -v "sdl" | grep -v "novte" | grep -v "widgets" | grep -v "/doc/" | grep -v "stubs.a" | grep -v "libviciidtv.a" | grep -v "libviciisc.a" | grep -v "libc64sc.a" | grep -v "libc64dtv.a" | grep -v "libscpu64.a" | grep -v "libvsid.a" | grep -v "libcbm" | grep -v "libpet.a" | grep -v "libplus4.a" | grep -v "libvic20.a" | grep -v "libc128.a" | grep -v "cbm2" | grep -v "libgfxoutputdrv.a" | grep -v "libffmpeg.a" | tr '\n' ' ')
echo "DEBUG LIBS: $LIBS"

# Collect object files in src/ but exclude c1541.o and c1541-stubs.o
SRC_OBJS=$(find "$VICE_DIR/src" -maxdepth 1 -name "*.o" ! -name "main.o" ! -name "c1541.o" ! -name "c1541-stubs.o" ! -name "cartconv.o" ! -name "petcat.o" | tr '\n' ' ')

# Shared architecture objects (needed for tick, archdep, uiactions, etc.)
# Exclude problematic objects and uiactions.o (implemented by headless/ui.c or stubbed)
# Exclude uimon.o (using headless/uimon.c instead)
SHARED_OBJS=$(find "$VICE_DIR/src/arch/shared" -maxdepth 1 -name "*.o" ! -name "macOS-util.o" ! -name "archdep_exit.o" ! -name "archdep_cbmfont.o" ! -name "uiactions.o" ! -name "archdep_get_vice_datadir.o" ! -name "archdep_default_logger.o" ! -name "uimon.o" ! -name "socketdrv.o" | tr '\n' ' ')

# Headless sources to fill gaps
# Headless sources to fill gaps
HEADLESS_SRCS="$VICE_DIR/src/main.c $VICE_DIR/src/arch/headless/ui.c $VICE_DIR/src/arch/headless/uimon.c $VICE_DIR/src/arch/headless/console.c $VICE_DIR/src/arch/headless/kbd.c $VICE_DIR/src/arch/headless/mousedrv.c $VICE_DIR/src/arch/headless/uistatusbar.c $VICE_DIR/src/arch/headless/archdep.c $VICE_DIR/src/arch/headless/c128ui.c $VICE_DIR/src/arch/headless/c64dtvui.c $VICE_DIR/src/arch/headless/c64scui.c $VICE_DIR/src/arch/headless/c64ui.c $VICE_DIR/src/arch/headless/cbm2ui.c $VICE_DIR/src/arch/headless/cbm5x0ui.c $VICE_DIR/src/arch/headless/petui.c $VICE_DIR/src/arch/headless/plus4ui.c $VICE_DIR/src/arch/headless/scpu64ui.c $VICE_DIR/src/arch/headless/vic20ui.c $VICE_DIR/src/arch/headless/vsidui.c"

# Manually compile vsync.c and sound.c as they are missing from make output
INCLUDES="-I$VICE_DIR/src -I$VICE_DIR/src/video -I$VICE_DIR/src/c64 -I$VICE_DIR/src/sid -I$VICE_DIR/src/vicii -I$VICE_DIR/src/raster -I$VICE_DIR/src/monitor -I$VICE_DIR/src/lib/p64 -I$VICE_DIR/src/platform -I$VICE_DIR/src/drive -I$VICE_DIR/src/vdrive -I$VICE_DIR -I$VICE_DIR/src/arch/shared -I$VICE_DIR/src/arch/shared/hotkeys -I$VICE_DIR/src/monitor -I$VICE_DIR/src/parallel -I$VICE_DIR/src/arch/headless -I$VICE_DIR/src/core/rtc -I$VICE_DIR/src/joyport -I$VICE_DIR/src/hvsc -I$VICE_DIR/src/arch/shared/socketdrv -I$VICE_DIR/src/printerdrv -I$VICE_DIR/src/lib -I$VICE_DIR/src/tapeport -I$VICE_DIR/src/userport -I$VICE_DIR/src/datasette -I$VICE_DIR/src/fsdevice -I$VICE_DIR/src/imagecontents -I$VICE_DIR/src/c64/cart -I$VICE_DIR/src/rs232drv -I$VICE_DIR/src/core -I$VICE_DIR/src/samplerdrv"

echo "Compiling vsync.c..."
clang -c "$VICE_DIR/src/vsync.c" -o "$VICE_DIR/src/vsync.o" \
    $INCLUDES -DMACOS_COMPILE -D_REENTRANT -UUSE_GTK3UI

echo "Compiling sound.c..."
clang -c "$VICE_DIR/src/sound.c" -o "$VICE_DIR/src/sound.o" \
    $INCLUDES -DMACOS_COMPILE -D_REENTRANT -UUSE_GTK3UI

echo "Compiling ciacore.c..."
clang -c "$VICE_DIR/src/core/ciacore.c" -o "$VICE_DIR/src/core/ciacore.o" \
    $INCLUDES -DMACOS_COMPILE -D_REENTRANT -UUSE_GTK3UI

echo "Updating libcore.a with new ciacore.o..."
if [ -f "$VICE_DIR/src/core/libcore.a" ]; then
    ar r "$VICE_DIR/src/core/libcore.a" "$VICE_DIR/src/core/ciacore.o"
else
    echo "Warning: libcore.a not found! Linking ciacore.o directly might fail if libcore.a is missing."
fi

    echo "Compiling c64.c..."
clang -c "$VICE_DIR/src/c64/c64.c" -o "$VICE_DIR/src/c64/c64.o" \
    $INCLUDES -DMACOS_COMPILE -D_REENTRANT -UUSE_GTK3UI

    echo "Compiling c64-resources.c..."
clang -c "$VICE_DIR/src/c64/c64-resources.c" -o "$VICE_DIR/src/c64/c64-resources.o" \
    $INCLUDES -DMACOS_COMPILE -D_REENTRANT -UUSE_GTK3UI

echo "Updating libc64.a with new c64.o..."
if [ -f "$VICE_DIR/src/c64/libc64.a" ]; then
    ar r "$VICE_DIR/src/c64/libc64.a" "$VICE_DIR/src/c64/c64.o" "$VICE_DIR/src/c64/c64-resources.o"
else
    echo "Warning: libc64.a not found!"
fi

echo "Compiling resid-dtv.cc..."
clang -c "$VICE_DIR/src/sid/resid-dtv.cc" -o "$VICE_DIR/src/sid/resid-dtv.o" \
    $INCLUDES -DMACOS_COMPILE -D_REENTRANT -UUSE_GTK3UI

echo "Updating libsid_dtv.a with new resid-dtv.o..."
if [ -f "$VICE_DIR/src/sid/libsid_dtv.a" ]; then
    ar r "$VICE_DIR/src/sid/libsid_dtv.a" "$VICE_DIR/src/sid/resid-dtv.o"
else
    echo "Warning: libsid_dtv.a not found!"
fi

echo "Compiling util.c..."
clang -c "$VICE_DIR/src/util.c" -o "$VICE_DIR/src/util.o" \
    $INCLUDES -DMACOS_COMPILE -D_REENTRANT -UUSE_GTK3UI

echo "Compiling zfile.c..."
clang -c "$VICE_DIR/src/zfile.c" -o "$VICE_DIR/src/zfile.o" \
    $INCLUDES -DMACOS_COMPILE -D_REENTRANT -UUSE_GTK3UI

echo "Compiling network.c..."
clang -c "$VICE_DIR/src/network.c" -o "$VICE_DIR/src/network.o" \
    $INCLUDES -DMACOS_COMPILE -D_REENTRANT -UUSE_GTK3UI

echo "Compiling traps.c..."
clang -c "$VICE_DIR/src/traps.c" -o "$VICE_DIR/src/traps.o" \
    $INCLUDES -DMACOS_COMPILE -D_REENTRANT -UUSE_GTK3UI

echo "Compiling sysfile.c..."
clang -c "$VICE_DIR/src/sysfile.c" -o "$VICE_DIR/src/sysfile.o" \
    $INCLUDES -DMACOS_COMPILE -D_REENTRANT -UUSE_GTK3UI

echo "Compiling socketdrv.c..."
clang -c "$VICE_DIR/src/arch/shared/socketdrv/socketdrv.c" -o "$VICE_DIR/src/arch/shared/socketdrv/socketdrv.o" \
    $INCLUDES -DMACOS_COMPILE -D_REENTRANT -UUSE_GTK3UI

echo "Compiling snapshot.c..."
clang -c "$VICE_DIR/src/snapshot.c" -o "$VICE_DIR/src/snapshot.o" \
    $INCLUDES -DMACOS_COMPILE -D_REENTRANT -UUSE_GTK3UI

echo "Compiling socket.c..."
clang -c "$VICE_DIR/src/socket.c" -o "$VICE_DIR/src/socket.o" \
    $INCLUDES -DMACOS_COMPILE -D_REENTRANT -UUSE_GTK3UI

echo "Compiling resources.c..."
clang -c "$VICE_DIR/src/resources.c" -o "$VICE_DIR/src/resources.o" \
    $INCLUDES -DMACOS_COMPILE -D_REENTRANT -UUSE_GTK3UI

echo "Compiling soundcoreaudio.c..."
clang -c "$VICE_DIR/src/arch/shared/sounddrv/soundcoreaudio.c" -o "$VICE_DIR/src/arch/shared/sounddrv/soundcoreaudio.o" \
    $INCLUDES -DMACOS_COMPILE -D_REENTRANT -UUSE_GTK3UI

echo "Compiling screenshot.c..."
clang -c "$VICE_DIR/src/screenshot.c" -o "$VICE_DIR/src/screenshot.o" \
    $INCLUDES -DMACOS_COMPILE -D_REENTRANT -UUSE_GTK3UI

echo "Compiling log.c..."
clang -c "$VICE_DIR/src/log.c" -o "$VICE_DIR/src/log.o" \
    $INCLUDES -DMACOS_COMPILE -D_REENTRANT -UUSE_GTK3UI

echo "Compiling zipcode.c..."
clang -c "$VICE_DIR/src/zipcode.c" -o "$VICE_DIR/src/zipcode.o" \
    $INCLUDES -DMACOS_COMPILE -D_REENTRANT -UUSE_GTK3UI

echo "Compiling palette.c..."
clang -c "$VICE_DIR/src/palette.c" -o "$VICE_DIR/src/palette.o" \
    $INCLUDES -DMACOS_COMPILE -D_REENTRANT -UUSE_GTK3UI

echo "Compiling alarm.c..."
clang -c "$VICE_DIR/src/alarm.c" -o "$VICE_DIR/src/alarm.o" \
    $INCLUDES -DMACOS_COMPILE -D_REENTRANT -UUSE_GTK3UI

echo "Compiling lib.c..."
clang -c "$VICE_DIR/src/lib.c" -o "$VICE_DIR/src/lib.o" \
    $INCLUDES -DMACOS_COMPILE -D_REENTRANT -UUSE_GTK3UI

echo "Compiling machine.c..."
clang -c "$VICE_DIR/src/machine.c" -o "$VICE_DIR/src/machine.o" \
    $INCLUDES -DMACOS_COMPILE -D_REENTRANT -UUSE_GTK3UI

echo "Compiling ram.c..."
clang -c "$VICE_DIR/src/ram.c" -o "$VICE_DIR/src/ram.o" \
    $INCLUDES -DMACOS_COMPILE -D_REENTRANT -UUSE_GTK3UI

echo "Compiling keyboard.c..."
clang -c "$VICE_DIR/src/keyboard.c" -o "$VICE_DIR/src/keyboard.o" \
    $INCLUDES -DMACOS_COMPILE -D_REENTRANT -UUSE_GTK3UI

echo "Compiling keymap.c..."
clang -c "$VICE_DIR/src/keymap.c" -o "$VICE_DIR/src/keymap.o" \
    $INCLUDES -DMACOS_COMPILE -D_REENTRANT -UUSE_GTK3UI

echo "Compiling kbdbuf.c..."
clang -c "$VICE_DIR/src/kbdbuf.c" -o "$VICE_DIR/src/kbdbuf.o" \
    $INCLUDES -DMACOS_COMPILE -D_REENTRANT -UUSE_GTK3UI

echo "Compiling machine-bus.c..."
clang -c "$VICE_DIR/src/machine-bus.c" -o "$VICE_DIR/src/machine-bus.o" \
    $INCLUDES -DMACOS_COMPILE -D_REENTRANT -UUSE_GTK3UI

echo "Compiling interrupt.c..."
clang -c "$VICE_DIR/src/interrupt.c" -o "$VICE_DIR/src/interrupt.o" \
    $INCLUDES -DMACOS_COMPILE -D_REENTRANT -UUSE_GTK3UI

echo "Compiling rawfile.c..."
clang -c "$VICE_DIR/src/rawfile.c" -o "$VICE_DIR/src/rawfile.o" \
    $INCLUDES -DMACOS_COMPILE -D_REENTRANT -UUSE_GTK3UI

echo "Compiling c64keyboard.c..."
clang -c "$VICE_DIR/src/c64/c64keyboard.c" -o "$VICE_DIR/src/c64/c64keyboard.o" \
    $INCLUDES -DMACOS_COMPILE -D_REENTRANT -UUSE_GTK3UI

echo "Compiling profiler.c..."
clang -c "$VICE_DIR/src/profiler.c" -o "$VICE_DIR/src/profiler.o" \
    $INCLUDES -DMACOS_COMPILE -D_REENTRANT -UUSE_GTK3UI

echo "Compiling opencbmlib.c..."
clang -c "$VICE_DIR/src/opencbmlib.c" -o "$VICE_DIR/src/opencbmlib.o" \
    $INCLUDES -DMACOS_COMPILE -D_REENTRANT -UUSE_GTK3UI

echo "Compiling attach.c..."
clang -c "$VICE_DIR/src/attach.c" -o "$VICE_DIR/src/attach.o" \
    $INCLUDES -DMACOS_COMPILE -D_REENTRANT -UUSE_GTK3UI

echo "Compiling init.c..."
clang -c "$VICE_DIR/src/init.c" -o "$VICE_DIR/src/init.o" \
    $INCLUDES -DMACOS_COMPILE -D_REENTRANT -UUSE_GTK3UI

echo "Compiling initcmdline.c..."
clang -c "$VICE_DIR/src/initcmdline.c" -o "$VICE_DIR/src/initcmdline.o" \
    $INCLUDES -DMACOS_COMPILE -D_REENTRANT -UUSE_GTK3UI

echo "Compiling gcr.c..."
clang -c "$VICE_DIR/src/gcr.c" -o "$VICE_DIR/src/gcr.o" \
    $INCLUDES -DMACOS_COMPILE -D_REENTRANT -UUSE_GTK3UI

echo "Compiling event.c..."
clang -c "$VICE_DIR/src/event.c" -o "$VICE_DIR/src/event.o" \
    $INCLUDES -DMACOS_COMPILE -D_REENTRANT -UUSE_GTK3UI

echo "Compiling fliplist.c..."
clang -c "$VICE_DIR/src/fliplist.c" -o "$VICE_DIR/src/fliplist.o" \
    $INCLUDES -DMACOS_COMPILE -D_REENTRANT -UUSE_GTK3UI

echo "Compiling crt.c..."
clang -c "$VICE_DIR/src/crt.c" -o "$VICE_DIR/src/crt.o" \
    $INCLUDES -DMACOS_COMPILE -D_REENTRANT -UUSE_GTK3UI

echo "Compiling cbmimage.c..."
clang -c "$VICE_DIR/src/cbmimage.c" -o "$VICE_DIR/src/cbmimage.o" \
    $INCLUDES -DMACOS_COMPILE -D_REENTRANT -UUSE_GTK3UI

echo "Compiling findpath.c..."
clang -c "$VICE_DIR/src/findpath.c" -o "$VICE_DIR/src/findpath.o" \
    $INCLUDES -DMACOS_COMPILE -D_REENTRANT -UUSE_GTK3UI

echo "Compiling info.c..."
clang -c "$VICE_DIR/src/info.c" -o "$VICE_DIR/src/info.o" \
    $INCLUDES -DMACOS_COMPILE -D_REENTRANT -UUSE_GTK3UI

echo "Compiling crc32.c..."
clang -c "$VICE_DIR/src/crc32.c" -o "$VICE_DIR/src/crc32.o" \
    $INCLUDES -DMACOS_COMPILE -D_REENTRANT -UUSE_GTK3UI

echo "Compiling debug.c..."
clang -c "$VICE_DIR/src/debug.c" -o "$VICE_DIR/src/debug.o" \
    $INCLUDES -DMACOS_COMPILE -D_REENTRANT -UUSE_GTK3UI

echo "Compiling cbmdos.c..."
clang -c "$VICE_DIR/src/cbmdos.c" -o "$VICE_DIR/src/cbmdos.o" \
    $INCLUDES -DMACOS_COMPILE -D_REENTRANT -UUSE_GTK3UI

echo "Compiling charset.c..."
clang -c "$VICE_DIR/src/charset.c" -o "$VICE_DIR/src/charset.o" \
    $INCLUDES -DMACOS_COMPILE -D_REENTRANT -UUSE_GTK3UI

echo "Compiling cmdline.c..."
clang -c "$VICE_DIR/src/cmdline.c" -o "$VICE_DIR/src/cmdline.o" \
    $INCLUDES -DMACOS_COMPILE -D_REENTRANT -UUSE_GTK3UI

echo "Compiling romset.c..."
clang -c "$VICE_DIR/src/romset.c" -o "$VICE_DIR/src/romset.o" \
    $INCLUDES -DMACOS_COMPILE -D_REENTRANT -UUSE_GTK3UI

echo "Compiling autostart.c..."
clang -c "$VICE_DIR/src/autostart.c" -o "$VICE_DIR/src/autostart.o" \
    $INCLUDES -DMACOS_COMPILE -D_REENTRANT -UUSE_GTK3UI

echo "Compiling sha1.c..."
clang -c "$VICE_DIR/src/sha1.c" -o "$VICE_DIR/src/sha1.o" \
    $INCLUDES -DMACOS_COMPILE -D_REENTRANT -UUSE_GTK3UI

echo "Compiling vicefeatures.c..."
clang -c "$VICE_DIR/src/vicefeatures.c" -o "$VICE_DIR/src/vicefeatures.o" \
    $INCLUDES -DMACOS_COMPILE -D_REENTRANT -UUSE_GTK3UI

echo "Compiling autostart-prg.c..."
clang -c "$VICE_DIR/src/autostart-prg.c" -o "$VICE_DIR/src/autostart-prg.o" \
    $INCLUDES -DMACOS_COMPILE -D_REENTRANT -UUSE_GTK3UI

echo "Compiling all C64 cartridge files..."
for file in "$VICE_DIR/src/c64/cart/"*.c; do
    filename=$(basename -- "$file")
    name="${filename%.*}"
    # echo "Compiling $filename..."
    clang -c "$file" -o "$VICE_DIR/src/c64/cart/$name.o" \
        $INCLUDES -DMACOS_COMPILE -D_REENTRANT -UUSE_GTK3UI
done

# Update SRC_OBJS to include the newly compiled objects
# Also include objects in subdirectories that were manually compiled
# Include ALL c64/cart/*.o
SRC_OBJS=$(find "$VICE_DIR/src" -maxdepth 1 -name "*.o" ! -name "main.o" ! -name "c1541.o" ! -name "c1541-stubs.o" ! -name "cartconv.o" ! -name "petcat.o" | tr '\n' ' ')
SRC_OBJS="$SRC_OBJS $VICE_DIR/src/c64/c64export.o $(find "$VICE_DIR/src/c64/cart" -name "*.o" | tr '\n' ' ')"

# Include socketdrv.o manually or ensure it is picked up?
# It is in subfolder, so SRC_OBJS won't pick it up.
# I will append it to SRC_OBJS or just add it to the link command.
SOCKETDRV_OBJ="$VICE_DIR/src/arch/shared/socketdrv/socketdrv.o"
SOUNDCORE_OBJ="$VICE_DIR/src/arch/shared/sounddrv/soundcoreaudio.o"
C64KEYBOARD_OBJ="$VICE_DIR/src/c64/c64keyboard.o"



echo "Compiling VICEGameCore.m separately..."
clang -c "$CORE_DIR/VICEGameCore.m" -o "$CORE_DIR/VICEGameCore.o" \
    -isysroot $(xcrun --show-sdk-path) \
    -arch arm64 \
    -fobjc-arc \
    -I"$VICE_DIR/src" \
    -I"$VICE_DIR/src/video" \
    -I"$VICE_DIR/src/c64" \
    -I"$VICE_DIR/src/sid" \
    -I"$VICE_DIR/src/vicii" \
    -I"$VICE_DIR/src/raster" \
    -I"$VICE_DIR/src/monitor" \
    -I"$VICE_DIR/src/lib/p64" \
    -I"$VICE_DIR/src/platform" \
    -I"$VICE_DIR/src/drive" \
    -I"$VICE_DIR/src/vdrive" \
    -I"$VICE_DIR" \
    -I"$VICE_DIR/src/arch/shared" \
    -I"$VICE_DIR/src/arch/shared/hotkeys" \
    -I"$VICE_DIR/src/monitor" \
    -I"$VICE_DIR/src/parallel" \
    -I"$VICE_DIR/src/arch/headless" \
    -I"$VICE_DIR/src/core/rtc" \
    -I"$VICE_DIR/src/joyport" \
    -I"$VICE_DIR/src/hvsc" \
    -I"$PROJECT_DIR/OpenEmu/SystemPlugins/Commodore 64" \
    -I"/Users/barriesanders/.gemini/antigravity/scratch/OpenEmu_Port/OpenEmu-SDK" \
    -D_REENTRANT \
    -DMACOS_COMPILE \
    -UUSE_VICE_THREAD \
    -F"$FRAMEWORK_PATH"

echo "Compiling and Linking..."
clang -bundle -o "$BUILD_DIR/VICE.oecoreplugin/Contents/MacOS/VICE" \
    -isysroot $(xcrun --show-sdk-path) \
    -arch arm64 \
    -fobjc-arc \
    -Wl,-rpath,@executable_path/../Frameworks \
    -I"$VICE_DIR/src" \
    -I"$VICE_DIR/src/video" \
    -I"$VICE_DIR/src/c64" \
    -I"$VICE_DIR/src/sid" \
    -I"$VICE_DIR/src/vicii" \
    -I"$VICE_DIR/src/raster" \
    -I"$VICE_DIR/src/monitor" \
    -I"$VICE_DIR/src/lib/p64" \
    -I"$VICE_DIR/src/platform" \
    -I"$VICE_DIR/src/drive" \
    -I"$VICE_DIR/src/vdrive" \
    -I"$VICE_DIR" \
    -I"$VICE_DIR/src/arch/shared" \
    -I"$VICE_DIR/src/arch/shared/hotkeys" \
    -I"$VICE_DIR/src/monitor" \
    -I"$VICE_DIR/src/parallel" \
    -I"$VICE_DIR/src/arch/headless" \
    -I"$VICE_DIR/src/core/rtc" \
    -I"$VICE_DIR/src/joyport" \
    -I"$VICE_DIR/src/hvsc" \
    -I"$PROJECT_DIR/OpenEmu/SystemPlugins/Commodore 64" \
    -I"/Users/barriesanders/.gemini/antigravity/scratch/OpenEmu_Port/OpenEmu-SDK" \
    -D_REENTRANT \
    -DMACOS_COMPILE \
    -UUSE_VICE_THREAD \
    -F"$FRAMEWORK_PATH" \
    -framework OpenEmuBase \
    -framework Cocoa \
    -framework OpenGL \
    -framework IOKit \
    -framework CoreVideo \
    -framework CoreServices \
    -framework AudioToolbox \
    -framework CoreAudio \
    "$CORE_DIR/VICEGameCore.o" \
    $HEADLESS_SRCS \
    $SRC_OBJS \
    $SHARED_OBJS \
    $SOCKETDRV_OBJ \
    $SOUNDCORE_OBJ \
    $C64KEYBOARD_OBJ \
    $LIBS $LIBS \
    -lstdc++ -lz -liconv

echo "Fixing RPATH for VICE Core Plugin..."
install_name_tool -add_rpath "@loader_path/../../../../../Frameworks" "$BUILD_DIR/VICE.oecoreplugin/Contents/MacOS/VICE"
install_name_tool -add_rpath "@executable_path/../Frameworks" "$BUILD_DIR/VICE.oecoreplugin/Contents/MacOS/VICE"

echo "Signing VICE Core Plugin..."
codesign --force --sign - --deep "$BUILD_DIR/VICE.oecoreplugin"


echo "Installing VICE Core Plugin to ~/Library/Application Support/OpenEmu/Cores..."
mkdir -p "$HOME/Library/Application Support/OpenEmu/Cores"
rm -rf "$HOME/Library/Application Support/OpenEmu/Cores/VICE.oecoreplugin"
cp -R "$BUILD_DIR/VICE.oecoreplugin" "$HOME/Library/Application Support/OpenEmu/Cores/"

echo "Build and Install Complete!"
