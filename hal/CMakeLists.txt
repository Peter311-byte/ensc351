file(GLOB MY_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")

add_library(hal STATIC ${MY_SOURCES})

target_include_directories(hal PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)