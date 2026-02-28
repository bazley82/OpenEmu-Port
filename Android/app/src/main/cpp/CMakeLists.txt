cmake_minimum_required(VERSION 3.22.1)

project("openemu_cores")

set(SHARED_CORES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../Shared-Cores")

# ─────────────────────────────────────────────────────────────────────────────
# REAL CORES (Beta 7 — fully linked)
# ─────────────────────────────────────────────────────────────────────────────

# ── 1. Gambatte (Game Boy / Color) ─────────────────────────────────────────
set(GAMBATTE_SRC_DIR "${SHARED_CORES_DIR}/Gambatte/src")

file(GLOB_RECURSE GAMBATTE_SOURCES
    "${GAMBATTE_SRC_DIR}/libgambatte/*.cpp"
    "${GAMBATTE_SRC_DIR}/resample/*.cpp"
)

add_library(gambatte SHARED
    "gambatte_core.cpp"
    ${GAMBATTE_SOURCES}
)
target_include_directories(gambatte PRIVATE
    "${GAMBATTE_SRC_DIR}"
    "${GAMBATTE_SRC_DIR}/libgambatte"
    "${GAMBATTE_SRC_DIR}/resample"
)
target_compile_definitions(gambatte PRIVATE HAVE_STDINT_H)
target_compile_options(gambatte PRIVATE -std=c++11 -O2 -fvisibility=hidden)
target_link_libraries(gambatte log android)

# ── 2. Nestopia (NES / Famicom) ────────────────────────────────────────────
set(NESTOPIA_SRC_DIR "${SHARED_CORES_DIR}/Nestopia/core")

file(GLOB_RECURSE NESTOPIA_SOURCES
    "${NESTOPIA_SRC_DIR}/*.cpp"
)
# Exclude macOS-specific ObjC++ files if any exist
list(FILTER NESTOPIA_SOURCES EXCLUDE REGEX ".*\\.mm$")

add_library(nestopia SHARED
    "nestopia_core.cpp"
    ${NESTOPIA_SOURCES}
)
target_include_directories(nestopia PRIVATE
    "${NESTOPIA_SRC_DIR}"
    "${NESTOPIA_SRC_DIR}/api"
    "${NESTOPIA_SRC_DIR}/board"
    "${NESTOPIA_SRC_DIR}/input"
)
target_compile_definitions(nestopia PRIVATE
    NST_NO_ZLIB
    NDEBUG
)
target_compile_options(nestopia PRIVATE -std=c++11 -O2 -fexceptions -frtti -Wno-narrowing)
target_link_libraries(nestopia log android)

# ── 3. Mupen64Plus (Nintendo 64) ───────────────────────────────────────────
# NOTE: Mupen64Plus requires its full plugin ecosystem (video, audio, RSP) and
# external libs (xxhash, md5) that are not in this repository.
# The mupen64plus_core.cpp bridge is shipped as a structured JNI hook that
# integrates cleanly when the plugin runtime is available. For Beta 7 we link
# only the C API headers as a compile-only stub so the .so is present.
set(M64P_SRC_DIR "${SHARED_CORES_DIR}/Mupen64Plus/mupen64plus-core/src")

add_library(mupen64plus SHARED "mupen64plus_core.cpp")
target_include_directories(mupen64plus PRIVATE
    "${M64P_SRC_DIR}"
    "${M64P_SRC_DIR}/api"
)
target_compile_definitions(mupen64plus PRIVATE
    ANDROID
    MUPEN64PLUS_VERSION=0x00020000
)
target_link_libraries(mupen64plus log android)

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
