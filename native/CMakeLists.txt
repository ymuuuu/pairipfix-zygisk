cmake_minimum_required(VERSION 3.22.1)
project(pairipfix_zygisk)

option(PAIRIP_DEBUG "Enable PairIPFix logging" OFF)
if (PAIRIP_DEBUG)
    add_compile_definitions(PAIRIP_LOG=1)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

macro(strip_and_move NAME)
    add_custom_command(TARGET ${NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory ${DEBUG_SYMBOLS_PATH}
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${NAME}> ${DEBUG_SYMBOLS_PATH}/$<TARGET_FILE_NAME:${NAME}>
            COMMAND ${CMAKE_STRIP} --strip-all $<TARGET_FILE:${NAME}>
    )
endmacro()

set(CXX_FLAGS "${CXX_FLAGS} -fno-exceptions -fno-rtti -fvisibility=hidden -fvisibility-inlines-hidden")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX_FLAGS}")

set(DOBBY_GENERATE_SHARED OFF CACHE INTERNAL "" FORCE)
set(DOBBY_DEBUG OFF CACHE INTERNAL "" FORCE)
set(LSPLANT_BUILD_SHARED OFF CACHE INTERNAL "" FORCE)

add_subdirectory(external/dobby)
add_subdirectory(external/lsplant/lsplant/src/main/jni lsplant_build)

add_library(xdl STATIC external/xdl/xdl/src/main/cpp/xdl.c)
target_include_directories(xdl PUBLIC external/xdl/xdl/src/main/cpp/include)

add_library(${MODULE_NAME} SHARED
    jni/main.cpp
    jni/lsplant_init.cpp
    jni/java_hooks.cpp
    jni/gate.cpp
    jni/native_shim.cpp
    jni/scrub.cpp
)

target_include_directories(${MODULE_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/jni
    external/lsplant/lsplant/src/main/jni/include
    external/dobby/include
)

target_link_libraries(${MODULE_NAME} PRIVATE
    lsplant_static
    dobby
    xdl
    log
)

strip_and_move(${MODULE_NAME})
