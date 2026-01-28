# OpenEmu (AI-Powered Apple Silicon Port)

![OpenEmu Screenshot](http://openemu.org/img/intro-md.png)

> [!IMPORTANT]
> **Transparency Disclaimer:** This repository is an experimental port of OpenEmu, created and maintained entirely through **AI-assisted coding** (using "Vibe Coding" techniques). The project was initiated by a user with no formal coding experience to test the capabilities of advanced AI agents (specifically Antigravity) in porting complex legacy software to run natively on Apple Silicon.

## About this Port
This version of OpenEmu has been specifically patched to run natively on Apple Silicon (M1/M2/M3) and includes several build fixes for modern macOS/Xcode environments.

### Key Modifications:
- **Full ARM64 Suite:** Successfully ported and verified all **25 emulation cores** (Nestopia, BSNES, Mupen64Plus, Snes9x, DeSmuME, Genesis Plus GX, etc.) for native Apple Silicon compatibility.
- **Modern Build Standards:** Updated all projects to `MACOSX_DEPLOYMENT_TARGET = 11.0` and resolved hundreds of narrowing conversion and linkage errors.
- **C64 Support:** Integrated Commodore 64 system support directly into the app bundle.
- **Permission Fixes:** Resolved the persistent "Input Monitoring" permission loop that affects many users on modern macOS versions.
- **Flattened Architecture:** Converted all submodules into regular directories to create a standalone, portable repository.
- **Custom Design:** Features a new, high-resolution "Liquid Glass" application icon, optimized for macOS Big Sur and Monterey/Ventura/Sonoma aesthetics.

## Quick Start
You can download the pre-compiled native app from the **[Releases](https://github.com/bazley82/OpenEmu-Port/releases)** section.

---

OpenEmu is an open-source project whose purpose is to bring macOS game emulation into the realm of first-class citizenship. The project leverages modern macOS technologies, such as Cocoa, Metal, Core Animation, and other third-party libraries. 

Currently, OpenEmu can load the following game engines as plugins:
* Atari 2600 ([Stella](https://github.com/stella-emu/stella)) - **[FIXED FOR ARM64]**
* Atari 5200 ([Atari800](https://github.com/atari800/atari800)) - **[FIXED FOR ARM64]**
* Atari 7800 ([ProSystem](https://gitlab.com/jgemu/prosystem)) - **[FIXED FOR ARM64]**
* Atari Lynx ([Mednafen](https://mednafen.github.io)) - **[FIXED FOR ARM64]**
* ColecoVision ([JollyCV](https://github.com/OpenEmu/JollyCV-Core)) - **[FIXED FOR ARM64]**
* Famicom Disk System ([Nestopia](https://gitlab.com/jgemu/nestopia)) - **[FIXED FOR ARM64]**
* Game Boy / Game Boy Color ([Gambatte](https://gitlab.com/jgemu/gambatte)) - **[FIXED FOR ARM64]**
* Game Boy Advance ([mGBA](https://github.com/mgba-emu/mgba)) - **[FIXED FOR ARM64]**
* Game Gear ([Genesis Plus](https://github.com/ekeeke/Genesis-Plus-GX)) - **[FIXED FOR ARM64]**
* Intellivision ([Bliss](https://github.com/jeremiah-sypult/BlissEmu)) - **[FIXED FOR ARM64]**
* Nintendo (NES) / Famicom ([Nestopia](https://gitlab.com/jgemu/nestopia), [FCEU](https://github.com/TASEmulators/fceux)) - **[FIXED FOR ARM64]**
* Nintendo 64 ([Mupen64Plus](https://github.com/mupen64plus)) - **[NATIVE SUPPORT VERIFIED]**
* Nintendo DS ([DeSmuME](https://github.com/TASEmulators/desmume)) - **[FIXED FOR ARM64]**
* Odyssey² / Videopac+ ([O2EM](https://sourceforge.net/projects/o2em/)) - **[FIXED FOR ARM64]**
* Sega 32X ([picodrive](https://github.com/notaz/picodrive)) - **[FIXED FOR ARM64]**
* Sega CD / Mega CD ([Genesis Plus](https://github.com/ekeeke/Genesis-Plus-GX)) - **[FIXED FOR ARM64]**
* Sega Genesis / Mega Drive ([Genesis Plus](https://github.com/ekeeke/Genesis-Plus-GX)) - **[FIXED FOR ARM64]**
* Sega Master System ([Genesis Plus](https://github.com/ekeeke/Genesis-Plus-GX)) - **[FIXED FOR ARM64]**
* Sega Saturn ([Mednafen](https://mednafen.github.io)) - **[FIXED FOR ARM64]**
* Sony PlayStation ([Mednafen](https://mednafen.github.io)) - **[FIXED FOR ARM64]**
* Super Nintendo (SNES) ([BSNES](https://github.com/bsnes-emu/bsnes), [Snes9x](https://github.com/snes9xgit/snes9x)) - **[FIXED FOR ARM64]**
* Vectrex ([VecXGL](https://github.com/james7780/VecXGL)) - **[FIXED FOR ARM64]**
* 3DO ([4DO](https://github.com/fourdo/fourdo)) - **[FIXED FOR ARM64]**
* Pokémon Mini ([PokeMini](https://github.com/pokerazor/pokemini)) - **[FIXED FOR ARM64]**
* WonderSwan ([Mednafen](https://mednafen.github.io)) - **[FIXED FOR ARM64]**
* **Commodore 64 (VICE)** - **[FIXED FOR ARM64]**

## Development
This port was developed collaboratively by **bazley82** and **Antigravity (AI Assistant)**.

## Minimum Requirements
- macOS Mojave 10.14.4 (for general use)
- Apple Silicon (M1/M2/M3) highly recommended for native performance.
