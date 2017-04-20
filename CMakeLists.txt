cmake_minimum_required(VERSION 2.8)
project( picamdemo )
SET(COMPILE_DEFINITIONS -Werror)
# Locate libraries and headers
find_package(WiringPi REQUIRED)
find_package(Threads REQUIRED) 

# Include headers
include_directories(${WIRINGPI_INCLUDE_DIRS}) 


find_package( OpenCV REQUIRED )
include_directories(/opt/vc/include)
include_directories(/opt/vc/userland-master/interface/vcos)
include_directories(/opt/vc/userland-master)
include_directories(/opt/vc/userland-master/interface/vcos/pthreads)
include_directories(/opt/vc/userland-master/interface/vmcs_host/linux)
include_directories(/opt/vc/userland-master/host_applications/linux/libs/bcm_host/include)
link_directories(/opt/vc/lib)
add_executable(picamdemo picam.cpp camera.cpp cameracontrol.cpp graphics.cpp)

target_link_libraries(picamdemo libmmal_core.so libmmal_util.so libmmal_vc_client.so libvcos.so libbcm_host.so GLESv2 EGL ${OpenCV_LIBS})
# Link against libraries
target_link_libraries(picamdemo ${WIRINGPI_LIBRARIES}) 
target_link_libraries(picamdemo ${CMAKE_THREAD_LIBS_INIT}) 
