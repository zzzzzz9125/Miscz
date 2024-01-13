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

#ifndef openfx_supportext_ofxsOGLHiDPI_h
#define openfx_supportext_ofxsOGLHiDPI_h

#include <memory>

#include "ofxsImageEffect.h"
#include "ofxsMacros.h"


#define kParamHiDPI "hidpi"
#define kParamHiDPILabel "HiDPI"
#define kParamHiDPIHint "Should be checked when the display area is High-DPI (a.k.a Retina). Draws OpenGL overlays twice larger."

namespace OFX {

inline void hiDPIDescribeParams(OFX::ImageEffectDescriptor &desc, GroupParamDescriptor* group, OFX::PageParamDescriptor* page)
{
    if ( desc.getParamDescriptor(kParamHiDPI) ) {
        // hiDPIDescribeParams() may be called several times (eg SeNoise which has both Transform andd Ramp interacts)
        return;
    }

    BooleanParamDescriptor* param = desc.defineBooleanParam(kParamHiDPI);
    param->setLabel(kParamHiDPILabel);
    param->setHint(kParamHiDPIHint);
    param->setAnimates(false);
    param->setIsSecret(true);
    param->setEvaluateOnChange(false);
    if (group) {
        param->setParent(*group);
    }
    if (page) {
        page->addChild(*param);
    }
}

}
#endif /* openfx_supportext_ofxsOGLHiDPI_h */
