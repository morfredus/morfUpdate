cmake_minimum_required(VERSION 3.21)

# Version pilotee par le fichier VERSION (meme convention que ComponentHub / SiteWatch).
set(MORFUPDATE_VERSION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/VERSION")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${MORFUPDATE_VERSION_FILE}")
file(STRINGS "${MORFUPDATE_VERSION_FILE}" MORFUPDATE_VERSION_FROM_FILE LIMIT_COUNT 1)
string(STRIP "${MORFUPDATE_VERSION_FROM_FILE}" MORFUPDATE_VERSION_FROM_FILE)

project(morfUpdate VERSION "${MORFUPDATE_VERSION_FROM_FILE}" LANGUAGES CXX)

# --- Standard C++ ---------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# --- Coeur : detection de version (Qt Core + Network, PAS de Widgets) ------
# Utilisable dans un service sans interface. La couche UI est separee.
find_package(Qt6 REQUIRED COMPONENTS Core Network)

add_library(morfUpdate STATIC
    src/Version.cpp
    src/GitHubReleaseSource.cpp
    src/UpdateChecker.cpp
    include/morfupdate/Version.h
    include/morfupdate/ReleaseInfo.h
    include/morfupdate/morfUpdateConfig.h
    include/morfupdate/IUpdateSource.h
    include/morfupdate/GitHubReleaseSource.h
    include/morfupdate/UpdateChecker.h
)
add_library(morfUpdate::morfUpdate ALIAS morfUpdate)

target_include_directories(morfUpdate PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(morfUpdate PUBLIC Qt6::Core Qt6::Network)
target_compile_definitions(morfUpdate PUBLIC MORFUPDATE_VERSION="${PROJECT_VERSION}")

if(MSVC)
    target_compile_options(morfUpdate PRIVATE /utf-8)
endif()

# --- Couche UI optionnelle : dialogue de notification (Qt Widgets) --------
option(MORFUPDATE_BUILD_WIDGETS "Compiler la couche UI (Qt Widgets)" ON)
if(MORFUPDATE_BUILD_WIDGETS)
    find_package(Qt6 REQUIRED COMPONENTS Widgets)
    add_library(morfUpdateWidgets STATIC
        src/UpdateDialog.cpp
        include/morfupdate/UpdateDialog.h
    )
    add_library(morfUpdate::Widgets ALIAS morfUpdateWidgets)
    target_link_libraries(morfUpdateWidgets PUBLIC
        morfUpdate::morfUpdate Qt6::Widgets)
    if(MSVC)
        target_compile_options(morfUpdateWidgets PRIVATE /utf-8)
    endif()
endif()

# --- Exemple (build autonome uniquement) ----------------------------------
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(_um_standalone ON)
else()
    set(_um_standalone OFF)
endif()
option(MORFUPDATE_BUILD_EXAMPLES "Compiler l'exemple de demonstration" ${_um_standalone})

if(MORFUPDATE_BUILD_EXAMPLES)
    add_subdirectory(examples/minimal)
    if(MORFUPDATE_BUILD_WIDGETS)
        add_subdirectory(examples/widget)
    endif()
endif()
