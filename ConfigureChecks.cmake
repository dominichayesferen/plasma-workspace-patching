set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH} )
include(CheckTypeSize)
include(FindPkgConfig)

# find_package(XKB) # kxkb, kdm

set(KWIN_BIN "kwin_x11" CACHE STRING "Name of the KWin binary")

find_program(some_x_program NAMES iceauth xrdb xterm)
if (NOT some_x_program)
    set(some_x_program /usr/bin/xrdb)
    message("Warning: Could not determine X binary directory. Assuming /usr/bin.")
endif (NOT some_x_program)
get_filename_component(proto_xbindir "${some_x_program}" PATH)
get_filename_component(XBINDIR "${proto_xbindir}" ABSOLUTE)
get_filename_component(xrootdir "${XBINDIR}" PATH)
set(XLIBDIR "${xrootdir}/lib/X11")

check_function_exists(statvfs HAVE_STATVFS)

check_include_files(limits.h HAVE_LIMITS_H)
check_include_files(sys/time.h HAVE_SYS_TIME_H)     # ksmserver, ksplashml, sftp
check_include_files(stdint.h HAVE_STDINT_H)         # kcontrol/kfontinst
check_include_files("sys/stat.h;sys/vfs.h" HAVE_SYS_VFS_H) # statvfs for plasma/solid
check_include_files("sys/stat.h;sys/statvfs.h" HAVE_SYS_STATVFS_H) # statvfs for plasma/solid
check_include_files(sys/param.h HAVE_SYS_PARAM_H)
check_include_files("sys/param.h;sys/mount.h" HAVE_SYS_MOUNT_H)
check_include_files("sys/types.h;sys/statfs.h" HAVE_SYS_STATFS_H)
check_include_files(unistd.h HAVE_UNISTD_H)
check_include_files(malloc.h HAVE_MALLOC_H)
check_function_exists(statfs HAVE_STATFS)
set(HAVE_FONTCONFIG FONTCONFIG_FOUND) # kcontrol/{fonts,kfontinst}
set(HAVE_OPENGL OPENGL_FOUND) # kwin
set(HAVE_XSHM X11_XShm_FOUND) # kwin, ksplash
set(HAVE_XTEST X11_XTest_FOUND) # khotkeys, kxkb, kdm
set(HAVE_XCOMPOSITE X11_Xcomposite_FOUND) # kicker, kwin
set(HAVE_XCURSOR X11_Xcursor_FOUND) # many uses
set(HAVE_XDAMAGE X11_Xdamage_FOUND) # kwin
set(HAVE_XFIXES X11_Xfixes_FOUND) # klipper, kicker, kwin
set(HAVE_XINERAMA X11_Xinerama_FOUND)
set(HAVE_XRANDR X11_Xrandr_FOUND) # kwin
set(HAVE_XRENDER X11_Xrender_FOUND) # kcontrol/style, kicker
set(HAVE_XF86MISC X11_xf86misc_FOUND) # kdesktop and kcontrol/lock
set(HAVE_DPMS X11_dpms_FOUND) # kdesktop
set(HAVE_XSYNC X11_XSync_FOUND) # kwin

set(CMAKE_EXTRA_INCLUDE_FILES sys/socket.h)
check_type_size("struct ucred" STRUCT_UCRED)       # kio_fonts

check_function_exists(getpeereid  HAVE_GETPEEREID) # kdesu
check_function_exists(setpriority  HAVE_SETPRIORITY) # kscreenlocker 

set(CMAKE_REQUIRED_INCLUDES ${X11_Xrandr_INCLUDE_PATH}/Xrandr.h)
set(CMAKE_REQUIRED_LIBRARIES ${X11_Xrandr_LIB})
check_function_exists(XRRGetScreenSizeRange XRANDR_1_2_FOUND)
set(HAS_RANDR_1_2 XRANDR_1_2_FOUND)
check_function_exists(XRRGetScreenResourcesCurrent XRANDR_1_3_FOUND)
set(HAS_RANDR_1_3 XRANDR_1_3_FOUND)
