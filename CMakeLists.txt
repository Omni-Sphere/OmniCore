cmake_minimum_required(VERSION 3.16)
project(OmniCore LANGUAGES CXX)

include(GNUInstallDirs)

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
)

target_include_directories(OmniCore
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Base>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/User>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Session>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/GlobalConfiguration>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/File>
        $<INSTALL_INTERFACE:include>
        $<INSTALL_INTERFACE:include/Base>
        $<INSTALL_INTERFACE:include/User>
        $<INSTALL_INTERFACE:include/Session>
        $<INSTALL_INTERFACE:include/GlobalConfiguration>
        $<INSTALL_INTERFACE:include/File>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Base
)

add_library(OmniCore::OmniCore ALIAS OmniCore)

if(NOT TARGET OmniUtils::OmniUtils)
    find_package(OmniUtils REQUIRED)
endif()
if(NOT TARGET OmniData::OmniData)
    find_package(OmniData REQUIRED)
endif()

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
include(GNUInstallDirs)

install(TARGETS OmniCore
    EXPORT OmniCoreTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} ${CMAKE_INSTALL_INCLUDEDIR}/Base ${CMAKE_INSTALL_INCLUDEDIR}/User ${CMAKE_INSTALL_INCLUDEDIR}/Session ${CMAKE_INSTALL_INCLUDEDIR}/GlobalConfiguration ${CMAKE_INSTALL_INCLUDEDIR}/File
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
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
