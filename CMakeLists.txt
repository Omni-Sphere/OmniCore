cmake_minimum_required(VERSION 3.16)
project(OmniCore LANGUAGES CXX)

# --- 0. Detección de la Raíz de Proyectos (Sin usar "../") ---
get_filename_component(PROYECTOS_ROOT "${CMAKE_CURRENT_SOURCE_DIR}" DIRECTORY)

# --- Configuración inteligente para compilación manual ---
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
        set(CMAKE_INSTALL_PREFIX "/usr" CACHE PATH "Directorio de instalación" FORCE)
    endif()
    set(CMAKE_INSTALL_INCLUDEDIR "include/OmniSphere" CACHE PATH "Directorio de cabeceras" FORCE)
    set(CMAKE_INSTALL_LIBDIR "lib/OmniSphere" CACHE PATH "Directorio de librerías" FORCE)
    set(OMNI_BASE_DIR "/usr")
else()
    if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
        set(CMAKE_INSTALL_PREFIX "${PROYECTOS_ROOT}/.omni-dev-sdk" CACHE PATH "Directorio de instalación" FORCE)
    endif()
    set(CMAKE_INSTALL_INCLUDEDIR "include/OmniSphere" CACHE PATH "Directorio de cabeceras" FORCE)
    set(CMAKE_INSTALL_LIBDIR "lib" CACHE PATH "Directorio de librerías" FORCE)
    set(OMNI_BASE_DIR "${PROYECTOS_ROOT}/.omni-dev-sdk")
endif()

include(GNUInstallDirs)

if(EXISTS "${OMNI_BASE_DIR}")
    list(APPEND CMAKE_PREFIX_PATH "${OMNI_BASE_DIR}")
    list(APPEND CMAKE_PREFIX_PATH "${OMNI_BASE_DIR}/lib/cmake/OmniUtils")
    list(APPEND CMAKE_PREFIX_PATH "${OMNI_BASE_DIR}/lib/cmake/OmniData")
    list(APPEND CMAKE_PREFIX_PATH "${OMNI_BASE_DIR}/lib/OmniSphere/cmake/OmniUtils")
    list(APPEND CMAKE_PREFIX_PATH "${OMNI_BASE_DIR}/lib/OmniSphere/cmake/OmniData")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

set(INCLUDE_DIR "${CMAKE_BINARY_DIR}/OmniCore/Include")
set(LIBS_DIR "${CMAKE_BINARY_DIR}/OmniCore")

if(WIN32)
    set(ARCHIVE_DIR "${CMAKE_BINARY_DIR}/OmniCore/Libs")
else()
    set(ARCHIVE_DIR "${LIBS_DIR}")
endif()

add_library(OmniCore SHARED
    User/User.cpp
    User/Repositories/User.cpp
    Session/Session.cpp
    Session/Repositories/Session.cpp
    GlobalConfiguration/GlobalConfiguration.cpp
    GlobalConfiguration/Repositories/GlobalConfiguration.cpp
    File/File.cpp
    File/Repositories/File.cpp
    Authorization/Authorization.cpp
    Authorization/Repositories/Authorization.cpp
)

# --- Resolución de dependencias Omni ---
if(NOT TARGET OmniUtils::OmniUtils)
    find_package(OmniUtils REQUIRED)
endif()
if(NOT TARGET OmniData::OmniData)
    find_package(OmniData REQUIRED)
endif()



# SOLUCIÓN: Solo exportamos la raíz limpia del proyecto e incluimos el SDK local para dependencias
target_include_directories(OmniCore
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Base
        "${OMNI_BASE_DIR}/include"
)

add_library(OmniCore::OmniCore ALIAS OmniCore)

if(ANDROID)
    target_link_libraries(OmniCore PUBLIC
        sodium
        Boost::json
        OmniUtils::OmniUtils
        OmniData::OmniData
    )
else()
    find_library(SODIUM_LIB sodium REQUIRED)
    find_package(Boost REQUIRED COMPONENTS json)
    target_link_libraries(OmniCore PUBLIC
        ${SODIUM_LIB}
        Boost::json
        OmniUtils::OmniUtils
        OmniData::OmniData
    )
    if(WIN32)
        target_link_libraries(OmniCore PUBLIC odbc32)
    elseif(APPLE)
        find_library(ODBC_LIB odbc REQUIRED)
        target_link_libraries(OmniCore PUBLIC ${ODBC_LIB})
    else()
        find_package(ODBC REQUIRED)
        target_link_libraries(OmniCore PUBLIC ODBC::ODBC)
    endif()
endif()

if(WIN32)
    set_target_properties(OmniCore PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${LIBS_DIR}"
        LIBRARY_OUTPUT_DIRECTORY "${LIBS_DIR}"
        ARCHIVE_OUTPUT_DIRECTORY "${ARCHIVE_DIR}"
    )
else()
    set_target_properties(OmniCore PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${LIBS_DIR}"
        ARCHIVE_OUTPUT_DIRECTORY "${LIBS_DIR}"
    )
endif()

set(PUBLIC_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/Base/BaseModel.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/User/User.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Session/Session.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/GlobalConfiguration/GlobalConfiguration.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/File/File.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Authorization/Authorization.hpp
)

foreach(HDR ${PUBLIC_HEADERS})
    file(RELATIVE_PATH REL_PATH "${CMAKE_CURRENT_SOURCE_DIR}" "${HDR}")
    set(DEST_PATH "${INCLUDE_DIR}/${REL_PATH}")
    get_filename_component(DEST_INC_DIR "${DEST_PATH}" DIRECTORY)
    add_custom_command(TARGET OmniCore POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${DEST_INC_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${HDR}" "${DEST_PATH}"
    )
endforeach()

set(
    HEADER_DIRS
    Base
    User/DTOs
    User/Models
    User/Enums
    Session/DTOs
    Session/Enums
    Session/Models
    GlobalConfiguration/Models
    GlobalConfiguration/DTOs
    File/DTOs
    File/Models
    Authorization
    Authorization/DTOs
    Authorization/Models
)

foreach(DIR ${HEADER_DIRS})
    file(GLOB_RECURSE HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/${DIR}/*.hpp")
    foreach(HDR ${HEADERS})
        file(RELATIVE_PATH REL_PATH "${CMAKE_CURRENT_SOURCE_DIR}" "${HDR}")
        set(DEST_PATH "${INCLUDE_DIR}/${REL_PATH}")
        get_filename_component(DEST_INC_DIR "${DEST_PATH}" DIRECTORY)
        add_custom_command(TARGET OmniCore POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${DEST_INC_DIR}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${HDR}" "${DEST_PATH}"
        )
    endforeach()
endforeach()

# --- Installation & Export ---
install(TARGETS OmniCore
    EXPORT OmniCoreTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME} # Sin subcarpetas extra que dupliquen rutas
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
    FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h"
    PATTERN "build" EXCLUDE
    PATTERN ".git" EXCLUDE
    PATTERN ".vscode" EXCLUDE
)

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/OmniCoreConfig.cmake"
"include(CMakeFindDependencyMacro)\n"
"find_dependency(Boost REQUIRED COMPONENTS json)\n"
"if(NOT WIN32 AND NOT ANDROID)\n"
"  find_dependency(ODBC REQUIRED)\n"
"endif()\n"
"find_dependency(OmniUtils REQUIRED)\n"
"find_dependency(OmniData REQUIRED)\n"
"include(\"\${CMAKE_CURRENT_LIST_DIR}/OmniCoreTargets.cmake\")\n"
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/OmniCoreConfig.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OmniCore
)

install(EXPORT OmniCoreTargets
    FILE OmniCoreTargets.cmake
    NAMESPACE OmniCore::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OmniCore
)