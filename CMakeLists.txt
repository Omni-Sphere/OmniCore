cmake_minimum_required(VERSION 3.16)
project(OmniCore LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)


set(LIBS_DIR "${CMAKE_BINARY_DIR}/OmniCore/Libs")
set(INCLUDE_DIR "${CMAKE_BINARY_DIR}/OmniCore/Include")

add_library(OmniCore SHARED
    User/User.cpp
    User/Repositories/User.cpp
    Session/Session.cpp
    Session/Repositories/Session.cpp
    GlobalConfiguration/GlobalConfiguration.cpp
    GlobalConfiguration/Repositories/GlobalConfiguration.cpp
)

target_include_directories(OmniCore
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Base
        ${CMAKE_CURRENT_SOURCE_DIR}/User
        ${CMAKE_CURRENT_SOURCE_DIR}/Session
        ${CMAKE_CURRENT_SOURCE_DIR}/GlobalConfiguration
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/OmniUtils/Include
        ${CMAKE_CURRENT_SOURCE_DIR}/OmniData/Include
)

find_library(SODIUM_LIB sodium REQUIRED)
find_package(Boost REQUIRED COMPONENTS json)

if(WIN32)
    target_link_libraries(OmniCore PUBLIC
        ${SODIUM_LIB}
        Boost::json
        ${CMAKE_CURRENT_SOURCE_DIR}/OmniUtils/libOmniUtils.dll.a
        ${CMAKE_CURRENT_SOURCE_DIR}/OmniData/libOmniData.dll.a
    )
    add_custom_command(TARGET OmniCore POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${LIBS_DIR}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${INCLUDE_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/libboost_json-mt.dll"
            "${LIBS_DIR}/libboost_json-mt.dll"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/libsodium-26.dll"
            "${LIBS_DIR}/libsodium-26.dll"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/OmniData/Libs/libOmniData.dll"
            "${LIBS_DIR}/libOmniData.dll"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/OmniUtils/Libs/libOmniUtils.dll"
            "${LIBS_DIR}/libOmniUtils.dll"
    )
elseif(APPLE)
    target_link_libraries(OmniCore PUBLIC
        ${SODIUM_LIB}
        Boost::json
        ${CMAKE_CURRENT_SOURCE_DIR}/OmniUtils/Libs/libOmniUtils.dylib
        ${CMAKE_CURRENT_SOURCE_DIR}/OmniData/Libs/libOmniData.dylib
    )
    add_custom_command(TARGET OmniCore POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${LIBS_DIR}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${INCLUDE_DIR}"
    )
else()
    target_link_libraries(OmniCore PUBLIC
        ${SODIUM_LIB}
        Boost::json
        ${CMAKE_CURRENT_SOURCE_DIR}/OmniUtils/Libs/libOmniUtils.so
        ${CMAKE_CURRENT_SOURCE_DIR}/OmniData/Libs/libOmniData.so
    )
    add_custom_command(TARGET OmniCore POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${LIBS_DIR}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${INCLUDE_DIR}"
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


if(WIN32)
    set_target_properties(OmniCore PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${LIBS_DIR}"
        LIBRARY_OUTPUT_DIRECTORY "${LIBS_DIR}"
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/OmniCore"
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
)

foreach(HDR ${PUBLIC_HEADERS})
    file(RELATIVE_PATH REL_PATH "${CMAKE_CURRENT_SOURCE_DIR}" "${HDR}")
    set(DEST_PATH "${INCLUDE_DIR}/${REL_PATH}")

    get_filename_component(DEST_INC_DIR "${DEST_PATH}" DIRECTORY)
    add_custom_command(TARGET OmniCore POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${DEST_INC_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy "${HDR}" "${DEST_PATH}"
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
)

foreach(DIR ${HEADER_DIRS})
    file(GLOB_RECURSE HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/${DIR}/*.hpp")
    foreach(HDR ${HEADERS})
        file(RELATIVE_PATH REL_PATH "${CMAKE_CURRENT_SOURCE_DIR}" "${HDR}")
        set(DEST_PATH "${INCLUDE_DIR}/${REL_PATH}")

        get_filename_component(DEST_INC_DIR "${DEST_PATH}" DIRECTORY)
        add_custom_command(TARGET OmniCore POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${DEST_INC_DIR}"
            COMMAND ${CMAKE_COMMAND} -E copy "${HDR}" "${DEST_PATH}"
        )
    endforeach()
endforeach()


if(UNIX AND NOT APPLE)
    install(TARGETS OmniCore
        LIBRARY DESTINATION lib/OmniSphere
        ARCHIVE DESTINATION lib/OmniSphere
        RUNTIME DESTINATION bin
    )
    
    foreach(HDR ${PUBLIC_HEADERS})
        file(RELATIVE_PATH REL_PATH "${CMAKE_CURRENT_SOURCE_DIR}" "${HDR}")
        get_filename_component(HDR_DIR "${REL_PATH}" DIRECTORY)
        
        if(HDR_DIR)
            install(FILES "${HDR}" DESTINATION "include/OmniSphere/OmniCore/${HDR_DIR}")
        else()
            install(FILES "${HDR}" DESTINATION "include/OmniSphere/OmniCore")
        endif()
    endforeach()
    
    foreach(DIR ${HEADER_DIRS})
        file(GLOB_RECURSE HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/${DIR}/*.hpp")
        foreach(HDR ${HEADERS})
            file(RELATIVE_PATH REL_PATH "${CMAKE_CURRENT_SOURCE_DIR}" "${HDR}")
            get_filename_component(HDR_DIR "${REL_PATH}" DIRECTORY)
            
            if(HDR_DIR)
                install(FILES "${HDR}" DESTINATION "include/OmniSphere/OmniCore/${HDR_DIR}")
            else()
                install(FILES "${HDR}" DESTINATION "include/OmniSphere/OmniCore")
            endif()
        endforeach()
    endforeach()
    
    install(CODE "
        file(WRITE \"/etc/ld.so.conf.d/omnisphere.conf\" \"/usr/local/lib/OmniSphere\\n\")
        execute_process(COMMAND ldconfig)
        message(STATUS \"Configured ldconfig for OmniSphere libraries\")
    ")
endif()

