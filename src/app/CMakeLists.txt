
set (APP_SRC

    main.cpp
    mainwindow.cpp
    vlfabstractreceiver.cpp
    VLFUdpReceiver.cpp
    vlfchannel.cpp
    vlfchannel.h
    VLFReceiverConfig.cpp
    VLFReceiverConfig.h
        )

set(APP_HDRS

    mainwindow.h
    vlfabstractreceiver.h
    VLFUdpReceiver.h
    BufferedWriter.h
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

target_link_libraries (${APP_NAME}
        ${QT_VERSION_BASE}::Core
        ${QT_VERSION_BASE}::Gui
        ${QT_VERSION_BASE}::Widgets
        ${QT_VERSION_BASE}::Sql
        ${QT_VERSION_BASE}::PrintSupport
        ${QT_VERSION_BASE}::Network
        liquid

        )
