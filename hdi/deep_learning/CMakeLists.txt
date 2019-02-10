set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

include_directories ("${CMAKE_SOURCE_DIR}")
include_directories ("${CMAKE_CURRENT_BINARY_DIR}/../../hdi/visualization")

#set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(APPLE)
else(APPLE)
    find_package(OpenMP REQUIRED)
    if (OPENMP_FOUND)
	message (STATUS "OpenMP found")
	set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    endif()
endif(APPLE)

find_package(Qt5Widgets REQUIRED)
find_package(Qt5OpenGL REQUIRED)

include_directories("$ENV{FLANN_DIR}/include")
message (STATUS "$ENV{FLANN_DIR}/include")

# Gather list of all .h files in "/"
file(GLOB HeaderFiles *.h)
file(GLOB SourceFiles *.cpp)
qt5_wrap_cpp(MocSources ${HeaderFiles} ${SourceFiles})

add_library   (hdideeplearning ${HeaderFiles} ${SourceFiles} ${MocSources} )

target_link_libraries (hdideeplearning Qt5::Widgets)
target_link_libraries (hdideeplearning Qt5::OpenGL)


########### INSTALL ##############
install (TARGETS hdideeplearning DESTINATION bin/hdi)
install (FILES ${HeaderFiles} ${UiHeaders} DESTINATION include/hdi/deep_learning)
