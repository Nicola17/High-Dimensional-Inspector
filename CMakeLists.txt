cmake_minimum_required (VERSION 2.8.11)
project (HighDimInspector)

# Use "-fPIC" / "-fPIE" for all targets by default, including static libs
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# CMake doesn't add "-pie" by default for executables (CMake issue #14983)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pie")

option(HDI_BUILD_VISUALIZATION "Build the visualization library" ON)
option(HDI_USE_ROARING "Use roaring bitmaps" ON)

option(APP_TDD "Build TDD" ON)
option(APP_COMMAND_LINE "Build command line tools" ON)
option(APP_VISUAL_TESTS "Build visual tests" ON)

if (HDI_USE_ROARING)
    add_definitions(-DPREPROC_USE_ROARING)
endif(HDI_USE_ROARING)

if(HDI_BUILD_VISUALIZATION)
    add_subdirectory (hdi/visualization)
endif(HDI_BUILD_VISUALIZATION)
add_subdirectory (hdi/dimensionality_reduction)
add_subdirectory (hdi/clustering)
add_subdirectory (hdi/data)
add_subdirectory (hdi/utils)
add_subdirectory (hdi/analytics)

#########################################
########### APPLICATIONS ################
#########################################

if(APP_TDD)
    add_subdirectory (applications/tdd)
endif(APP_TDD)

if(APP_COMMAND_LINE)
    add_subdirectory (applications/command_line_tools)
    add_subdirectory (applications/command_line_tools/hsne_volume_analyzer)
    add_subdirectory (applications/command_line_tools/data_conversion)
    if (HDI_USE_ROARING)
        add_subdirectory (applications/command_line_tools/waow_visualization)
    endif(HDI_USE_ROARING)
endif(APP_COMMAND_LINE)

if(APP_VISUAL_TESTS)
    add_subdirectory (applications/visual_tests)
endif(APP_VISUAL_TESTS)
