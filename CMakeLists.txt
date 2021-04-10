cmake_minimum_required(VERSION 3.14)

project(test)

aux_source_directory(. source)
add_executable(${PROJECT_NAME} ${source})

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)