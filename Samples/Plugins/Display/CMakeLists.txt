cmake_minimum_required(VERSION 3.10)
set (CMAKE_CXX_STANDARD 17)
project(TestXRProvider)
set(CMAKE_BUILD_TYPE Release)

Set(LIBRARY_NAME "DisplayProviderSample")

# set a project version
set (PROJECT_VERSION_MAJOR 1.0.2)
string(TIMESTAMP PROJECT_BUILD_TIME "%Y%m%d%H")

# Call configure files on ver.h.in to set the version.
# Uses the standard ${VARIABLE} syntax in the file
configure_file(version.h.in ${PROJECT_BINARY_DIR}/../include/version.h)

# option(USE_DEBUG "Enter debug mode" OFF)
# if (USE_DEBUG)
#   add_definitions(-DDLL_EXPORTS)
# endif()

# add_library(${LIBRARY_NAME} SHARED)

# target_sources(${LIBRARY_NAME}
# 	PRIVATE
# 	"src/*.cc"
# )

file(GLOB SOURCES "./Source/*.cpp")
add_library(${LIBRARY_NAME} SHARED ${SOURCES})

#Bring the headers, such as Student.h into the project
target_include_directories(${LIBRARY_NAME}
    PUBLIC
    include
    include/api
    ../../../Headers
)
 
#However, the file(GLOB...) allows for wildcard additions:
# file(GLOB SOURCES "src/*.cc")
# add_library(${LIBRARY_NAME} SHARED ${SOURCES})
 
#Generate the shared library from the sources
#find_package(OpenGLES3)
# get_filename_component(PROJECT_SOURCE_PARENT_DIR ${CMAKE_SOURCE_DIR} PATH)
# MESSAGE(CMAKE_SOURCE_DIR dir: ${PROJECT_SOURCE_PARENT_DIR})
# MESSAGE(CMAKE_MODULE_PATH dir: ${CMAKE_MODULE_PATH})

get_filename_component(PROJECT_SOURCE_PARENT_DIR ${CMAKE_CURRENT_SOURCE_DIR} PATH)
if(${CMAKE_ANDROID_ARCH_ABI} STREQUAL "arm64-v8a")
  MESSAGE("Set 64 lib")
#   set(NR_API_LIB ${CMAKE_CURRENT_SOURCE_DIR}/libs/64/libnr_api.so)
  set(DESTINY_PATH ${PROJECT_SOURCE_PARENT_DIR}/build/android/arm64)
else()
  MESSAGE("Set 32 lib")
#   set(NR_API_LIB ${CMAKE_CURRENT_SOURCE_DIR}/libs/32/libnr_api.so)
  set(DESTINY_PATH ${PROJECT_SOURCE_PARENT_DIR}/build/android/arm32)
endif()

# ${NR_API_LIB}
target_link_libraries(${LIBRARY_NAME}
    PUBLIC
    GLESv3
    EGL
    dl
)
	
#Set the location for library installation -- i.e., /usr/lib in this case
# not really necessary in this example. Use "sudo make install" to apply


install(TARGETS ${LIBRARY_NAME} DESTINATION ${DESTINY_PATH})