project(LCD)
cmake_minimum_required(VERSION 2.8)
include_directories(. include)
set(DEST_DIR /opt/lcdsrv)
set(SRC_LIST
main.cpp
utils.cpp
configfile.cpp
logging.cpp
winstar_lcd.cpp
include/common.h
include/config.h
include/configfile.h
include/logging.h
include/stubs.h
include/winstar_lcd.h
)
aux_source_directory(. SRC_LIST)
add_executable(${PROJECT_NAME} ${SRC_LIST})
add_library(winstar_lcd SHARED winstar_lcd.cpp include/winstar_lcd.h)

target_link_libraries(${PROJECT_NAME} Ltps)

install(TARGETS ${PROJECT_NAME} DESTINATION ${DEST_DIR})
install(TARGETS winstar_lcd DESTINATION /usr/lib)
install(FILES lcdsrv.conf DESTINATION ${DEST_DIR})
