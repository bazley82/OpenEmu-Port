package org.openemu.android

import java.util.Locale

object RomSystemIdentifier {
    
    data class SystemInfo(
        val systemName: String,
        val coreName: String,
        val extensions: List<String>
    )

    private val systems = listOf(
        SystemInfo("Nintendo Entertainment System", "nestopia", listOf("nes", "fds")),
        SystemInfo("Super Nintendo", "snes9x", listOf("sfc", "smc")),
        SystemInfo("Game Boy Advance", "mgba", listOf("gba")),
        SystemInfo("Game Boy / Color", "gambatte", listOf("gb", "gbc")),
        SystemInfo("Nintendo 64", "mupen64plus", listOf("n64", "z64", "v64")),
        SystemInfo("Nintendo DS", "desmume", listOf("nds")),
        SystemInfo("Sega Genesis", "genesisplus", listOf("md", "gen", "smd")),
        SystemInfo("Sega Master System", "genesisplus", listOf("sms")),
        SystemInfo("Sega Game Gear", "genesisplus", listOf("gg")),
        SystemInfo("Sega Saturn", "mednafen_saturn", listOf("cue")),
        SystemInfo("Sony PlayStation", "mednafen_psx", listOf("cue", "bin", "img")),
        SystemInfo("Atari 2600", "stella", listOf("a26")),
        SystemInfo("Atari 5200", "atari800", listOf("a52")),
        SystemInfo("Atari 7800", "prosystem", listOf("a78")),
        SystemInfo("Sega 32X", "picodrive", listOf("32x")),
        SystemInfo("3DO", "4do", listOf("iso")),
        SystemInfo("Pokémon Mini", "pokemini", listOf("min")),
        SystemInfo("Odyssey²", "o2em", listOf("bin")),
        SystemInfo("Vectrex", "vecxgl", listOf("vec"))
    )

    fun identify(fileName: String): SystemInfo? {
        val extension = fileName.substringAfterLast(".", "").lowercase(Locale.ROOT)
        if (extension.isEmpty()) return null
        
        // Specific disambiguation (e.g., .bin could be PSX or O2EM)
        // For Beta 6, we'll prioritize PSX for .bin if it's large, but here we just return the first match or use a heuristic.
        // For simplicity, we search for the extension.
        
        return systems.find { it.extensions.contains(extension) }
    }

    fun getAllSupportedExtensions(): List<String> {
        return systems.flatMap { it.extensions }.distinct()
    }
}
