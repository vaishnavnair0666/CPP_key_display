cmake_minimum_required(VERSION 3.10)
project(key-display)

find_package(PkgConfig REQUIRED)

pkg_check_modules(LIBINPUT REQUIRED libinput)
pkg_check_modules(XKB REQUIRED xkbcommon)
pkg_check_modules(UDEV REQUIRED libudev)

add_executable(key-display src/main.cpp)

target_include_directories(key-display PRIVATE
  ${LIBINPUT_INCLUDE_DIRS}
  ${XKB_INCLUDE_DIRS}
  ${UDEV_INCLUDE_DIRS}
)

target_link_libraries(key-display
  ${LIBINPUT_LIBRARIES}
  ${XKB_LIBRARIES}
  ${UDEV_LIBRARIES}
)
