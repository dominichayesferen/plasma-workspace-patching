# KI18N Translation Domain for this library
add_definitions(-DTRANSLATION_DOMAIN=\"kcm_desktoplayout\")

if(X11_Xcursor_FOUND)
    set(HAVE_XCURSOR TRUE)
endif()

if(${Breeze_FOUND} AND ${BREEZE_WITH_KDECORATION})
    set(HAVE_BREEZE_DECO TRUE)
endif()

configure_file (config-kcm.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-kcm.h )

set(kcm_desktoplayout_SRCS
    kcm.cpp
    ../kcms-common_p.h
)

if (X11_Xcursor_FOUND)
    set(kcm_desktoplayout_SRCS
        ${kcm_desktoplayout_SRCS}
        ../cursortheme/xcursor/cursortheme.cpp
        ../cursortheme/xcursor/xcursortheme.cpp
    )
endif ()

kcmutils_generate_module_data(
    kcm_desktoplayout_SRCS
    MODULE_DATA_HEADER desktoplayoutdata.h
    MODULE_DATA_CLASS_NAME DesktopLayoutData
    SETTINGS_HEADERS desktoplayoutsettings.h
    SETTINGS_CLASSES DesktopLayoutSettings
)
kconfig_add_kcfg_files(kcm_desktoplayout_SRCS desktoplayoutsettings.kcfgc GENERATE_MOC)

add_library(kcm_desktoplayout_static STATIC ${kcm_desktoplayout_SRCS})
target_link_libraries(kcm_desktoplayout_static
    Qt::DBus
    KF5::I18n
    KF5::KCMUtils
    KF5::IconThemes
    KF5::JobWidgets
    KF5::Service
    KF5::QuickAddons
    KF5::KIOWidgets
    KF5::Declarative
    PW::KWorkspace
    krdb
)
if(X11_FOUND)
    target_link_libraries(kcm_desktoplayout_static Qt::X11Extras)
endif()
if (X11_Xcursor_FOUND)
   target_link_libraries(kcm_desktoplayout_static X11::Xcursor)
endif ()
if (X11_Xfixes_FOUND)
   target_link_libraries(kcm_desktoplayout_static X11::Xfixes)
endif ()

add_library(kcm_desktoplayout MODULE kcmmain.cpp)
kcoreaddons_desktop_to_json(kcm_desktoplayout "kcm_desktoplayout.desktop" SERVICE_TYPES kcmodule.desktop)
target_link_libraries(kcm_desktoplayout kcm_desktoplayout_static)

#this desktop file is installed only for retrocompatibility with sycoca
install(FILES desktoplayoutsettings.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR})
install(FILES kcm_desktoplayout.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})
set_target_properties(kcm_desktoplayout PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/kcms")
install(TARGETS kcm_desktoplayout DESTINATION ${KDE_INSTALL_PLUGINDIR}/kcms)

kpackage_install_package(package kcm_desktoplayout kcms)

###### Command line tool

kconfig_add_kcfg_files(plasma-apply-desktoplayout_SRCS desktoplayoutsettings.kcfgc GENERATE_MOC)

add_executable(plasma-apply-desktoplayout dltool.cpp)

target_link_libraries(plasma-apply-desktoplayout kcm_desktoplayout_static)

ADD_CUSTOM_TARGET(plasma-apply-desktoplayout_link ALL
                  COMMAND ${CMAKE_COMMAND} -E create_symlink ${KDE_INSTALL_FULL_BINDIR}/plasma-apply-desktoplayout ${CMAKE_CURRENT_BINARY_DIR}/desktoplayouttool)
install( FILES ${CMAKE_CURRENT_BINARY_DIR}/desktoplayouttool DESTINATION ${KDE_INSTALL_FULL_BINDIR}/ )
install( TARGETS plasma-apply-desktoplayout ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )
