cmake_minimum_required(VERSION 3.20)
project(landru VERSION 0.2.0 LANGUAGES C)

if(NOT DEFINED LANDRU_GAME)
    set(LANDRU_GAME "TIE" CACHE STRING "Landru game version (TIE or XW)")
endif()
if(NOT DEFINED LANDRU_MODERN)
    option(LANDRU_MODERN "Enable native rendering extensions" ON)
endif()
if(NOT LANDRU_GAME STREQUAL "XW" AND NOT LANDRU_GAME STREQUAL "TIE")
    message(FATAL_ERROR "LANDRU_GAME must be XW or TIE")
endif()

set(LANDRU_SOURCES
    actanim.c
    actback.c
    actcust.c
    actdelt.c
    actor.c
    actraw.c
    actrect.c
    bitmap.c
    btnchk.c
    btnpush.c
    btnsldr.c
    btnstr.c
    btntext.c
    canvas.c
    color.c
    cursor.c
    delta.c
    dialog.c
    dlg.c
    dlgjoy.c
    dirty.c
    error.c
    fade.c
    file.c
    filedir.c
    film.c
    flip.c
    font.c
    host.c
    inpattr.c
    inpcall.c
    input.c
    io.c
    joy.c
    memcom.c
    memhdl.c
    memptr.c
    mouse.c
    paint.c
    pal.c
    paragrp.c
    rect.c
    remap.c
    render.c
    res.c
    scale.c
    sound.c
    stream.c
    style.c
    surface.c
    task.c
    timer.c
    vesa.c
    video.c
    view.c
    viewadd.c
)

set(LANDRU_HEADERS
    include/landru/actanim.h
    include/landru/actback.h
    include/landru/actcust.h
    include/landru/actdelt.h
    include/landru/actor.h
    include/landru/actraw.h
    include/landru/actrect.h
    include/landru/bitmap.h
    include/landru/btnchk.h
    include/landru/btnpush.h
    include/landru/btnsldr.h
    include/landru/btnstr.h
    include/landru/btntext.h
    include/landru/canvas.h
    include/landru/config.h
    include/landru/color.h
    include/landru/cursor.h
    include/landru/delta.h
    include/landru/dialog.h
    include/landru/dlg.h
    include/landru/dlgjoy.h
    include/landru/dirty.h
    include/landru/error.h
    include/landru/fade.h
    include/landru/file.h
    include/landru/filedir.h
    include/landru/film.h
    include/landru/flip.h
    include/landru/font.h
    include/landru/fourcc.h
    include/landru/host.h
    include/landru/inpattr.h
    include/landru/inpcall.h
    include/landru/input.h
    include/landru/io.h
    include/landru/joy.h
    include/landru/memcom.h
    include/landru/memhdl.h
    include/landru/memptr.h
    include/landru/mouse.h
    include/landru/paint.h
    include/landru/pal.h
    include/landru/paragrp.h
    include/landru/rect.h
    include/landru/remap.h
    include/landru/render.h
    include/landru/res.h
    include/landru/scale.h
    include/landru/sound.h
    include/landru/stream.h
    include/landru/style.h
    include/landru/surface.h
    include/landru/task.h
    include/landru/timer.h
    include/landru/vesa.h
    include/landru/video.h
    include/landru/view.h
    include/landru/viewadd.h
    binio.h
    canvas_internal.h
    dialog_internal.h
    host_internal.h
    render_internal.h
    timer_internal.h
    vesa_internal.h
)

set(LANDRU_FORMAT_SOURCES ${LANDRU_SOURCES})
if(LANDRU_GAME STREQUAL "XW")
    list(REMOVE_ITEM LANDRU_SOURCES btnchk.c btnsldr.c btnstr.c btntext.c)
else()
    list(REMOVE_ITEM LANDRU_SOURCES actback.c actrect.c dlg.c dlgjoy.c)
endif()

add_library(landru STATIC ${LANDRU_SOURCES})
target_compile_definitions(landru PUBLIC LANDRU_GAME_${LANDRU_GAME})
if(LANDRU_MODERN)
    target_compile_definitions(landru PUBLIC LANDRU_MODERN)
endif()
add_library(landru::landru ALIAS landru)
target_compile_features(landru PUBLIC c_std_99)

target_include_directories(landru
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Replace inherited directory-wide include paths so the shared engine cannot
# accidentally depend on headers from a consuming game.
set_property(TARGET landru PROPERTY INCLUDE_DIRECTORIES
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

set_target_properties(landru PROPERTIES
    VERSION 0.2.0
    SOVERSION 2
    POSITION_INDEPENDENT_CODE ON
)

find_program(LANDRU_CLANG_FORMAT NAMES clang-format)
if(LANDRU_CLANG_FORMAT)
    add_custom_target(landru_format
        COMMAND ${LANDRU_CLANG_FORMAT} -i
            ${LANDRU_FORMAT_SOURCES} ${LANDRU_HEADERS}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        VERBATIM)
else()
    add_custom_target(landru_format
        COMMAND "${CMAKE_COMMAND}" -E echo "clang-format was not found"
        COMMAND "${CMAKE_COMMAND}" -E false VERBATIM)
endif()
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    add_custom_target(format DEPENDS landru_format)
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
)

install(TARGETS landru
    EXPORT landruTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(EXPORT landruTargets
    FILE landruTargets.cmake
    NAMESPACE landru::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/landru
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/landruConfigVersion.cmake"
    VERSION 0.2.0
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/landruConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/landruConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/landru
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/landruConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/landruConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/landru
)
