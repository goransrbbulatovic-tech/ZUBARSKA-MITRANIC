cmake_minimum_required(VERSION 3.16)

# Naziv EXE-a
set(APP_NAME "Zubarska ordinacija Mitranic")

project(DentaPro VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# ── Qt detection (Qt6 first, fall back to Qt5) ───────────────────────────────
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Sql PrintSupport)
find_package(Qt6 OPTIONAL_COMPONENTS Pdf)

if(Qt6Pdf_FOUND)
    set(QT_LIBS Qt6::Core Qt6::Widgets Qt6::Sql Qt6::PrintSupport Qt6::Pdf)
    add_compile_definitions(HAVE_QT_PDF)
    message(STATUS "Using Qt6 with PDF support")
else()
    set(QT_LIBS Qt6::Core Qt6::Widgets Qt6::Sql Qt6::PrintSupport)
    message(STATUS "Using Qt6 without PDF support")
endif()

# ── Sources ──────────────────────────────────────────────────────────────────
set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/database.cpp
    src/patientdialog.cpp
    src/interventiondialog.cpp
    src/patienthistorywidget.cpp
    src/signaturewidget.cpp
    src/reportgenerator.cpp
    src/clinicsettings.cpp
    src/clinicsettingsdialog.cpp
    src/statisticswidget.cpp
    src/exportmanager.cpp
)

set(HEADERS
    src/mainwindow.h
    src/database.h
    src/models.h
    src/patientdialog.h
    src/interventiondialog.h
    src/patienthistorywidget.h
    src/signaturewidget.h
    src/reportgenerator.h
    src/clinicsettings.h
    src/clinicsettingsdialog.h
    src/statisticswidget.h
    src/exportmanager.h
)

# ── Resources ─────────────────────────────────────────────────────────────────
set(RESOURCES resources/resources.qrc)

# ── Platform-specific ────────────────────────────────────────────────────────
if(WIN32)
    set(WIN_ICON "")
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/app.rc")
        set(WIN_ICON resources/app.rc)
    endif()
    add_executable(DentaPro WIN32 ${SOURCES} ${HEADERS} ${RESOURCES} ${WIN_ICON})
elseif(APPLE)
    add_executable(DentaPro MACOSX_BUNDLE ${SOURCES} ${HEADERS} ${RESOURCES})
else()
    add_executable(DentaPro ${SOURCES} ${HEADERS} ${RESOURCES})
endif()

# ── Naziv EXE fajla ───────────────────────────────────────────────────────────
set_target_properties(DentaPro PROPERTIES
    OUTPUT_NAME "${APP_NAME}"
)

# ── Link ──────────────────────────────────────────────────────────────────────
target_link_libraries(DentaPro PRIVATE ${QT_LIBS})

# ── Include paths ─────────────────────────────────────────────────────────────
target_include_directories(DentaPro PRIVATE src)

# ── Install ───────────────────────────────────────────────────────────────────
install(TARGETS DentaPro
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION bin
)

# ── Post-build: windeployqt ───────────────────────────────────────────────────
if(WIN32)
    if(Qt6_FOUND)
        get_target_property(_qmake_executable Qt6::qmake IMPORTED_LOCATION)
    else()
        get_target_property(_qmake_executable Qt5::qmake IMPORTED_LOCATION)
    endif()
    get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")

    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET DentaPro POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}"
                    --verbose 0
                    --no-compiler-runtime
                    --no-translations
                    "$<TARGET_FILE:DentaPro>"
            COMMENT "Deploying Qt runtime DLLs..."
        )
    endif()
endif()
