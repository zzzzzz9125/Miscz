# ***** BEGIN LICENSE BLOCK *****
# This file is part of openfx-misc <https://github.com/NatronGitHub/openfx-misc>,
# Copyright (C) 2013-2018 INRIA
#
# openfx-misc is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# openfx-misc is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with openfx-Miscz.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
# ***** END LICENSE BLOCK *****

########################################################################
IF(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
MESSAGE(FATAL_ERROR "Prevented in-tree built. This is bad practice.")
ENDIF(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})

########################################################################
# Project setup
########################################################################
CMAKE_MINIMUM_REQUIRED(VERSION 3.1)
PROJECT(OpenFXMiscz CXX)
ENABLE_TESTING()

# ========================== build platform ==========================

MESSAGE (STATUS "  Platform:")
MESSAGE (STATUS "    Host:"             ${CMAKE_HOST_SYSTEM_NAME} ${CMAKE_HOST_SYSTEM_VERSION} ${CMAKE_HOST_SYSTEM_PROCESSOR})
if(CMAKE_CROSSCOMPILING)
  MESSAGE (STATUS "    Target:"         ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_VERSION} ${CMAKE_SYSTEM_PROCESSOR})
endif()
MESSAGE (STATUS "    CMake:"            ${CMAKE_VERSION})
MESSAGE (STATUS "    CMake generator:"  ${CMAKE_GENERATOR})
MESSAGE (STATUS "    CMake build tool:" ${CMAKE_BUILD_TOOL})
if(MSVC)
  MESSAGE (STATUS "    MSVC:"           ${MSVC_VERSION})
endif()
if(CMAKE_GENERATOR MATCHES Xcode)
  MESSAGE (STATUS "    Xcode:"          ${XCODE_VERSION})
endif()
if(NOT CMAKE_GENERATOR MATCHES "Xcode|Visual Studio")
  MESSAGE (STATUS "    Configuration:"  ${CMAKE_BUILD_TYPE})
endif()

include(FindOpenGL REQUIRED)

set (CMAKE_CXX_STANDARD 11)

# Use -Ofast when applicable (implies -ffast-math)
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set (CMAKE_CXX_FLAGS_RELEASE "-Wall -Wextra -Ofast -DNDEBUG")
    set (CMAKE_C_FLAGS_RELEASE   "-Wall -Wextra -Ofast -DNDEBUG")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
    set (CMAKE_CXX_FLAGS_RELEASE "-Wall -Ofast -DNDEBUG")
    set (CMAKE_C_FLAGS_RELEASE   "-Wall -Ofast -DNDEBUG")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set (CMAKE_CXX_FLAGS_RELEASE "-Wall -Wextra -Ofast -DNDEBUG")
    set (CMAKE_C_FLAGS_RELEASE   "-Wall -Wextra -Ofast -DNDEBUG")
endif()
SET(OPENFX_PATH ${CMAKE_CURRENT_SOURCE_DIR}/openfx)

# Check that submodule have been initialized and updated
if(NOT EXISTS ${PROJECT_SOURCE_DIR}/openfx/include)
  message(FATAL_ERROR
    "\n submodule(s) are missing, please update your repository:\n"
    "  > git submodule update -i\n")
endif()

# C++ Include directories
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/SupportExt)
INCLUDE_DIRECTORIES(${OPENFX_PATH}/include)
INCLUDE_DIRECTORIES(${OPENFX_PATH}/Support/include)
INCLUDE_DIRECTORIES(${OPENFX_PATH}/Support/Plugins/include)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/CImg)

# Define "DEBUG" on debug builds
SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")

# /MP on MSVC
ADD_COMPILE_OPTIONS($<$<CXX_COMPILER_ID:MSVC>:/MP>)

SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")

set(OFX_SUPPORT_HEADER_DIR "${OPENFX_PATH}/Support/include")
set(OFX_SUPPORT_LIBRARY_DIR "${OPENFX_PATH}/Support/Library")

FILE(GLOB SUPPORT_SOURCES
    "${OFX_SUPPORT_HEADER_DIR}/*.h"

    "${OFX_SUPPORT_LIBRARY_DIR}/ofxsCore.cpp"
    "${OFX_SUPPORT_LIBRARY_DIR}/ofxsImageEffect.cpp"
    "${OFX_SUPPORT_LIBRARY_DIR}/ofxsInteract.cpp"
    "${OFX_SUPPORT_LIBRARY_DIR}/ofxsLog.cpp"
    "${OFX_SUPPORT_LIBRARY_DIR}/ofxsMultiThread.cpp"
    "${OFX_SUPPORT_LIBRARY_DIR}/ofxsParams.cpp"
    "${OFX_SUPPORT_LIBRARY_DIR}/ofxsProperty.cpp"
    "${OFX_SUPPORT_LIBRARY_DIR}/ofxsPropertyValidation.cpp"
    "${OFX_SUPPORT_LIBRARY_DIR}/*.h"
)

#ADD_LIBRARY(Support STATIC )
#TARGET_COMPILE_DEFINITIONS(Support PRIVATE OFX_SUPPORTS_OPENGLRENDER OFX_EXTENSIONS_TUTTLE OFX_EXTENSIONS_NUKE OFX_EXTENSIONS_NATRON OFX_EXTENSIONS_VEGAS NOMINMAX)

FILE(GLOB MISC_SOURCES
  "Card3D/*.cpp"
  "CornerPin/*.cpp"
  "Position/*.cpp"
  "SpriteSheet/*.cpp"
  "Transform/*.cpp"
  "SupportExt/tinythread.cpp"
  "SupportExt/ofxsThreadSuite.cpp"
  "SupportExt/ofxsFileOpen.cpp"
  "SupportExt/ofxsGenerator.cpp"
  "SupportExt/ofxsOGLTextRenderer.cpp"
  "SupportExt/ofxsOGLFontData.cpp"
  "SupportExt/ofxsRamp.cpp"
  "SupportExt/ofxsShutter.cpp"
  "SupportExt/ofxsTransform3x3.cpp"
  "SupportExt/ofxsTransformInteract.cpp"
  "SupportExt/ofxsRectangleInteract.cpp"
  "SupportExt/*.h"
)

ADD_LIBRARY(Miscz SHARED ${MISC_SOURCES} ${SUPPORT_SOURCES})
SET_TARGET_PROPERTIES(Miscz PROPERTIES PREFIX "")
SET_TARGET_PROPERTIES(Miscz PROPERTIES SUFFIX ".ofx")

# Add extra flags to the link step of the plugin
if(APPLE)
    set_target_properties(Miscz PROPERTIES LINK_FLAGS "-fvisibility=hidden -exported_symbols_list,${OFX_SUPPORT_HEADER_DIR}/osxSymbols")
    set_target_properties(Miscz PROPERTIES INSTALL_RPATH "@loader_path/../Frameworks;@loader_path/../Libraries")
elseif(MINGW)
    set_target_properties(Miscz PROPERTIES LINK_FLAGS "-shared -fvisibility=hidden -Xlinker --version-script=${OFX_SUPPORT_HEADER_DIR}/linuxSymbols")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
# Linux & FreeBSD
    set_target_properties(Miscz PROPERTIES LINK_FLAGS "-Wl,-fvisibility=hidden,--version-script=${OFX_SUPPORT_HEADER_DIR}/linuxSymbols")
    set_target_properties(Miscz PROPERTIES INSTALL_RPATH "$ORIGIN/../../Libraries")
endif()

TARGET_COMPILE_DEFINITIONS(Miscz PRIVATE OFX_EXTENSIONS_VEGAS OFX_EXTENSIONS_NUKE OFX_EXTENSIONS_NATRON OFX_EXTENSIONS_TUTTLE OFX_SUPPORTS_OPENGLRENDER NOMINMAX)
#TARGET_LINK_LIBRARIES(Miscz Support ${OPENGL_gl_LIBRARY})
TARGET_LINK_LIBRARIES(Miscz ${OPENGL_gl_LIBRARY})

FILE(GLOB CIMG_SOURCES
#  "CImg/CImg.h"
#  "CImg/CImgFilter.cpp"
#  "CImg/CImgFilter.h"
#  "CImg/CImgOperator.cpp"
#  "CImg/CImgOperator.h"
#  "CImg/Bilateral/CImgBilateral.cpp"
#  "CImg/Blur/CImgBlur.cpp"
#  "CImg/Blur/blurmaple.map"
#  "CImg/Denoise/CImgDenoise.cpp"
#  "CImg/Distance/CImgDistance.cpp"
#  "CImg/Equalize/CImgEqualize.cpp"
#  "CImg/Erode/CImgDilate.cpp"
#  "CImg/Erode/CImgErode.cpp"
#  "CImg/ErodeSmooth/CImgErodeSmooth.cpp"
#  "CImg/Expression/CImgExpression.cpp"
#  "CImg/Guided/CImgGuided.cpp"
#  "CImg/HistEQ/CImgHistEQ.cpp"
#  "CImg/Inpaint/CImgInpaint-gpl.cpp"
#  "CImg/Inpaint/inpaint.h"
#  "CImg/Matrix/CImgMatrix.cpp"
#  "CImg/Median/CImgMedian.cpp"
#  "CImg/Noise/CImgNoise.cpp"
#  "CImg/Plasma/CImgPlasma.cpp"
#  "CImg/RollingGuidance/CImgRollingGuidance.cpp"
#  "CImg/SharpenInvDiff/CImgSharpenInvDiff.cpp"
#  "CImg/SharpenShock/CImgSharpenShock.cpp"
#  "CImg/Smooth/CImgSmooth.cpp"
  "SupportExt/tinythread.cpp"
  "SupportExt/ofxsThreadSuite.cpp"
#  "SupportExt/ofxsFileOpen.cpp"
#  "SupportExt/ofxsGenerator.cpp"
  "SupportExt/ofxsLut.cpp"
#  "SupportExt/ofxsMultiPlane.cpp"
#  "SupportExt/ofxsOGLTextRenderer.cpp"
#  "SupportExt/ofxsOGLFontData.cpp"
#  "SupportExt/ofxsRamp.cpp"
#  "SupportExt/ofxsShutter.cpp"
#  "SupportExt/ofxsTracking.cpp"
#  "SupportExt/ofxsTransform3x3.cpp"
#  "SupportExt/ofxsTransformInteract.cpp"
#  "SupportExt/ofxsRectangleInteract.cpp"
  "SupportExt/*.h"
)

# FILE(GLOB CIMG_RESOURCES
#  "CImg/Bilateral/net.sf.cimg.CImgBilateral.png"
#  "CImg/Bilateral/net.sf.cimg.CImgBilateral.svg"
#  "CImg/Blur/net.sf.cimg.CImgBlur.png"
#  "CImg/Blur/net.sf.cimg.CImgBlur.svg"
#  "CImg/Blur/net.sf.cimg.CImgChromaBlur.png"
#  "CImg/Blur/net.sf.cimg.CImgChromaBlur.svg"
#  "CImg/Blur/net.sf.cimg.CImgBloom.png"
#  "CImg/Blur/net.sf.cimg.CImgBloom.svg"
#  "CImg/Blur/eu.cimg.EdgeDetect.png"
#  "CImg/Blur/eu.cimg.EdgeDetect.svg"
#  "CImg/Blur/eu.cimg.EdgeExtend.png"
#  "CImg/Blur/eu.cimg.EdgeExtend.svg"
#  "CImg/Blur/fr.inria.EdgeBlur.png"
#  "CImg/Blur/fr.inria.EdgeBlur.svg"
#  "CImg/Blur/net.sf.cimg.CImgLaplacian.png"
#  "CImg/Blur/net.sf.cimg.CImgLaplacian.svg"
#  "CImg/Blur/net.sf.cimg.CImgSharpen.png"
#  "CImg/Blur/net.sf.cimg.CImgSharpen.svg"
#  "CImg/Denoise/net.sf.cimg.CImgDenoise.png"
#  "CImg/Denoise/net.sf.cimg.CImgDenoise.svg"
#  "CImg/Equalize/net.sf.cimg.CImgEqualize.png"
#  "CImg/Equalize/net.sf.cimg.CImgEqualize.svg"
#  "CImg/Erode/net.sf.cimg.CImgDilate.png"
#  "CImg/Erode/net.sf.cimg.CImgDilate.svg"
#  "CImg/Erode/net.sf.cimg.CImgErode.png"
#  "CImg/Erode/net.sf.cimg.CImgErode.svg"
#  "CImg/ErodeSmooth/net.sf.cimg.CImgErodeSmooth.png"
#  "CImg/ErodeSmooth/net.sf.cimg.CImgErodeSmooth.svg"
#  "CImg/Expression/net.sf.cimg.CImgExpression.png"
#  "CImg/Expression/net.sf.cimg.CImgExpression.svg"
#  "CImg/Guided/net.sf.cimg.CImgGuided.png"
#  "CImg/Guided/net.sf.cimg.CImgGuided.svg"
#  "CImg/HistEQ/net.sf.cimg.CImgHistEQ.png"
#  "CImg/HistEQ/net.sf.cimg.CImgHistEQ.svg"
#  "CImg/Inpaint/eu.cimg.Inpaint.png"
#  "CImg/Inpaint/eu.cimg.Inpaint.svg"
#  "CImg/Noise/net.sf.cimg.CImgNoise.png"
#  "CImg/Noise/net.sf.cimg.CImgNoise.svg"
#  "CImg/Plasma/net.sf.cimg.CImgPlasma.png"
#  "CImg/Plasma/net.sf.cimg.CImgPlasma.svg"
#  "CImg/RollingGuidance/net.sf.cimg.CImgRollingGuidance.png"
#  "CImg/RollingGuidance/net.sf.cimg.CImgRollingGuidance.svg"
#  "CImg/SharpenInvDiff/net.sf.cimg.CImgSharpenInvDiff.png"
#  "CImg/SharpenInvDiff/net.sf.cimg.CImgSharpenInvDiff.svg"
#  "CImg/SharpenShock/net.sf.cimg.CImgSharpenShock.png"
#  "CImg/Smooth/net.sf.cimg.CImgSmooth.png"
#  "CImg/Smooth/net.sf.cimg.CImgSmooth.svg"
# )


ADD_LIBRARY(CImg SHARED ${CIMG_SOURCES} ${SUPPORT_SOURCES})
SET_TARGET_PROPERTIES(CImg PROPERTIES PREFIX "")
SET_TARGET_PROPERTIES(CImg PROPERTIES SUFFIX ".ofx")

# Add extra flags to the link step of the plugin
if(APPLE)
    set_target_properties(CImg PROPERTIES LINK_FLAGS "-bundle -fvisibility=hidden -exported_symbols_list,${OFX_SUPPORT_HEADER_DIR}/osxSymbols")
    set_target_properties(CImg PROPERTIES INSTALL_RPATH "@loader_path/../Frameworks;@loader_path/../Libraries")
elseif(WIN32)
    set_target_properties(CImg PROPERTIES LINK_FLAGS "-fvisibility=hidden,--version-script=${OFX_SUPPORT_HEADER_DIR}/linuxSymbols")
else()
# Linux & FreeBSD
    set_target_properties(CImg PROPERTIES LINK_FLAGS "-Wl,-fvisibility=hidden,--version-script=${OFX_SUPPORT_HEADER_DIR}/linuxSymbols")
    set_target_properties(CImg PROPERTIES INSTALL_RPATH "$ORIGIN/../../Libraries")
endif()

TARGET_COMPILE_DEFINITIONS(CImg PRIVATE OFX_EXTENSIONS_VEGAS OFX_EXTENSIONS_NUKE OFX_EXTENSIONS_NATRON OFX_EXTENSIONS_TUTTLE OFX_SUPPORTS_OPENGLRENDER NOMINMAX)
#TARGET_LINK_LIBRARIES(CImg Support)
TARGET_LINK_LIBRARIES(CImg)

# Find OpenMP
# To compile on homebrew:
# env CXX=/usr/local/opt/llvm/bin/clang++ CC=/usr/local/opt/llvm/bin/clang LDFLAGS=-L/usr/local/opt/llvm/lib cmake ..
find_package(OpenMP)
# MSVC is OpenMP 2.0 and has issues, see https://github.com/dtschump/CImg/issues/208 and https://msdn.microsoft.com/fr-fr/library/ff520162.aspx
if (OPENMP_FOUND AND NOT MSVC)
#if (OPENMP_FOUND)
    #include_directories("${OPENMP_INCLUDES}")
    #link_directories("${OPENMP_LIBRARIES}")
    # set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    # set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    # set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${OpenMP_SHARED_LINKER_FLAGS}")
    # set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
    SET_TARGET_PROPERTIES(CImg PROPERTIES COMPILE_FLAGS "${OpenMP_CXX_FLAGS}")
    SET_TARGET_PROPERTIES(CImg PROPERTIES LINK_FLAGS "${OpenMP_EXE_LINKER_FLAGS}")
    TARGET_LINK_LIBRARIES(CImg ${OpenMP_CXX_LIB_NAMES})
endif(OPENMP_FOUND AND NOT MSVC)

IF (MSVC)
  # Some files require this option. This breaks compatibility with older linkers.
  SET_TARGET_PROPERTIES(Miscz PROPERTIES COMPILE_FLAGS "/bigobj")
  # If the build system is at least Windows Vista (6.0),
  # compile for Windows Vista, to get interruptible CImg plugins (see CImgFilter.h)
  IF (NOT ${CMAKE_SYSTEM_VERSION} VERSION_LESS 6.0) # Windows Vista
    ADD_DEFINITIONS(-D_WIN32_WINNT=0x0600 -DWINVER=0x0600)
    MESSAGE(STATUS "CImg plugins should be interruptible")
  ELSE ()
    MESSAGE(STATUS "CImg plugins may not be interruptible")
  ENDIF ()
ENDIF ()

# Find and set the arch name.
# http://openeffects.org/documentation/reference/ch02s02.html
SET(OFX_ARCH UNKNOWN)
# Win
if(MINGW)
    set(OFX_ARCH "Win32")
    if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")
        set(OFX_ARCH "Win64")
    endif()
# Visual Studio    
elseif(MSVC)
    set(OFX_ARCH "Win32")
    if((${CMAKE_SYSTEM_PROCESSOR} STREQUAL "IA64") OR (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "AMD64"))
        set(OFX_ARCH "Win64")
    endif()
# FreeBSD
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
    set(OFX_ARCH "FreeBSD-x86")
    if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")
        set(OFX_ARCH "FreeBSD-x86-64")
    endif()
    if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm")
        if("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
            set(OFX_ARCH FreeBSD-arm-64)
        else()
            set(OFX_ARCH FreeBSD-arm)
        endif()
    endif()
# MacOS
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    set(OFX_ARCH "MacOS")
# Linux
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set(OFX_ARCH "Linux-x86")
    if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")
        set(OFX_ARCH "Linux-x86-64")
    endif()
    if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm")
        if("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
            set(OFX_ARCH Linux-arm-64)
        else()
            set(OFX_ARCH Linux-arm)
        endif()
    endif()
endif()

SET(OFX_ARCH_NAME ${OFX_ARCH} CACHE STRING "OpenFX target OS and architecture")
IF("${OFX_ARCH_NAME}" EQUAL "UNKNOWN")
  MESSAGE(SEND_ERROR "OFX_ARCH_NAME was not detected. Install targets will be unavailable. See http://openeffects.org/documentation/reference/ch02s02.html to pick an arch name.")
ELSE()
  INSTALL(TARGETS Miscz
    RUNTIME DESTINATION Miscz.ofx.bundle/Contents/${OFX_ARCH_NAME}
    LIBRARY DESTINATION Miscz.ofx.bundle/Contents/${OFX_ARCH_NAME}
  )

#  INSTALL(DIRECTORY "Shadertoy/presets"
#    DESTINATION Miscz.ofx.bundle/Contents/Resources
#  )

#  INSTALL(FILES ${MISC_RESOURCES}
#    DESTINATION Miscz.ofx.bundle/Contents/Resources
#  )

  INSTALL(FILES Info.plist
    DESTINATION Miscz.ofx.bundle/Contents
  )
#  INSTALL(TARGETS CImg
#    RUNTIME DESTINATION CImg.ofx.bundle/Contents/${OFX_ARCH_NAME}
#    LIBRARY DESTINATION CImg.ofx.bundle/Contents/${OFX_ARCH_NAME}
#  )

#  INSTALL(FILES ${CIMG_RESOURCES}
#    DESTINATION CImg.ofx.bundle/Contents/Resources
#  )

#  INSTALL(FILES CImg/Info.plist
#    DESTINATION CImg.ofx.bundle/Contents
#  )
ENDIF()
