find_package(Vulkan REQUIRED)
find_package(Stb REQUIRED)

add_library(tinyxml2 STATIC)
target_sources(tinyxml2 PRIVATE 
    tinyxml2/tinyxml2.h
    tinyxml2/tinyxml2.cpp)
target_include_directories(tinyxml2 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/tinyxml2)
set_target_properties(tinyxml2 PROPERTIES FOLDER External CXX_STANDARD 20)

file (GLOB_RECURSE ImGUI_Head ${CMAKE_CURRENT_SOURCE_DIR}/ImGui/*.h)
file (GLOB_RECURSE ImGUI_Src ${CMAKE_CURRENT_SOURCE_DIR}/ImGui/*.cpp)

add_library(ImGui STATIC)
target_sources(ImGui PRIVATE ${ImGUI_Head} ${ImGUI_Src})
target_include_directories(ImGui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/ImGui)
set_target_properties(ImGui PROPERTIES FOLDER External CXX_STANDARD 20)

add_library(OffsetAllocator STATIC)
target_sources(OffsetAllocator PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/OffsetAllocator/OffsetAllocator.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/OffsetAllocator/OffsetAllocator.cpp)
target_include_directories(OffsetAllocator PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/OffsetAllocator)
set_target_properties(OffsetAllocator PROPERTIES FOLDER External CXX_STANDARD 20)

file (GLOB_RECURSE RPMalloc_Head ${CMAKE_CURRENT_SOURCE_DIR}/rpmalloc/*.h)
file (GLOB_RECURSE RPMalloc_Src ${CMAKE_CURRENT_SOURCE_DIR}/rpmalloc/*.c)

add_library(RPMalloc STATIC)
target_sources(RPMalloc PRIVATE ${RPMalloc_Head} ${RPMalloc_Src})
target_include_directories(RPMalloc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/rpmalloc)
set_target_properties(RPMalloc PROPERTIES FOLDER External CXX_STANDARD 20)

add_library(ImFileDialog STATIC ${CMAKE_CURRENT_SOURCE_DIR}/ImFileDialog/ImFileDialog.cpp)
target_include_directories(ImFileDialog PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/ImFileDialog 
    ${CMAKE_CURRENT_SOURCE_DIR}/ImGui
    ${Stb_INCLUDE_DIR}
)
target_link_libraries(ImFileDialog PUBLIC ImGui)
set_target_properties(ImFileDialog PROPERTIES FOLDER External CXX_STANDARD 20)

file (GLOB_RECURSE ImGuiZmo_Head ${CMAKE_CURRENT_SOURCE_DIR}/ImGuizmo/*.h)
file (GLOB_RECURSE ImGuiZmo_Src ${CMAKE_CURRENT_SOURCE_DIR}/ImGuizmo/*.cpp)
add_library(ImGuizmo STATIC)
target_sources(ImGuizmo PRIVATE ${ImGuiZmo_Head} ${ImGuiZmo_Src})
target_include_directories(ImGuizmo PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/ImGuizmo 
    ${CMAKE_CURRENT_SOURCE_DIR}/ImGui)
set_target_properties(ImGuizmo PROPERTIES FOLDER External CXX_STANDARD 20)

file (GLOB_RECURSE ImGuiNodeEditor_Head ${CMAKE_CURRENT_SOURCE_DIR}/ImGuiNodeEditor/*.h)
file (GLOB_RECURSE ImGuiNodeEditor_Inl ${CMAKE_CURRENT_SOURCE_DIR}/ImGuiNodeEditor/*.inl)
file (GLOB_RECURSE ImGuiNodeEditor_Src ${CMAKE_CURRENT_SOURCE_DIR}/ImGuiNodeEditor/*.cpp)
add_library(ImGuiNodeEditor STATIC)
target_sources(ImGuiNodeEditor PRIVATE ${ImGuiNodeEditor_Head} ${ImGuiNodeEditor_Inl} ${ImGuiNodeEditor_Src})
target_include_directories(ImGuiNodeEditor PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/ImGuiNodeEditor 
    ${CMAKE_CURRENT_SOURCE_DIR}/ImGui)
set_target_properties(ImGuiNodeEditor PROPERTIES FOLDER External CXX_STANDARD 20)

option(VULTANA_FETCH_ISPC "Fetch latest ISPC release into Binary/ when Binary/ispc.exe is missing" ON)

if(NOT ISPC_EXECUTABLE AND EXISTS "${PROJECT_SOURCE_DIR}/Binary/ispc.exe")
    set(ISPC_EXECUTABLE "${PROJECT_SOURCE_DIR}/Binary/ispc.exe" CACHE FILEPATH "ISPC compiler executable")
endif()
if(NOT ISPC_EXECUTABLE)
    find_program(ISPC_EXECUTABLE ispc)   # PATH or -DISPC_EXECUTABLE=... override
endif()

if(NOT ISPC_EXECUTABLE AND VULTANA_FETCH_ISPC AND NOT ISPC_BOOTSTRAP_DONE)
    file(DOWNLOAD "https://api.github.com/repos/ispc/ispc/releases/latest"
         "${CMAKE_BINARY_DIR}/ispc_latest.json" TIMEOUT 60 STATUS _ispcApiStatus)
    list(GET _ispcApiStatus 0 _ispcApiCode)
    if(_ispcApiCode EQUAL 0)
        file(READ "${CMAKE_BINARY_DIR}/ispc_latest.json" _ispcJson)
        string(JSON _ispcTag ERROR_VARIABLE _ispcJsonErr GET "${_ispcJson}" tag_name)   # e.g. "v1.31.0"
        set(_ispcZip "${CMAKE_BINARY_DIR}/ispc-${_ispcTag}-windows.zip")
        file(DOWNLOAD "https://github.com/ispc/ispc/releases/download/${_ispcTag}/ispc-${_ispcTag}-windows.zip"
             "${_ispcZip}" TIMEOUT 600 STATUS _ispcDlStatus)
        list(GET _ispcDlStatus 0 _ispcDlCode)
        if(_ispcDlCode EQUAL 0)
            file(ARCHIVE_EXTRACT INPUT "${_ispcZip}" DESTINATION "${CMAKE_BINARY_DIR}/ispc_dl")
            file(GLOB_RECURSE _ispcExe "${CMAKE_BINARY_DIR}/ispc_dl/*/bin/ispc.exe")
            if(_ispcExe)
                list(GET _ispcExe 0 _ispcExe)
                file(RENAME "${_ispcExe}" "${PROJECT_SOURCE_DIR}/Binary/ispc.exe")
                set(ISPC_EXECUTABLE "${PROJECT_SOURCE_DIR}/Binary/ispc.exe" CACHE FILEPATH "ISPC compiler executable")
            endif()
            file(REMOVE_RECURSE "${CMAKE_BINARY_DIR}/ispc_dl")
            file(REMOVE "${_ispcZip}")
        endif()
    endif()
    set(ISPC_BOOTSTRAP_DONE ON CACHE INTERNAL "ispc bootstrap attempted this configure")
endif()

if(NOT ISPC_EXECUTABLE)
    message(WARNING "ispc not found — kernel regeneration unavailable (prebuilt objs still work, Slice5 §0.1)")
endif()

# ispc_texcomp static library — sources live under ISPCTextureCompressor/ispc_texcomp/.
# Guarded so a missing prebuilt/*.obj degrades to "no ispc_texcomp target" (Framework's
# `if(TARGET ispc_texcomp)` then falls back to RGBA8) instead of a hard configure failure.
set(_ispcRoot ${CMAKE_CURRENT_SOURCE_DIR}/ISPCTextureCompressor/ispc_texcomp)
set(_ispcKernelObjs
    ${_ispcRoot}/prebuilt/kernel.obj
    ${_ispcRoot}/prebuilt/kernel_astc.obj)
set(_ispcKernelsPresent TRUE)
foreach(_o IN LISTS _ispcKernelObjs)
    if(NOT EXISTS "${_o}")
        set(_ispcKernelsPresent FALSE)
    endif()
endforeach()

if(_ispcKernelsPresent)
    add_library(ispc_texcomp STATIC
        ISPCTextureCompressor/ispc_texcomp/ispc_texcomp.cpp
        ISPCTextureCompressor/ispc_texcomp/ispc_texcomp_astc.cpp
        ${_ispcRoot}/prebuilt/kernel.obj
        ${_ispcRoot}/prebuilt/kernel_astc.obj
    )
    target_include_directories(ispc_texcomp
        PUBLIC  ${_ispcRoot}
        PRIVATE ${_ispcRoot}/prebuilt)  # kernel_ispc.h / kernel_astc_ispc.h
    set_target_properties(ispc_texcomp PROPERTIES FOLDER External CXX_STANDARD 20)
else()
    message(WARNING "ispc_texcomp prebuilt kernels missing (<repo>/External/ISPCTextureCompressor/ispc_texcomp/prebuilt/*.obj) -> BCn cooking disabled (RGBA8 fallback). Regenerate via the ispc_texcomp_regenerate target (Slice5 §3.13) then reconfigure.")
endif()

if(ISPC_EXECUTABLE)
    add_custom_target(ispc_texcomp_regenerate
        COMMAND ${ISPC_EXECUTABLE} -O2 kernel.ispc      -o prebuilt/kernel.obj      -h prebuilt/kernel_ispc.h      --target=avx2-i32x8 --opt=fast-math
        COMMAND ${ISPC_EXECUTABLE} -O2 kernel_astc.ispc -o prebuilt/kernel_astc.obj -h prebuilt/kernel_astc_ispc.h --target=avx2-i32x8 --opt=fast-math
        WORKING_DIRECTORY ${_ispcRoot}
        COMMENT "Regenerating ISPC kernel prebuilt objects (avx2-i32x8)")
endif()