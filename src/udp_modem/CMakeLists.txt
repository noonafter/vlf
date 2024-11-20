
set (APP_SRC
        main.cpp
        udp_modem_widget.cpp
        freqspinbox.cpp
        udp_modem_worker.cpp
        udp_wave_config.cpp udp_wave_config.h)

set(APP_HDRS
        udp_modem_widget.h
        freqspinbox.h
        udp_modem_worker.h
        udp_config.h)

set(APP_UIS
        udp_modem_widget.ui
        freqspinbox.ui
        )

QT5_WRAP_UI(APP_UI_HDRS ${APP_UI})

set(APP_NAME udp_mod)
add_executable(${APP_NAME} ${APP_SRC} ${APP_HDRS} ${APP_UI_HDRS})

#message( here ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(${APP_NAME} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries (${APP_NAME}
        ${QT_VERSION_BASE}::Core
        ${QT_VERSION_BASE}::Gui
        ${QT_VERSION_BASE}::Widgets
        ${QT_VERSION_BASE}::Sql
        ${QT_VERSION_BASE}::PrintSupport
        ${QT_VERSION_BASE}::Network


        )
