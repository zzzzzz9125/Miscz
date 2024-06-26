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

#include "ofxsShutter.h"

using namespace OFX;

namespace OFX {
void
shutterDescribeInContext(ImageEffectDescriptor &desc,
                         ContextEnum /*context*/,
                         PageParamDescriptor* page,
                         GroupParamDescriptor* group)
{
    // shutter
    {
        DoubleParamDescriptor* param = desc.defineDoubleParam(kParamShutter);
        param->setLabel(kParamShutterLabel);
        param->setHint(kParamShutterHint);
        param->setDefault(0.5);
        param->setIncrement(0.01);
        param->setRange(0., 2.);
        param->setDisplayRange(0., 2.);
        if (group) {
            param->setParent(*group);
        }
        if (page) {
            page->addChild(*param);
        }
    }

    // shutteroffset
    {
        ChoiceParamDescriptor* param = desc.defineChoiceParam(kParamShutterOffset);
        param->setLabel(kParamShutterOffsetLabel);
        param->setHint(kParamShutterOffsetHint);
        assert(param->getNOptions() == eShutterOffsetCentered);
        param->appendOption(kParamShutterOffsetOptionCentered);
        assert(param->getNOptions() == eShutterOffsetStart);
        param->appendOption(kParamShutterOffsetOptionStart);
        assert(param->getNOptions() == eShutterOffsetEnd);
        param->appendOption(kParamShutterOffsetOptionEnd);
        assert(param->getNOptions() == eShutterOffsetCustom);
        param->appendOption(kParamShutterOffsetOptionCustom);
        param->setAnimates(true);
        param->setDefault(eShutterOffsetStart);
        if (group) {
            param->setParent(*group);
        }
        if (page) {
            page->addChild(*param);
        }
    }

    // shuttercustomoffset
    {
        DoubleParamDescriptor* param = desc.defineDoubleParam(kParamShutterCustomOffset);
        param->setLabel(kParamShutterCustomOffsetLabel);
        param->setHint(kParamShutterCustomOffsetHint);
        param->setDefault(0.);
        param->setIncrement(0.1);
        param->setRange(-1., 1.);
        param->setDisplayRange(-1., 1.);
        if (group) {
            param->setParent(*group);
        }
        if (page) {
            page->addChild(*param);
        }
    }
} // shutterDescribeInContext

void
shutterRange(double time,
             double shutter,
             ShutterOffsetEnum shutteroffset,
             double shuttercustomoffset,
             OfxRangeD* range)
{
    switch (shutteroffset) {
    case eShutterOffsetCentered:
        range->min = time - shutter / 2;
        range->max = time + shutter / 2;
        break;
    case eShutterOffsetStart:
        range->min = time;
        range->max = time + shutter;
        break;
    case eShutterOffsetEnd:
        range->min = time - shutter;
        range->max = time;
        break;
    case eShutterOffsetCustom:
        range->min = time + shuttercustomoffset;
        range->max = time + shuttercustomoffset + shutter;
        break;
    default:
        range->min = time;
        range->max = time;
        break;
    }
}
} // namespace OFX
