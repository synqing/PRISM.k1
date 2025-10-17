cmake_minimum_required(VERSION 3.16)

# Point this test app to the main project's components directory
set(EXTRA_COMPONENT_DIRS ../components)

# Match the main project's target
set(IDF_TARGET esp32s3)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(test_all_app)

