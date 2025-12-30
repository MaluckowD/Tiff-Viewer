cmake_minimum_required(VERSION 3.10)
project(CalibrationModule)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Включение автоматических систем Qt
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets Xml)

# ВАЖНО: Устанавливаем статическую runtime для совместимости
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    set(CMAKE_CXX_FLAGS_DEBUG "/MTd /Zi /Od")
    set(CMAKE_CXX_FLAGS_RELEASE "/MT /O2")
endif()

# Список исходных файлов модуля
set(MODULE_SOURCES
    calibration_interface.cpp
    calibration_dialog.cpp
)

set(MODULE_HEADERS
    calibration_interface.h
    calibration_dialog.h
)

# Создаем DLL библиотеку
add_library(CalibrationModule SHARED ${MODULE_SOURCES} ${MODULE_HEADERS})

# Устанавливаем статическую runtime для DLL
if(MSVC)
    set_property(TARGET CalibrationModule PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    target_compile_options(CalibrationModule PRIVATE 
        $<$<CONFIG:Debug>:/MTd>
        $<$<CONFIG:Release>:/MT>
    )
endif()

# Экспортируем символы для Windows DLL
target_compile_definitions(CalibrationModule PRIVATE CALIBRATION_MODULE_EXPORT)

# Линкуем Qt библиотеки
target_link_libraries(CalibrationModule
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Xml
    Qt5::QWindowsIntegrationPlugin
)

# Установка DLL в директорию с исполняемым файлом
install(TARGETS CalibrationModule
    RUNTIME DESTINATION ${CMAKE_BINARY_DIR}
    LIBRARY DESTINATION ${CMAKE_BINARY_DIR}
)

# Копируем DLL в директорию с исполняемым файлом при сборке
add_custom_command(TARGET CalibrationModule POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:CalibrationModule> ${CMAKE_BINARY_DIR}/$<CONFIG>/
    COMMENT "Копирование CalibrationModule.dll в директорию с исполняемым файлом"
)
