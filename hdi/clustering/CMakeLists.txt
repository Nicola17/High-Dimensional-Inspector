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

include_directories("$ENV{FLANN_DIR}/include")
message (STATUS "$ENV{FLANN_DIR}/include")

# Gather list of all .h files in "/"
file(GLOB HeaderFiles *.h)
file(GLOB SourceFiles *.cpp)

add_library   (hdiclustering ${HeaderFiles} ${SourceFiles} )


########### INSTALL ##############
install (TARGETS hdiclustering DESTINATION bin/hdi)
install (FILES ${HeaderFiles} ${UiHeaders} DESTINATION include/hdi/clustering)
