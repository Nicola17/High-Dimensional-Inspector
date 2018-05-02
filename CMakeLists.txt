cmake_minimum_required (VERSION 2.8.11)
project (HighDimInspector)

option(APP_TDD "Build TDD" ON)
option(APP_COMMAND_LINE "Build command line tools" ON)
option(APP_VISUAL_TESTS "Build visual tests" ON)

add_subdirectory (hdi/visualization)
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
    add_subdirectory (applications/command_line_tools/waow_visualization)
endif(APP_COMMAND_LINE)

if(APP_VISUAL_TESTS)
    add_subdirectory (applications/visual_tests)
endif(APP_VISUAL_TESTS)
