package org.openemu.android

import java.util.Locale

object RomSystemIdentifier {

    /**
     * @param systemName   Human-readable name shown in the sidebar
     * @param coreName     Android library name (for System.loadLibrary, stubs)
     * @param libretroSo   Pre-built Libretro .so filename WITHOUT path (null = stub only)
     * @param extensions   ROM file extensions (lowercase, no dot)
     */
    data class SystemInfo(
        val systemName: String,
        val coreName: String,
        val libretroSo: String?,
        val extensions: List<String>
    )

    private val systems = listOf(
        // ── Beta 8: Pre-built Libretro cores ────────────────────────────────
        SystemInfo("Game Boy",        "libretro_bridge", "gambatte_libretro_android.so",             listOf("gb")),
        SystemInfo("Game Boy Color",  "libretro_bridge", "gambatte_libretro_android.so",             listOf("gbc")),
        SystemInfo("NES",             "libretro_bridge", "nestopia_libretro_android.so",             listOf("nes", "fds", "unf", "unif")),
        SystemInfo("Nintendo 64",     "libretro_bridge", "mupen64plus_next_gles3_libretro_android.so", listOf("n64", "z64", "v64")),
        // ── Stub cores (wired in future betas) ──────────────────────────────
        SystemInfo("Super Nintendo",   "snes9x",      null, listOf("sfc", "smc", "fig")),
        SystemInfo("Game Boy Advance", "mgba",        null, listOf("gba")),
        SystemInfo("Nintendo DS",      "desmume",     null, listOf("nds")),
        SystemInfo("Sega Genesis",     "genesisplus", null, listOf("md", "gen", "smd", "bin")),
        SystemInfo("Sega Master System","genesisplus", null, listOf("sms")),
        SystemInfo("Sega Game Gear",   "genesisplus", null, listOf("gg")),
        SystemInfo("Sega Saturn",      "mednafen",    null, listOf("cue")),
        SystemInfo("Sony PlayStation", "mednafen",    null, listOf("img", "chd")),
        SystemInfo("Atari 2600",       "stella",      null, listOf("a26")),
        SystemInfo("Atari 5200",       "atari800",    null, listOf("a52")),
        SystemInfo("Atari 7800",       "prosystem",   null, listOf("a78")),
        SystemInfo("Sega 32X",         "picodrive",   null, listOf("32x")),
        SystemInfo("3DO",              "4do",         null, listOf("iso")),
        SystemInfo("Pokémon Mini",     "pokemini",    null, listOf("min")),
        SystemInfo("Odyssey²",         "o2em",        null, listOf("o2")),
        SystemInfo("Vectrex",          "vecxgl",      null, listOf("vec"))
    )

    fun identify(fileName: String): SystemInfo? {
        val extension = fileName.substringAfterLast(".", "").lowercase(Locale.ROOT)
        if (extension.isEmpty()) return null
        return systems.find { it.extensions.contains(extension) }
    }

    fun getAllSupportedExtensions(): List<String> =
        systems.flatMap { it.extensions }.distinct()
}
