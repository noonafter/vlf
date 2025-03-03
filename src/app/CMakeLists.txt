
set (APP_SRC

    main.cpp
    mainwindow.cpp
    vlfabstractreceiver.cpp
    VLFUdpReceiver.cpp
    vlfchannel.cpp
    vlfchannel.h
    VLFReceiverConfig.cpp
    VLFReceiverConfig.h
    freqplotter.cpp
    freqplotter.h
    qcustomplot.h
    qcustomplot.cpp
        )

set(APP_HDRS

    mainwindow.h
    vlfabstractreceiver.h
    VLFUdpReceiver.h
    BufferedWriter.h
    fdacoefs.h
        )

set(APP_UIS
    mainwindow.ui
    )

QT5_WRAP_UI(APP_UI_HDRS ${APP_UI})

set(APP_NAME vlf_rx)
add_executable(${APP_NAME} ${APP_SRC} ${APP_HDRS} ${APP_UI_HDRS})

target_include_directories(${APP_NAME} PUBLIC
        ${CMAKE_SOURCE_DIR}/src/readerwriterqueue
        )

target_link_directories(${APP_NAME} PUBLIC
        ${FFTW3_PATH}
        )

target_link_libraries (${APP_NAME}
        ${QT_VERSION_BASE}::Core
        ${QT_VERSION_BASE}::Gui
        ${QT_VERSION_BASE}::Widgets
        ${QT_VERSION_BASE}::Sql
        ${QT_VERSION_BASE}::PrintSupport
        ${QT_VERSION_BASE}::Network
        liquid
        libfftw3f-3

        )

# 定义源文件路径和目标路径
set(DLL_SRC "${CMAKE_SOURCE_DIR}/src/fftw-3.3.5-dll64/libfftw3f-3.dll")
set(DEST_DIR "$<TARGET_FILE_DIR:${APP_NAME}>")

# 添加自定义命令实现自动拷贝
add_custom_command(TARGET ${APP_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy "${DLL_SRC}" "${DEST_DIR}"
        COMMENT "Copying DLL to output directory"
        VERBATIM
        )