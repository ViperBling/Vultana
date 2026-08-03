include_guard(GLOBAL)

# Keep all manifest-mode packages in the repository, but outside Git.  This
# directory is the only VCPKG_INSTALLED_DIR used by this project; packages in
# vcpkg's global installed/<triplet> directory are never a CMake search root.
get_filename_component(VULTANA_SOURCE_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

set(VULTANA_VCPKG_INSTALLED_DIR "${VULTANA_SOURCE_ROOT}/.vcpkg_installed" CACHE PATH
    "Project-local directory for vcpkg manifest packages")
set(VCPKG_MANIFEST_MODE ON CACHE BOOL "Enable vcpkg manifest mode")
set(VCPKG_MANIFEST_DIR "${VULTANA_SOURCE_ROOT}" CACHE PATH "vcpkg manifest directory")
set(VCPKG_INSTALLED_DIR "${VULTANA_VCPKG_INSTALLED_DIR}" CACHE PATH
    "Do not use vcpkg's global installed directory for Vultana" FORCE)
set(VCPKG_TARGET_TRIPLET "x64-windows" CACHE STRING "vcpkg target triplet")

# CMake Tools can provide CMAKE_TOOLCHAIN_FILE itself.  Otherwise, locate a
# regular vcpkg installation through VCPKG_ROOT or the PATH.  The latter makes
# a fresh VS Code CMake Configure work without a machine-specific preset.
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE OR CMAKE_TOOLCHAIN_FILE STREQUAL "")
    set(VULTANA_VCPKG_ROOT "" CACHE PATH "Root directory of the vcpkg installation")

    if(NOT VULTANA_VCPKG_ROOT AND DEFINED ENV{VCPKG_ROOT})
        set(VULTANA_VCPKG_ROOT "$ENV{VCPKG_ROOT}")
    endif()

    if(NOT VULTANA_VCPKG_ROOT)
        find_program(VULTANA_VCPKG_EXECUTABLE NAMES vcpkg vcpkg.exe)
        if(VULTANA_VCPKG_EXECUTABLE)
            get_filename_component(VULTANA_VCPKG_ROOT "${VULTANA_VCPKG_EXECUTABLE}" DIRECTORY)
        endif()
    endif()

    if(NOT VULTANA_VCPKG_ROOT OR
       NOT EXISTS "${VULTANA_VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
        message(FATAL_ERROR
            "Vultana requires vcpkg. Set the VCPKG_ROOT environment variable "
            "or the VULTANA_VCPKG_ROOT CMake cache variable to the vcpkg root, "
            "then configure again.")
    endif()

    set(CMAKE_TOOLCHAIN_FILE
        "${VULTANA_VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE FILEPATH "vcpkg toolchain file")
endif()

# The project standardizes on Ninja.  CMake Tools normally supplies it through
# the selected MSVC kit, but a plain CMake Configure may not inherit that PATH.
# In that case, use the Ninja executable bundled with the newest VS instance.
if(CMAKE_GENERATOR STREQUAL "Ninja" AND
   (NOT DEFINED CMAKE_MAKE_PROGRAM OR
    CMAKE_MAKE_PROGRAM STREQUAL "" OR
    CMAKE_MAKE_PROGRAM MATCHES "-NOTFOUND$"))
    find_program(VULTANA_NINJA_EXECUTABLE NAMES ninja ninja.exe)

    if(NOT VULTANA_NINJA_EXECUTABLE)
        # "$ENV{ProgramFiles(x86)}" cannot be spelled directly: CMP0053 rejects
        # '(' in a variable name, which makes the whole file a syntax error.
        # Look the environment variable up through its name instead.
        set(VULTANA_PROGRAM_FILES_X86_VAR "ProgramFiles(x86)")

        find_program(VULTANA_VSWHERE_EXECUTABLE NAMES vswhere vswhere.exe
            HINTS
                "$ENV{${VULTANA_PROGRAM_FILES_X86_VAR}}/Microsoft Visual Studio/Installer"
                "$ENV{ProgramFiles}/Microsoft Visual Studio/Installer")

        if(VULTANA_VSWHERE_EXECUTABLE)
            execute_process(
                COMMAND "${VULTANA_VSWHERE_EXECUTABLE}" -latest -products *
                        -requires Microsoft.Component.MSBuild -property installationPath
                OUTPUT_VARIABLE VULTANA_VS_INSTALLATION_PATH
                OUTPUT_STRIP_TRAILING_WHITESPACE)

            if(VULTANA_VS_INSTALLATION_PATH)
                set(VULTANA_VS_NINJA
                    "${VULTANA_VS_INSTALLATION_PATH}/Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe")
                if(EXISTS "${VULTANA_VS_NINJA}")
                    set(VULTANA_NINJA_EXECUTABLE "${VULTANA_VS_NINJA}")
                endif()
            endif()
        endif()
    endif()

    if(NOT VULTANA_NINJA_EXECUTABLE)
        message(FATAL_ERROR
            "The x64-windows presets require Ninja. Install Ninja or the Visual "
            "Studio CMake tools component, or select a VS Code kit that provides Ninja.")
    endif()

    set(CMAKE_MAKE_PROGRAM "${VULTANA_NINJA_EXECUTABLE}" CACHE FILEPATH
        "Ninja executable selected by Vultana" FORCE)
endif()
