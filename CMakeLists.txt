cmake_minimum_required(VERSION 3.10)

# Принудительно устанавливаем x64 архитектуру ПЕРЕД объявлением проекта
if(WIN32)
    set(CMAKE_GENERATOR_PLATFORM x64)
    set(CMAKE_VS_PLATFORM_NAME x64)
endif()

project(HyperspectralViewer)

# Настройка стандарта C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ВАЖНО: Принудительно устанавливаем СТАТИЧЕСКУЮ runtime библиотеку для совместимости с Qt
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    # Устанавливаем флаги компилятора для статической runtime
    set(CMAKE_CXX_FLAGS_DEBUG "/MTd /Zi /Od")
    set(CMAKE_CXX_FLAGS_RELEASE "/MT /O2")
    set(CMAKE_C_FLAGS_DEBUG "/MTd /Zi /Od")
    set(CMAKE_C_FLAGS_RELEASE "/MT /O2")
endif()

# Включение автоматических систем Qt
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets)

# Принудительная загрузка LibTIFF через FetchContent
include(FetchContent)

# Устанавливаем политику для временных меток
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

# Настройки для LibTIFF перед загрузкой
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)
set(tiff-tools OFF CACHE BOOL "Build TIFF tools" FORCE)
set(tiff-tests OFF CACHE BOOL "Build TIFF tests" FORCE)
set(tiff-contrib OFF CACHE BOOL "Build TIFF contrib" FORCE)
set(tiff-docs OFF CACHE BOOL "Build TIFF documentation" FORCE)
set(tiff-deprecated OFF CACHE BOOL "Build deprecated TIFF features" FORCE)

FetchContent_Declare(
    libtiff
    URL https://download.osgeo.org/libtiff/tiff-4.3.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

FetchContent_MakeAvailable(libtiff)

# Принудительно устанавливаем статическую runtime для LibTIFF
if(TARGET tiff)
    if(MSVC)
        set_property(TARGET tiff PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
        target_compile_options(tiff PRIVATE 
            $<$<CONFIG:Debug>:/MTd>
            $<$<CONFIG:Release>:/MT>
        )
    endif()
endif()

# Устанавливаем переменные для LibTIFF
set(TIFF_INCLUDE_DIRS ${libtiff_SOURCE_DIR}/libtiff)
set(TIFF_LIBRARIES tiff)

# Список исходных файлов
set(SOURCES
    main.cpp
    main_window.cpp
    tiff_reader.cpp
    hyperspectral_image.cpp
    histogram_widget.cpp
    dialogs.cpp
    spectral_reader.cpp
    spectral_info_dialog.cpp
    image_label.cpp
    spectral_curve_dialog.cpp
)

set(HEADERS
    main_window.h
    tiff_reader.h
    hyperspectral_image.h
    histogram_widget.h
    spectral_reader.h
    spectral_info_dialog.h
    dialogs.h
    image_label.h
    spectral_curve_dialog.h
)

# Создание исполняемого файла
add_executable(HyperspectralViewer ${SOURCES} ${HEADERS})

# Устанавливаем статическую runtime для нашего исполняемого файла
if(MSVC)
    set_property(TARGET HyperspectralViewer PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    target_compile_options(HyperspectralViewer PRIVATE 
        $<$<CONFIG:Debug>:/MTd>
        $<$<CONFIG:Release>:/MT>
    )
endif()

# Включаем директории заголовочных файлов
target_include_directories(HyperspectralViewer PRIVATE 
    ${TIFF_INCLUDE_DIRS}
    ${libtiff_BINARY_DIR}/libtiff  # Для сгенерированных заголовков
)

# Линкуем библиотеки
target_link_libraries(HyperspectralViewer 
    Qt5::Core 
    Qt5::Gui 
    Qt5::Widgets
    Qt5::QWindowsIntegrationPlugin
    ${TIFF_LIBRARIES}
)

# Для Windows
if(WIN32)
    target_compile_definitions(HyperspectralViewer PRIVATE 
        _CRT_SECURE_NO_WARNINGS  # Отключаем предупреждения MSVC
    )
endif()
