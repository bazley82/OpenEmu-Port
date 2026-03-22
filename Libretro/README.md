# OpenEmu Libretro Bridge

This directory contains a unified Libretro bridge for OpenEmu, allowing a single translation layer to support multiple emulation cores.

## Supported Systems
- **Super Nintendo** (Standard RetroPad mapping)
- **Sega Genesis** (6-button mapping)
- **PlayStation** (via **SwanStation**)
- **Nintendo DS** (via **DeSmuME**)
- **Sony PSP** (via **PPSSPP**)
- **Nintendo 64** (via **Mupen64Plus-Next**)

## Project Structure
- `Core/`: Contains `OELibretroGameCore.mm` and its dependencies.
- `Plugins/`: Contains the `.oecoreplugin` bundle wrappers for each system.
- `Cores/`: Cloned Libretro core repositories for building.
- `Shaders/`: Libretro Slang shaders for enhanced visuals.

## Building and Packaging

A `Makefile` is provided to automate the compilation and packaging of the core plugins.

1. **Compile and Package**:
   ```bash
   make all
   ```
   This will:
   - Compile `OELibretroGameCore.mm` into `OELibretroGameCore.dylib`.
   - Create the MacOS executable for each plugin by copying and renaming the dylib.
   - Set the correct `install_name` for each plugin.

2. **Core Injection**:
   For each plugin (e.g., `PSX.oecoreplugin`), place the actual Libretro core dylib (e.g., `swanstation_libretro.dylib`) into:
   `Contents/Resources/libretro_core.dylib`

3. **Installation**:
   Copy the `.oecoreplugin` folders to your OpenEmu `Plugins/Cores` directory.

## Features
- **Metal Hardware Rendering**: Support for `RETRO_ENVIRONMENT_SET_HW_RENDER` with Metal synchronization.
- **VFS Support**: Integrated `libretro-common` VFS for robust file I/O.
- **Flexible Input**: System-specific mapping for all major consoles.
- **State Management**: Implemented `saveState` and `loadState` via Libretro serialization.
