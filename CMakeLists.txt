cmake_minimum_required(VERSION 3.16)
project(OmniCore LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    set(DEST_LIB_DIR ${CMAKE_CURRENT_BINARY_DIR}/Win64)
elseif(APPLE)
    set(DEST_LIB_DIR ${CMAKE_CURRENT_BINARY_DIR}/macOS)
else()
    set(DEST_LIB_DIR ${CMAKE_CURRENT_BINARY_DIR}/Linux)
endif()

add_library(OmniCore SHARED
    Database/DataTable.cpp
    Database/Database.cpp
    User/User.cpp
    User/Repositories/User.cpp
    Session/Session.cpp
    Session/Repositories/Session.cpp
    Item/Item.cpp
    Item/Repositories/Item.cpp
    ItemGroup/ItemGroup.cpp
    ItemGroup/Repositories/ItemGroup.cpp
    ItemBrand/ItemBrand.cpp
    ItemBrand/Repositories/ItemBrand.cpp
    GlobalConfiguration/GlobalConfiguration.cpp
    GlobalConfiguration/Repositories/GlobalConfiguration.cpp
)

target_include_directories(OmniCore
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Base
        ${CMAKE_CURRENT_SOURCE_DIR}/Database
        ${CMAKE_CURRENT_SOURCE_DIR}/User
        ${CMAKE_CURRENT_SOURCE_DIR}/Session
        ${CMAKE_CURRENT_SOURCE_DIR}/Item
        ${CMAKE_CURRENT_SOURCE_DIR}/ItemGroup
        ${CMAKE_CURRENT_SOURCE_DIR}/ItemBrand
        ${CMAKE_CURRENT_SOURCE_DIR}/GlobalConfiguration
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/OmniUtils
)

find_library(SODIUM_LIB sodium REQUIRED)
find_package(Boost REQUIRED COMPONENTS json)

if(WIN32)
    target_link_libraries(OmniCore PUBLIC
        ${SODIUM_LIB}
        Boost::json
        ${CMAKE_CURRENT_SOURCE_DIR}/libOmniUtils.dll.a
    )
    add_custom_command(TARGET OmniCore POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/libOmniUtils.dll"
        "${DEST_LIB_DIR}/libOmniUtils.dll"
    )
elseif(APPLE)
    target_link_libraries(OmniCore PUBLIC
        ${SODIUM_LIB}
        Boost::json
        ${CMAKE_CURRENT_SOURCE_DIR}/libOmniUtils.dylib
    )
else()
    target_link_libraries(OmniCore PUBLIC
        ${SODIUM_LIB}
        Boost::json
        ${CMAKE_CURRENT_SOURCE_DIR}/libOmniUtils.so
    )
endif()

if(WIN32)
    target_link_libraries(OmniCore PUBLIC odbc32)
elseif(APPLE)
    find_library(ODBC_LIB odbc REQUIRED)
    target_link_libraries(OmniCore PUBLIC ${ODBC_LIB})
else()
    find_package(ODBC REQUIRED)
    target_link_libraries(OmniCore PUBLIC ODBC::ODBC)
endif()

set_target_properties(OmniCore PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${DEST_LIB_DIR}
    LIBRARY_OUTPUT_DIRECTORY ${DEST_LIB_DIR}
    ARCHIVE_OUTPUT_DIRECTORY ${DEST_LIB_DIR}
)

set(PUBLIC_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/Base/BaseModel.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Database/Database.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/User/User.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Session/Session.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Item/Item.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ItemGroup/ItemGroup.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ItemBrand/ItemBrand.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/GlobalConfiguration/GlobalConfiguration.hpp
)

foreach(HDR ${PUBLIC_HEADERS})
    file(RELATIVE_PATH REL_PATH "${CMAKE_CURRENT_SOURCE_DIR}" "${HDR}")
    set(DEST_PATH "${DEST_LIB_DIR}/OmniCore/${REL_PATH}")

    get_filename_component(DEST_DIR "${DEST_PATH}" DIRECTORY)
    add_custom_command(TARGET OmniCore POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${DEST_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy "${HDR}" "${DEST_PATH}"
    )
endforeach()

set(
    HEADER_DIRS
    Base
    Database
    User/DTOs
    User/Models
    User/Enums
    Session/DTOs
    Session/Enums
    Session/Models
    Item/Models
    Item/DTOs
    ItemGroup/Models
    ItemGroup/DTOs
    ItemBrand/Models
    ItemBrand/DTOs
    GlobalConfiguration/Models
    GlobalConfiguration/DTOs
)

foreach(DIR ${HEADER_DIRS})
    file(GLOB_RECURSE HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/${DIR}/*.hpp")
    foreach(HDR ${HEADERS})
        file(RELATIVE_PATH REL_PATH "${CMAKE_CURRENT_SOURCE_DIR}" "${HDR}")
        set(DEST_PATH "${DEST_LIB_DIR}/OmniCore/${REL_PATH}")

        get_filename_component(DEST_DIR "${DEST_PATH}" DIRECTORY)
        add_custom_command(TARGET OmniCore POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${DEST_DIR}"
            COMMAND ${CMAKE_COMMAND} -E copy "${HDR}" "${DEST_PATH}"
        )
    endforeach()
endforeach()