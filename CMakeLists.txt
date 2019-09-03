cmake_minimum_required (VERSION 2.8.11)

project (HighDimInspector)

if(APPLE)
  set(CMAKE_OSX_DEPLOYMENT_TARGET "10.12" CACHE STRING "Minimum OS X deployment version")
  set_target_properties(${PROJECT} PROPERTIES XCODE_ATTRIBUTE_COMPILER_INDEX_STORE_ENABLE "NO")

  set (LLVM_ROOT_DIR "/usr/local/opt/llvm")
  find_package( LLVM )
  if( LLVM_FOUND )
    include_directories (${LLVM_LIBRARY_DIRS}/clang/${LLVM_VERSION_BASE_STRING}/include)

    set(CMAKE_CXX_COMPILER "/usr/local/opt/llvm/bin/clang++")
    set(CMAKE_C_COMPILER "/usr/local/opt/llvm/bin/clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fopenmp")
    set(CMAKE_XCODE_ATTRIBUTE_CC "/usr/local/opt/llvm/bin/clang")
    set(CMAKE_XCODE_ATTRIBUTE_CXX "/usr/local/opt/llvm/bin/clang++")

    find_library(IOMP5LIB
      NAMES "iomp5" "iomp5md" "libiomp5" "libiomp5md"
      HINTS ${LLVM_LIBRARY_DIRS})
    set (OMP_LIBRARIES ${OMP_LIBRARIES} ${IOMP5LIB})
  else(LLVM_FOUND)
    message("OS is macOS, no OpenMP support detected, using Grand Central Dispatch instead.")
    add_definitions( -D__USE_GCD__)
  endif( LLVM_FOUND )
endif(APPLE)

if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.12" CACHE STRING "Minimum OS X deployment version")
endif(APPLE)

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
