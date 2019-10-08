cmake_minimum_required(VERSION 2.6)

######################################################################################
# Project declaration and options
######################################################################################

project(wii-sphero)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fPIC -fpermissive")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wextra -Woverloaded-virtual")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g -pthread")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
include_directories(/usr/local/include /usr/include/sphero .)

link_directories(/usr/local/lib)
link_directories(/usr/lib/arm-linux-gnueabihf) 

add_executable(wii-sphero wii-sphero.cpp wiimote.cpp spheromanager.cpp)

target_link_libraries(wii-sphero xwiimote sphero wiringPi boost_system boost_thread ncurses)

install(TARGETS wii-sphero
  PERMISSIONS SETUID OWNER_EXECUTE OWNER_READ OWNER_WRITE GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ
  DESTINATION /home/pi/bin)

install(FILES wii-sphero.desktop
  DESTINATION /home/pi/Desktop)

install(FILES sphero_mm.png sphero.cfg
  DESTINATION /home/pi)

install(FILES run-wii-sphero
  DESTINATION /home/pi/bin
  PERMISSIONS OWNER_EXECUTE OWNER_READ OWNER_WRITE GROUP_EXECUTE GROUP_READ )
