set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

include_directories ("${CMAKE_SOURCE_DIR}")

if(APPLE)
else(APPLE)
  find_package(OpenMP REQUIRED)
  if (OPENMP_FOUND)
    message (STATUS "OpenMP found")
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
  endif()
endif(APPLE)

find_package(Qt5 REQUIRED
  Core
  Gui
)

include_directories("$ENV{FLANN_DIR}/include")
message (STATUS "$ENV{FLANN_DIR}/include")

# Gather list of all .h files in "/"
file(GLOB HeaderFiles *.h)
file(GLOB SourceFiles *.cpp)

add_library   (hdidimensionalityreduction ${HeaderFiles} ${SourceFiles} )

target_link_libraries (hdidimensionalityreduction Qt5::Core Qt5::Gui)


########### INSTALL ##############
install (TARGETS hdidimensionalityreduction DESTINATION bin/hdi)
install (FILES ${HeaderFiles} ${UiHeaders} DESTINATION include/hdi/dimensionality_reduction)
