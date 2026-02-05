# ReSpeaker XVF3800 VAD Project
# ESP-IDF CMake configuration

cmake_minimum_required(VERSION 3.16)

# Include ESP-SR component from component registry
set(EXTRA_COMPONENT_DIRS $ENV{IDF_PATH}/components)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(respeaker_xvf3800_vad)
