cmake_minimum_required(VERSION 3.16)
set(EXTRA_COMPONENT_DIRS "$ENV{ESP_IDF_LIB_PATH}/components")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(embeded_system_control_stack)