set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(APPLE)
else(APPLE)
  find_package(OpenMP REQUIRED)
  if (OPENMP_FOUND)
    message (STATUS "OpenMP found")
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
  endif()
endif(APPLE)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5OpenGL REQUIRED)
IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  find_package(OpenGL REQUIRED)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

include_directories ("catch")
include_directories ("${CMAKE_SOURCE_DIR}")
include_directories ("${CMAKE_CURRENT_BINARY_DIR}/../../hdi/visualization")

file(GLOB SourceFiles *.cpp)

file(GLOB HeaderFiles ${CMAKE_SOURCE_DIR}/visualization/*qobj.h)
qt5_wrap_cpp(MocSources ${HeaderFiles})

add_executable (tdd ${SourceFiles} ${MocSources})


target_link_libraries (tdd LINK_PUBLIC hdidimensionalityreduction)
target_link_libraries (tdd LINK_PUBLIC hdidata)
target_link_libraries (tdd LINK_PUBLIC hdiutils)
target_link_libraries (tdd LINK_PUBLIC hdivisualization)
target_link_libraries (tdd Qt5::Widgets)
target_link_libraries (tdd Qt5::OpenGL)
IF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  target_link_libraries (tdd Opengl32.lib)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  target_link_libraries(tdd ${OPENGL_LIBRARIES})
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

set_target_properties(tdd PROPERTIES FOLDER "Applications/Test Driven Development")
