cmake_minimum_required (VERSION 2.8.11)
project (HighDimInspector)

option(APP_TDD "Build TDD" OFF)
option(APP_COMMAND_LINE "Build command line tools" OFF)
option(APP_HSNE "Build HSNE applications" OFF)
option(APP_DEEP_EYES "Build DeepEyes" OFF)
option(APP_COLLABORATION "Build collaboration tests" OFF)
option(APP_VISUAL_TESTS "Build visual tests" OFF)
option(USE_CAFFE "Caffe" OFF)



if(USE_CAFFE)
    find_package(Caffe REQUIRED)
    include_directories(${Caffe_INCLUDE_DIRS})
    add_definitions(${Caffe_DEFINITIONS})    # ex. -DCPU_ONLY
    add_definitions("-DUSE_CAFFE")    # ex. -DCPU_ONLY
    add_subdirectory (hdi/deep_learning)
    if(APP_DEEP_EYES)
	add_subdirectory (applications/deep_eyes)
	add_subdirectory (applications/deep_eyes/deep_eyes_training)
    endif(APP_DEEP_EYES)
endif(USE_CAFFE)

add_subdirectory (hdi/dimensionality_reduction)
add_subdirectory (hdi/analytics)
add_subdirectory (hdi/clustering)
add_subdirectory (hdi/data)
add_subdirectory (hdi/utils)
add_subdirectory (hdi/visualization)



#########################################
########### APPLICATIONS ################
#########################################

if(APP_TDD)
    add_subdirectory (applications/tdd)
endif(APP_TDD)

if(APP_COMMAND_LINE)
    add_subdirectory (applications/command_line_tools)
    add_subdirectory (applications/command_line_tools/hsne_volume_analyzer)
endif(APP_COMMAND_LINE)

if(APP_VISUAL_TESTS)
    add_subdirectory (applications/visual_tests)
endif(APP_VISUAL_TESTS)

############ HierarchicalSNE ############
if(APP_HSNE)
    add_subdirectory (applications/hierarchical_sne/development)
    add_subdirectory (applications/hierarchical_sne/hyperspectral_images)
    add_subdirectory (applications/hierarchical_sne/deep_learning)
endif(APP_HSNE)

############ Collaborations #############
if(APP_COLLABORATIONS)
    add_subdirectory (applications/collaborations/renata/brain_tumor)
    add_subdirectory (applications/collaborations/lkeb/gene_analysis)
    add_subdirectory (applications/collaborations/sea_simulation)
endif(APP_COLLABORATIONS)
