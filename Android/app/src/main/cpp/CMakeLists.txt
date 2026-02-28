cmake_minimum_required(VERSION 3.22.1)

project("openemu_cores")

set(SHARED_CORES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../Shared-Cores")

# ─────────────────────────────────────────────────────────────────────────────
# BETA 8 — Pre-built Libretro cores shipped as jniLibs; no source compilation.
#
# The three real cores (Gambatte, Nestopia, Mupen64Plus-Next) are official
# arm64-v8a .so files from buildbot.libretro.com and live in:
#   app/src/main/jniLibs/arm64-v8a/
#
# They are loaded at runtime via dlopen() inside libretro_bridge.cpp.
# ─────────────────────────────────────────────────────────────────────────────

# ── Unified Libretro bridge (dlopen frontend) ─────────────────────────────
add_library(libretro_bridge SHARED "libretro_bridge.cpp")
target_compile_options(libretro_bridge PRIVATE -std=c++17 -O2 -fvisibility=hidden)
target_link_libraries(libretro_bridge log android dl)

# ─────────────────────────────────────────────────────────────────────────────
# STUB CORES (remaining 15 — JNI dispatch only, to be wired in future betas)
# ─────────────────────────────────────────────────────────────────────────────

macro(add_stub_core CORE_NAME INCLUDE_DIR)
    add_library(${CORE_NAME} SHARED "mgmt_interface.cpp")
    target_include_directories(${CORE_NAME} PRIVATE ${INCLUDE_DIR})
    target_link_libraries(${CORE_NAME} log android)
endmacro()

add_stub_core(mgba         "${SHARED_CORES_DIR}/mGBA/src")
add_stub_core(snes9x       "${SHARED_CORES_DIR}/SNES9x/src")
add_stub_core(desmume      "${SHARED_CORES_DIR}/DeSmuME/desmume/src")
add_stub_core(genesisplus  "${SHARED_CORES_DIR}/GenesisPlus/libretro")
add_stub_core(mednafen     "${SHARED_CORES_DIR}/Mednafen")
add_stub_core(bsnes        "${SHARED_CORES_DIR}/BSNES")
add_stub_core(fceu         "${SHARED_CORES_DIR}/FCEU")
add_stub_core(stella       "${SHARED_CORES_DIR}/Stella")
add_stub_core(atari800     "${SHARED_CORES_DIR}/Atari800")
add_stub_core(prosystem    "${SHARED_CORES_DIR}/ProSystem")
add_stub_core(picodrive    "${SHARED_CORES_DIR}/picodrive")
add_stub_core(pokemini     "${SHARED_CORES_DIR}/PokeMini")
add_stub_core(o2em         "${SHARED_CORES_DIR}/O2EM")
add_stub_core(vecxgl       "${SHARED_CORES_DIR}/VecXGL")
add_stub_core(4do          "${SHARED_CORES_DIR}/4DO")
