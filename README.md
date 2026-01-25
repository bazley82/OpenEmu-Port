# OpenEmu (AI-Powered Apple Silicon Port)

![OpenEmu Screenshot](http://openemu.org/img/intro-md.png)

> [!IMPORTANT]
> **Transparency Disclaimer:** This repository is an experimental port of OpenEmu, created and maintained entirely through **AI-assisted coding** (using "Vibe Coding" techniques). The project was initiated by a user with no formal coding experience to test the capabilities of advanced AI agents (specifically Antigravity) in porting complex legacy software to run natively on Apple Silicon.

## About this Port
This version of OpenEmu has been specifically patched to run natively on Apple Silicon (M1/M2/M3) and includes several build fixes for modern macOS/Xcode environments.

### Key Modifications:
- **Native Apple Silicon Support:** Fixed multiple architecture-specific compilation errors (narrowing conversions, precision loss) in major cores like **Nestopia** and **Mupen64Plus**.
- **C64 Support:** Integrated Commodore 64 system support directly into the app bundle.
- **Permission Fixes:** Resolved the persistent "Input Monitoring" permission loop that affects many users on modern macOS versions. Added an "Ignore" button to permanently suppress these alerts.
- **Flattened Architecture:** Converted all submodules into regular directories to create a standalone, portable repository that avoids the common "broken submodule" issues during cloning.

## Quick Start
You can download the pre-compiled native app from the **[Releases](https://github.com/bazley82/OpenEmu-Port/releases)** section.

---

OpenEmu is an open-source project whose purpose is to bring macOS game emulation into the realm of first-class citizenship. The project leverages modern macOS technologies, such as Cocoa, Metal, Core Animation, and other third-party libraries. 

Currently, OpenEmu can load the following game engines as plugins:
* Atari 2600 ([Stella](https://github.com/stella-emu/stella))
* Atari 5200 ([Atari800](https://github.com/atari800/atari800))
* Atari 7800 ([ProSystem](https://gitlab.com/jgemu/prosystem))
* Atari Lynx ([Mednafen](https://mednafen.github.io))
* ColecoVision ([CrabEmu](https://sourceforge.net/projects/crabemu/))
* Famicom Disk System ([Nestopia](https://gitlab.com/jgemu/nestopia))
* Game Boy / Game Boy Color ([Gambatte](https://gitlab.com/jgemu/gambatte))
* Game Boy Advance ([mGBA](https://github.com/mgba-emu/mgba))
* GameCube ([Dolphin](https://github.com/dolphin-emu/dolphin))
* Game Gear ([Genesis Plus](https://github.com/ekeeke/Genesis-Plus-GX))
* Intellivision ([Bliss](https://github.com/jeremiah-sypult/BlissEmu))
* NeoGeo Pocket ([Mednafen](https://mednafen.github.io))
* Nintendo (NES) / Famicom ([FCEUX](https://github.com/TASEmulators/fceux), [Nestopia](https://gitlab.com/jgemu/nestopia))
* Nintendo 64 ([Mupen64Plus](https://github.com/mupen64plus))
* Nintendo DS ([DeSmuME](https://github.com/TASEmulators/desmume))
* Odyssey² / Videopac+ ([O2EM](https://sourceforge.net/projects/o2em/))
* PC-FX ([Mednafen](https://mednafen.github.io))
* SG-1000 ([Genesis Plus](https://github.com/ekeeke/Genesis-Plus-GX))
* Sega 32X ([picodrive](https://github.com/notaz/picodrive))
* Sega CD / Mega CD ([Genesis Plus](https://github.com/ekeeke/Genesis-Plus-GX))
* Sega Genesis / Mega Drive ([Genesis Plus](https://github.com/ekeeke/Genesis-Plus-GX))
* Sega Master System ([Genesis Plus](https://github.com/ekeeke/Genesis-Plus-GX))
* Sega Saturn ([Mednafen](https://mednafen.github.io))
* Sony PSP ([PPSSPP](https://github.com/hrydgard/ppsspp))
* Sony PlayStation ([Mednafen](https://mednafen.github.io))
* Super Nintendo (SNES) ([BSNES](https://github.com/bsnes-emu/bsnes), [Snes9x](https://github.com/snes9xgit/snes9x))
* TurboGrafx-16 / PC Engine ([Mednafen](https://mednafen.github.io))
* TurboGrafx-CD / PCE-CD ([Mednafen](https://mednafen.github.io))
* Vectrex ([VecXGL](https://github.com/james7780/VecXGL))
* Virtual Boy ([Mednafen](https://mednafen.github.io))
* WonderSwan ([Mednafen](https://mednafen.github.io))
* **Commodore 64 (VICE)** - *Integrated in this port*

## Development
This port was developed collaboratively by **bazley82** and **Antigravity (AI Assistant)**.

## Minimum Requirements
- macOS Mojave 10.14.4 (for general use)
- Apple Silicon (M1/M2/M3) highly recommended for native performance.
