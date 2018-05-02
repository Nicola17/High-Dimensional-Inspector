set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

include_directories ("${CMAKE_SOURCE_DIR}")
include_directories ("${CMAKE_CURRENT_BINARY_DIR}/../visualization")

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

# Gather list of all .h files in "/"
file(GLOB HeaderFiles *.h)
file(GLOB SourceFiles *.cpp)
file(GLOB FormFiles *.ui)
file(GLOB ResourceFiles *.qrc)

qt5_wrap_ui(UiHeaders ${FormFiles})
qt5_wrap_cpp(MocSources ${HeaderFiles})
qt5_add_resources(Resources ${ResourceFiles})

add_library   (hdianalytics ${HeaderFiles} ${SourceFiles} ${UiHeaders} ${MocSources} ${Resources} )

target_link_libraries (hdianalytics Qt5::Widgets)
target_link_libraries (hdianalytics Qt5::OpenGL)

########### INSTALL ##############
install (TARGETS hdianalytics DESTINATION bin/hdi)
install (FILES ${HeaderFiles} ${UiHeaders} DESTINATION include/hdi/analytics)
