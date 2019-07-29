set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories ("${CMAKE_SOURCE_DIR}")

if (CMAKE_GENERATOR STREQUAL Xcode)
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
else()
  find_package(OpenMP REQUIRED)
  if (OPENMP_FOUND)
    message (STATUS "OpenMP found")
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
  endif()
endif()

include_directories("$ENV{FLANN_DIR}/include")
message (STATUS "$ENV{FLANN_DIR}/include")

# Gather list of all .h files in "/"
file(GLOB HeaderFiles *.h gpgpu_sne/*.h)
file(GLOB SourceFiles *.cpp gpgpu_sne/*.cpp)

add_library   (hdidimensionalityreduction ${HeaderFiles} ${SourceFiles} )

########### INSTALL ##############
install (TARGETS hdidimensionalityreduction DESTINATION bin/hdi)
install (FILES ${HeaderFiles} ${UiHeaders} DESTINATION include/hdi/dimensionality_reduction)
