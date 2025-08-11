# ===============================================================
# CMakeLists.txt — Unified Build System for PAC-RF Project
# ===============================================================
cmake_minimum_required(VERSION 3.15)
project(pac_rf LANGUAGES C)

# -------------------------------
# Build Options
# -------------------------------
option(PACRF_BUILD_GUI     "Build GTK4 GUI frontend"              ON)
option(PACRF_WITH_LIBUSB   "Build with libusb hardware features"  ON)

# -------------------------------
# Platform banner
# -------------------------------
if(APPLE)
    set(PLATFORM "macOS")
    message(STATUS "📦 Building for macOS (Local Development)")
elseif(UNIX)
    set(PLATFORM "Linux")
    message(STATUS "🚀 Building for Linux (Production / Embedded)")
else()
    set(PLATFORM "Unknown")
    message(WARNING "⚠️ Platform not explicitly supported")
endif()

# -------------------------------
# Output & Compiler Flags
# -------------------------------
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
# GNU99 keeps PAC-RF's GCC 4.8 happy; fine on clang/mac too
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -std=gnu99")

# -------------------------------
# Optional ARM/NEON notes
# -------------------------------
if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm|aarch64")
    if(UNIX AND NOT APPLE)
        message(STATUS "[SKIP] Generic ARM64 detected — skipping NEON flags for compatibility")
        # set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mfpu=neon -mfloat-abi=hard")
    else()
        message(STATUS "[SKIP] ARM detected, but NEON flags skipped on macOS")
    endif()
endif()

# -------------------------------
# macOS pkg-config path auto-detect (Homebrew / MacPorts)
# -------------------------------
# This avoids having to export PKG_CONFIG_PATH manually on Mac.
if(APPLE)
    # Try Homebrew first: ask brew for its prefix (works on Intel & Apple Silicon)
    find_program(BREW_EXEC brew)
    if(BREW_EXEC)
        execute_process(COMMAND ${BREW_EXEC} --prefix
                        OUTPUT_VARIABLE BREW_PREFIX
                        OUTPUT_STRIP_TRAILING_WHITESPACE)
        if(EXISTS "${BREW_PREFIX}/lib/pkgconfig" OR EXISTS "${BREW_PREFIX}/share/pkgconfig")
            list(APPEND CMAKE_PREFIX_PATH "${BREW_PREFIX}")
            # Merge into PKG_CONFIG_PATH used by pkg-config inside CMake
            set(ADDED_PC_PATHS "")
            if(EXISTS "${BREW_PREFIX}/lib/pkgconfig")
                list(APPEND ADDED_PC_PATHS "${BREW_PREFIX}/lib/pkgconfig")
            endif()
            if(EXISTS "${BREW_PREFIX}/share/pkgconfig")
                list(APPEND ADDED_PC_PATHS "${BREW_PREFIX}/share/pkgconfig")
            endif()
            if(ADDED_PC_PATHS)
                if(DEFINED ENV{PKG_CONFIG_PATH})
                    set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:${ADDED_PC_PATHS}")
                else()
                    # Join list into colon-separated string
                    string (REPLACE ";" ":" ADDED_PC_PATHS_JOINED "${ADDED_PC_PATHS}")
                    set(ENV{PKG_CONFIG_PATH} "${ADDED_PC_PATHS_JOINED}")
                endif()
                message(STATUS "pkg-config search paths (brew): $ENV{PKG_CONFIG_PATH}")
            endif()
        endif()
    else()
        # If brew not found, try common prefixes directly
        foreach(PFX "/opt/homebrew" "/usr/local")
            if(EXISTS "${PFX}/lib/pkgconfig" OR EXISTS "${PFX}/share/pkgconfig")
                if(DEFINED ENV{PKG_CONFIG_PATH})
                    set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:${PFX}/lib/pkgconfig:${PFX}/share/pkgconfig")
                else()
                    set(ENV{PKG_CONFIG_PATH} "${PFX}/lib/pkgconfig:${PFX}/share/pkgconfig")
                endif()
                list(APPEND CMAKE_PREFIX_PATH "${PFX}")
            endif()
        endforeach()
        if(DEFINED ENV{PKG_CONFIG_PATH})
            message(STATUS "pkg-config search paths (fallback): $ENV{PKG_CONFIG_PATH}")
        endif()
    endif()

    # MacPorts fallback
    foreach(PFX "/opt/local")
        if(EXISTS "${PFX}/lib/pkgconfig" OR EXISTS "${PFX}/share/pkgconfig")
            if(DEFINED ENV{PKG_CONFIG_PATH})
                set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:${PFX}/lib/pkgconfig:${PFX}/share/pkgconfig")
            else()
                set(ENV{PKG_CONFIG_PATH} "${PFX}/lib/pkgconfig:${PFX}/share/pkgconfig")
            endif()
            list(APPEND CMAKE_PREFIX_PATH "${PFX}")
        endif()
    endforeach()
endif()

# -------------------------------
# Sources
# -------------------------------
set(SRC_COMMON
    src/common/bit_parser.c
    src/common/commands.c
    src/common/handlers.c
    src/common/interface.c
    src/common/logger.c
    src/common/queue_manager.c
    src/common/nmea.c
)
set(SRC_CLI src/cli/main.c)
set(SRC_GUI src/gui/main_gui.c)

include_directories(${CMAKE_SOURCE_DIR}/include)

# -------------------------------
# Dependencies (pkg-config)
# -------------------------------
find_package(PkgConfig REQUIRED)

# libusb (optional; CLI wires it in if present)
set(LIBUSB_FOUND_TEXT "NO")
if(PACRF_WITH_LIBUSB)
    pkg_check_modules(LIBUSB QUIET libusb-1.0)
    if(LIBUSB_FOUND)
        add_definitions(-DHAVE_LIBUSB=1)
        set(LIBUSB_FOUND_TEXT "${LIBUSB_VERSION}")
        message(STATUS "libusb found: ${LIBUSB_VERSION} — USB features enabled")
    else()
        message(STATUS "libusb not found; building WITHOUT USB features")
    endif()
endif()

# GTK4 (GUI-only; optional so PAC-RF builds headless)
set(GTK4_FOUND_TEXT "NO")
if(PACRF_BUILD_GUI)
    pkg_check_modules(GTK4 QUIET gtk4)
    if(GTK4_FOUND)
        set(GTK4_FOUND_TEXT "${GTK4_VERSION}")
    else()
        message(WARNING "⚠️ GTK4 not found — GUI will be skipped on this platform")
        set(PACRF_BUILD_GUI OFF)
    endif()
endif()

# Threads (GLib / workers)
find_package(Threads REQUIRED)

# -------------------------------
# Core library (shared by CLI & GUI)
# -------------------------------
add_library(pacrf_core STATIC ${SRC_COMMON})
target_include_directories(pacrf_core PUBLIC ${CMAKE_SOURCE_DIR}/include)
# Do NOT force libusb into the core library; only the CLI should depend on it.
# (Keeps GUI independent of libusb on platforms where it's messy.)

# -------------------------------
# CLI target
# -------------------------------
add_executable(pac_rf_exec ${SRC_CLI})
target_link_libraries(pac_rf_exec PRIVATE pacrf_core Threads::Threads)
# If libusb available, add flags/libs *to CLI only*
if(PACRF_WITH_LIBUSB AND LIBUSB_FOUND)
    target_include_directories(pac_rf_exec PRIVATE ${LIBUSB_INCLUDE_DIRS})
    if(LIBUSB_LIBRARY_DIRS)
        target_link_directories(pac_rf_exec PRIVATE ${LIBUSB_LIBRARY_DIRS})
    endif()
    target_link_libraries(pac_rf_exec PRIVATE ${LIBUSB_LIBRARIES})
    if(LIBUSB_CFLAGS_OTHER)
        target_compile_options(pac_rf_exec PRIVATE ${LIBUSB_CFLAGS_OTHER})
    endif()
endif()

# -------------------------------
# GUI target (only if GTK4 present & requested)
# -------------------------------
if(PACRF_BUILD_GUI)
    add_executable(pac_rf_gui ${SRC_GUI})
    target_include_directories(pac_rf_gui PRIVATE ${GTK4_INCLUDE_DIRS})
    if(GTK4_LIBRARY_DIRS)
        target_link_directories(pac_rf_gui PRIVATE ${GTK4_LIBRARY_DIRS})
    endif()
    target_link_libraries(pac_rf_gui PRIVATE pacrf_core Threads::Threads ${GTK4_LIBRARIES})
    if(GTK4_CFLAGS_OTHER)
        target_compile_options(pac_rf_gui PRIVATE ${GTK4_CFLAGS_OTHER})
    endif()
endif()

# -------------------------------
# Build Profile Summary
# -------------------------------
message(STATUS "")
message(STATUS "================ Build Profile ================")
message(STATUS "Target Platform        : ${PLATFORM}")
message(STATUS "Architecture           : ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "CLI Target             : ✔ Built (pac_rf_exec)")
if(PACRF_BUILD_GUI)
    message(STATUS "GUI Target             : ✔ Built (pac_rf_gui)")
else()
    message(STATUS "GUI Target             : ✖ Skipped (GTK4 not found or disabled)")
endif()
if(PACRF_WITH_LIBUSB AND LIBUSB_FOUND)
    message(STATUS "libusb Found           : ${LIBUSB_FOUND_TEXT}")
elseif(PACRF_WITH_LIBUSB AND NOT LIBUSB_FOUND)
    message(STATUS "libusb Found           : NO (USB features off)")
else()
    message(STATUS "libusb Support         : ✖ Disabled")
endif()
message(STATUS "NEON Optimization      : ${CMAKE_C_FLAGS}")
message(STATUS "===============================================")
