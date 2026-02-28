cmake_minimum_required(VERSION 3.22.1)

project("openemu_cores")

# Shared Core Source Path
set(SHARED_CORES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../Shared-Cores")

# Helper for adding cores
macro(add_core CORE_NAME CORE_DIR INCLUDE_DIRS)
    add_library(${CORE_NAME} SHARED 
        "mgmt_interface.cpp" # Every core needs the JNI interface for now
        # Add core-specific sources here if not using subdirectories
    )
    target_include_directories(${CORE_NAME} PRIVATE ${INCLUDE_DIRS})
    target_link_libraries(${CORE_NAME} log android)
endmacro()

# 1. mGBA
add_library(mgba SHARED "mgmt_interface.cpp")
target_include_directories(mgba PRIVATE "${SHARED_CORES_DIR}/mGBA/src")
target_compile_definitions(mgba PRIVATE CORE_GBA)
target_link_libraries(mgba log android)

# 2. Nestopia
add_library(nestopia SHARED "mgmt_interface.cpp")
target_include_directories(nestopia PRIVATE "${SHARED_CORES_DIR}/Nestopia/core" "${SHARED_CORES_DIR}/Nestopia/core/api")
target_compile_definitions(nestopia PRIVATE CORE_NES)
target_link_libraries(nestopia log android)

# 3. Snes9x
add_library(snes9x SHARED "mgmt_interface.cpp")
target_include_directories(snes9x PRIVATE "${SHARED_CORES_DIR}/SNES9x/src")
target_compile_definitions(snes9x PRIVATE CORE_SNES)
target_link_libraries(snes9x log android)

# ... (Apply to all targets)

# 4. Mupen64Plus
add_library(mupen64plus SHARED "mgmt_interface.cpp")
target_include_directories(mupen64plus PRIVATE "${SHARED_CORES_DIR}/Mupen64Plus/mupen64plus-core/src")
target_link_libraries(mupen64plus log android)

# 5. DeSmuME
add_library(desmume SHARED "mgmt_interface.cpp")
target_include_directories(desmume PRIVATE "${SHARED_CORES_DIR}/DeSmuME/desmume/src")
target_link_libraries(desmume log android)

# 6. GenesisPlus
add_library(genesisplus SHARED "mgmt_interface.cpp")
target_include_directories(genesisplus PRIVATE "${SHARED_CORES_DIR}/GenesisPlus/libretro")
target_link_libraries(genesisplus log android)

# ... (Continuing for all 18)

# 7. Mednafen (Shared)
add_library(mednafen SHARED "mgmt_interface.cpp")
target_include_directories(mednafen PRIVATE "${SHARED_CORES_DIR}/Mednafen")
target_compile_definitions(mednafen PRIVATE CORE_MEDNAFEN)
target_link_libraries(mednafen log android)

# 8. BSNES
add_library(bsnes SHARED "mgmt_interface.cpp")
target_include_directories(bsnes PRIVATE "${SHARED_CORES_DIR}/BSNES")
target_link_libraries(bsnes log android)

# 9. FCEU
add_library(fceu SHARED "mgmt_interface.cpp")
target_include_directories(fceu PRIVATE "${SHARED_CORES_DIR}/FCEU")
target_link_libraries(fceu log android)

# 10. Stella
add_library(stella SHARED "mgmt_interface.cpp")
target_include_directories(stella PRIVATE "${SHARED_CORES_DIR}/Stella")
target_link_libraries(stella log android)

# 11. Atari800
add_library(atari800 SHARED "mgmt_interface.cpp")
target_include_directories(atari800 PRIVATE "${SHARED_CORES_DIR}/Atari800")
target_link_libraries(atari800 log android)

# 12. ProSystem
add_library(prosystem SHARED "mgmt_interface.cpp")
target_include_directories(prosystem PRIVATE "${SHARED_CORES_DIR}/ProSystem")
target_link_libraries(prosystem log android)

# 13. Gambatte
add_library(gambatte SHARED "mgmt_interface.cpp")
target_include_directories(gambatte PRIVATE "${SHARED_CORES_DIR}/Gambatte")
target_link_libraries(gambatte log android)

# 14. picodrive
add_library(picodrive SHARED "mgmt_interface.cpp")
target_include_directories(picodrive PRIVATE "${SHARED_CORES_DIR}/picodrive")
target_link_libraries(picodrive log android)

# 15. 4DO
add_library(4do SHARED "mgmt_interface.cpp")
target_include_directories(4do PRIVATE "${SHARED_CORES_DIR}/4DO")
target_link_libraries(4do log android)

# 16. PokeMini
add_library(pokemini SHARED "mgmt_interface.cpp")
target_include_directories(pokemini PRIVATE "${SHARED_CORES_DIR}/PokeMini")
target_link_libraries(pokemini log android)

# 17. O2EM
add_library(o2em SHARED "mgmt_interface.cpp")
target_include_directories(o2em PRIVATE "${SHARED_CORES_DIR}/O2EM")
target_link_libraries(o2em log android)

# 18. VecXGL
add_library(vecxgl SHARED "mgmt_interface.cpp")
target_include_directories(vecxgl PRIVATE "${SHARED_CORES_DIR}/VecXGL")
target_link_libraries(vecxgl log android)
