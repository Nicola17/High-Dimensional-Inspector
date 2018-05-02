set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

include_directories ("${CMAKE_SOURCE_DIR}")
include_directories ("${CMAKE_CURRENT_BINARY_DIR}")
MESSAGE( STATUS "${CMAKE_CURRENT_BINARY_DIR}")

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
find_package(Qt5WebKit REQUIRED)
find_package(Qt5WebKitWidgets REQUIRED)

# Gather list of all .h files in "/"
file(GLOB HeaderFiles *.h)
file(GLOB QOBJHeaderFiles *_qobj.h)
file(GLOB SourceFiles *.cpp)
file(GLOB FormFiles *.ui)
file(GLOB ResourceFiles *.qrc)
file(GLOB_RECURSE WebFiles "web/*")

qt5_wrap_ui(UiHeaders ${FormFiles})
qt5_wrap_cpp(MocSources ${QOBJHeaderFiles})
qt5_add_resources(Resources ${ResourceFiles})

add_library(hdivisualization STATIC ${HeaderFiles} ${SourceFiles} ${UiHeaders} ${MocSources} ${Resources} ${WebFiles})

SET_TARGET_PROPERTIES(hdivisualization PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries (hdivisualization Qt5::Widgets)
target_link_libraries (hdivisualization Qt5::OpenGL)
target_link_libraries (hdivisualization Qt5::WebKit)
target_link_libraries (hdivisualization Qt5::WebKitWidgets)
target_link_libraries (hdivisualization hdiutils)


########### INSTALL ##############
install (TARGETS hdivisualization DESTINATION bin/hdi)
install (FILES ${HeaderFiles} ${UiHeaders} DESTINATION include/hdi/visualization)
