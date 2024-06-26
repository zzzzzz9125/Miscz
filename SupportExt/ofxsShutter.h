/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of openfx-supportext <https://github.com/NatronGitHub/openfx-supportext>,
 * (C) 2018-2021 The Natron Developers
 * (C) 2013-2018 INRIA
 *
 * openfx-supportext is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * openfx-supportext is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with openfx-supportext.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

/*
 * OFX Shutter parameter support
 */

#ifndef openfx_supportext_ofxsShutter_h
#define openfx_supportext_ofxsShutter_h

#include <memory>

#include "ofxsImageEffect.h"
#include "ofxsMacros.h"


#define kParamShutter "shutter"
#define kParamShutterLabel "Shutter"
#define kParamShutterHint "Controls how long (in frames) the shutter should remain open."

#define kParamShutterOffset "shutterOffset"
#define kParamShutterOffsetLabel "Shutter Offset"
#define kParamShutterOffsetHint "Controls when the shutter should be open/closed. Ignored if there is no motion blur (i.e. shutter=0 or motionBlur=0)."
#define kParamShutterOffsetOptionCentered "Centered", "Centers the shutter around the frame (from t-shutter/2 to t+shutter/2).", "centered"
#define kParamShutterOffsetOptionStart "Start", "Open the shutter at the frame (from t to t+shutter).", "start"
#define kParamShutterOffsetOptionEnd "End", "Close the shutter at the frame (from t-shutter to t).", "end"
#define kParamShutterOffsetOptionCustom "Custom", "Open the shutter at t+shuttercustomoffset (from t+shuttercustomoffset to t+shuttercustomoffset+shutter).", "custom"

#define kGroupMotionBlur "motionBlurGroup"
#define kGroupMotionBlurLabel "Motion Blur"
enum ShutterOffsetEnum
{
    eShutterOffsetCentered,
    eShutterOffsetStart,
    eShutterOffsetEnd,
    eShutterOffsetCustom
};


#define kParamShutterCustomOffset "shutterCustomOffset"
#define kParamShutterCustomOffsetLabel "Custom Offset"
#define kParamShutterCustomOffsetHint "When custom is selected, the shutter is open at current time plus this offset (in frames). Ignored if there is no motion blur (i.e. shutter=0 or motionBlur=0)."

namespace OFX {
void shutterDescribeInContext(OFX::ImageEffectDescriptor &desc, OFX::ContextEnum context, OFX::PageParamDescriptor* page, OFX::GroupParamDescriptor* group);
void shutterRange(double time, double shutter, ShutterOffsetEnum shutteroffset, double shuttercustomoffset, OfxRangeD* range);
}
#endif /* defined(openfx_supportext_ofxsShutter_h) */
