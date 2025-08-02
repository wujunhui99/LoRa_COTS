# Install script for directory: /Users/junhui/code/hw/code-lora/LoRa_COTS/src

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set path to fallback-tool for dependency-resolution.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/objdump")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/Users/junhui/code/hw/code-lora/LoRa_COTS/build/src/boards/NucleoL476/cmake_install.cmake")
  include("/Users/junhui/code/hw/code-lora/LoRa_COTS/build/src/boards/cmake_install.cmake")
  include("/Users/junhui/code/hw/code-lora/LoRa_COTS/build/src/radio/cmake_install.cmake")
  include("/Users/junhui/code/hw/code-lora/LoRa_COTS/build/src/system/cmake_install.cmake")
  include("/Users/junhui/code/hw/code-lora/LoRa_COTS/build/src/mac/cmake_install.cmake")
  include("/Users/junhui/code/hw/code-lora/LoRa_COTS/build/src/peripherals/cmake_install.cmake")
  include("/Users/junhui/code/hw/code-lora/LoRa_COTS/build/src/apps/l2b/cmake_install.cmake")

endif()

