cmake_minimum_required(VERSION 3.22.1)

project("openemu_cores")

# Shared Core Source Path
set(SHARED_CORES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../../Shared-Cores")

# mGBA Core (Example Integration)
# Note: Real Libretro cores often require specific defines and platform-specific source selection.
set(MGBA_DIR "${SHARED_CORES_DIR}/mGBA")

# Mocking the mGBA integration for the milestone
add_library(mgba SHARED 
    "mgmt_interface.cpp" # This will be our JNI bridge
)

target_include_directories(mgba PRIVATE "${MGBA_DIR}/src" "${MGBA_DIR}/include")
target_link_libraries(mgba log android)
