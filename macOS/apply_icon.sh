#!/bin/bash

ICON_SOURCE="/Users/barriesanders/.gemini/antigravity/brain/999e1540-95b2-4135-8047-485122022aa2/icon_preview_512.png"
ICONSET_PATH="/Users/barriesanders/.gemini/antigravity/scratch/OpenEmu_Port/OpenEmu/Graphics.xcassets/OpenEmu.appiconset"

echo "Generating icons from $ICON_SOURCE..."

# Function to resize and copy
generate_icon() {
    local size=$1
    local name=$2
    sips -z $size $size "$ICON_SOURCE" --out "$ICONSET_PATH/$name-srgb.png" > /dev/null
    sips -z $size $size "$ICON_SOURCE" --out "$ICONSET_PATH/$name-p3.png" > /dev/null
}

generate_icon 16 "icon-16"
generate_icon 32 "icon-32"
generate_icon 64 "icon-64"
generate_icon 128 "icon-128"
generate_icon 256 "icon-256"
generate_icon 512 "icon-512"
generate_icon 1024 "icon-1024"

echo "Icons generated successfully in $ICONSET_PATH"
