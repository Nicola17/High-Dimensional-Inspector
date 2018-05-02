MACRO(ADD_PROJECT Name Sources MocSources)
  add_executable (${Name} ${Sources} ${MocSources})

  target_link_libraries (${Name} hdianalytics)
  target_link_libraries (${Name} hdidimensionalityreduction)
  target_link_libraries (${Name} hdiclustering)
  target_link_libraries (${Name} hdidata)
  target_link_libraries (${Name} hdivisualization)
  target_link_libraries (${Name} hdiutils)
  target_link_libraries (${Name} Qt5::Widgets)
  target_link_libraries (${Name} Qt5::OpenGL)
  target_link_libraries (${Name} Qt5::WebKit)
  target_link_libraries (${Name} Qt5::WebKitWidgets)
  qt5_use_modules(${Name} WebKitWidgets)
  IF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
      target_link_libraries (${Name} Opengl32.lib)
    target_link_libraries (${Name} "$ENV{FLANN_DIR}/lib/flann_cpp_s.lib")
  ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
      target_link_libraries(${Name} ${OPENGL_LIBRARIES})
    target_link_libraries(${Name} "libflann_cpp.so")
  ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

  MESSAGE( STATUS "Command line project: " ${Name} )

  set_target_properties(${Name} PROPERTIES FOLDER "Applications/Command line tools")
ENDMACRO()

MACRO(ADD_COMMANDLINE_PROJECT Name Sources)
  add_executable (${Name} ${Sources})
  target_link_libraries (${Name} hdidimensionalityreduction)
  target_link_libraries (${Name} hdiclustering)
  target_link_libraries (${Name} hdidata)
  target_link_libraries (${Name} hdiutils)
  IF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    target_link_libraries (${Name} "$ENV{FLANN_DIR}/lib/flann_cpp_s.lib")
  ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    target_link_libraries(${Name} "libflann_cpp.so")
  ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

  MESSAGE( STATUS "Command line project: " ${Name} )

  set_target_properties(${Name} PROPERTIES FOLDER "Applications/Command line tools")
ENDMACRO()

########################################################################################################

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

include_directories ("${CMAKE_SOURCE_DIR}")
include_directories ("${CMAKE_CURRENT_BINARY_DIR}/../../hdi/visualization")

set(CMAKE_INCLUDE_CURRENT_DIR ON)
find_package(Qt5 REQUIRED
  Widgets
  OpenGL
  WebKit
)
IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    find_package(OpenGL REQUIRED)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

if(APPLE)
else(APPLE)
  find_package(OpenMP REQUIRED)
  if (OPENMP_FOUND)
  message (STATUS "OpenMP found")
  set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
  endif()
endif(APPLE)
set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")

file(GLOB VisualizationQObjHeaderFiles ${CMAKE_SOURCE_DIR}/hdi/visualization/*qobj.h)
file(GLOB AnalyticsQObjHeaderFiles ${CMAKE_SOURCE_DIR}/hdi/analytics/*qobj.h)
qt5_wrap_cpp(MocSources ${AnalyticsQObjHeaderFiles} ${VisualizationQObjHeaderFiles})
qt5_add_resources(Resources "../shared_resources/shared_resources.qrc")

ADD_PROJECT(atsne_pva_cmd atsne_pva_cmd.cpp "${Resources}")
ADD_PROJECT(hsne_cmd hsne_cmd.cpp "${Resources}")
ADD_PROJECT(simple_embedding_viewer simple_embedding_viewer.cpp "${Resources}")
ADD_PROJECT(pex_viewer pex_viewer.cpp  "${Resources}")
ADD_COMMANDLINE_PROJECT(tsne_cmd tsne_cmd.cpp)
ADD_COMMANDLINE_PROJECT(atsne_cmd atsne_cmd.cpp)
ADD_COMMANDLINE_PROJECT(tsne_distance_matrix_cmd tsne_distance_matrix_cmd.cpp)
