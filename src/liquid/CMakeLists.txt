# CMake 最低版本
cmake_minimum_required(VERSION 3.10)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(GenConfig)
include(GetSources)

# 创建库
add_library(liquid
        src/libliquid.c
        # nco依赖
        ${nco_SOURCES}
        ${utility_SOURCES}
        ${modem_SOURCES}

        # agc
        ${agc_SOURCES}
        ${random_SOURCES}

        # multi_channel
        ${matrix_SOURCES}
        ${math_SOURCES}
        ${filter_SOURCES}
        ${buffer_SOURCES}
        ${dotprod_SOURCES}
        ${fft_SOURCES}
        ${multichannel_SOURCES}
        )

# 设置头文件路径
target_include_directories(liquid PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${FFTW3_PATH}
        )

target_link_directories(liquid PUBLIC
        ${FFTW3_PATH}
)

target_link_libraries(liquid
        libfftw3f-3
)