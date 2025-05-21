set (SHADER_ROOT ${PROJECT_SOURCE_DIR}/Shaders)

file (GLOB_RECURSE SHADER_FILES
    "${SHADER_ROOT}/*.hlsl"
    "${SHADER_ROOT}/*.hlsli"
    "${SHADER_ROOT}/*.h"
    "${SHADER_ROOT}/*.hpp"
)

set_source_files_properties(${SHADER_FILES} PROPERTIES HEADER_FILE_ONLY TRUE)

source_group(TREE ${SHADER_ROOT} PREFIX Shaders FILES ${SHADER_FILES})