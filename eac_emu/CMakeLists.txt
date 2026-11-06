cmake_minimum_required(VERSION 3.20)
project(eac_emu CXX)

add_library(eac_dll SHARED
    src/log.cpp src/dll/dllmain.cpp src/dll/eos_api.cpp src/dll/legacy_api.cpp)
target_include_directories(eac_dll PRIVATE include)
set_target_properties(eac_dll PROPERTIES OUTPUT_NAME "EasyAntiCheat_EOS"
    PREFIX "" SUFFIX ".dll")
target_link_options(eac_dll PRIVATE "/DEF:${CMAKE_SOURCE_DIR}/src/dll/eac_emu.def")

add_executable(eac_ctl src/log.cpp src/ctl/main.cpp src/ctl/driver_ctl.cpp)
target_include_directories(eac_ctl PRIVATE include)
target_link_libraries(eac_ctl PRIVATE advapi32)
set_target_properties(eac_ctl PROPERTIES OUTPUT_NAME "eac_emu_ctl")

# surucu icin build.bat kullan (WDK yollari gerekli)