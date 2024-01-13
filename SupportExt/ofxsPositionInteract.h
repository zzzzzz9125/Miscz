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
 * OFX generic position interact.
 */

#ifndef openfx_supportext_ofxsPositionInteract_h
#define openfx_supportext_ofxsPositionInteract_h

#include <cmath>

#ifdef __APPLE__
#ifndef GL_SILENCE_DEPRECATION
#define GL_SILENCE_DEPRECATION // Yes, we are still doing OpenGL 2.1
#endif
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include <ofxsInteract.h>
#include <ofxsImageEffect.h>
#include "ofxsOGLTextRenderer.h"
#include "ofxsOGLHiDPI.h"
#include "ofxsMacros.h"

#define kDrawPointTrajectoryMaxPoints 1000
#define kDrawPointTrajectoryMinStepsPerKey 8

namespace OFX {

// Before calling this function: make sure the OpenGL props are correctly set.
// Try to draw one point per frame, but no more thankDrawPointTrajectoryMaxPoints
// in total, and at least kDrawPointTrajectoryMinStepsPerKey between two keys.
inline void
drawPointTrajectory(Double2DParam* p)
{
    int numKeys = p->getNumKeys();

    if (numKeys > 0) {
        int maxStepsPerKey = int(kDrawPointTrajectoryMaxPoints / numKeys);
        glBegin(GL_POINTS);
        for (int i=0; i < numKeys; ++i) {
            double time = p->getKeyTime(i);
            OfxPointD pt;
            p->getValueAtTime(time, pt.x, pt.y);
            glVertex2d(pt.x, pt.y);

        }
        glEnd();
        glBegin(GL_LINE_STRIP);
        double time = p->getKeyTime(0);
        for (int i = 1; i < numKeys; ++i) {
            double timeNext = p->getKeyTime(i);
            int steps = std::max(kDrawPointTrajectoryMinStepsPerKey, std::min(int(timeNext - time), maxStepsPerKey));
            for (int j = (i == 1 ? 0 : 1); j <= steps; ++j) {
                double timeStep = time + j * (timeNext - time) / steps;

                OfxPointD pt;
                p->getValueAtTime(timeStep, pt.x, pt.y);
                glVertex2d(pt.x, pt.y);
            }
            time = timeNext;
        }
        glEnd();
    }
}


/// template for a generic position interact.
/*
   The PositionInteractParam class must define a static name() function, returning the OFX parameter name.
   (using const char* directly as template parameter is not reliable) :
   namespace {
   struct MyPositionInteractParam {
     static const char *name() { return kMyName; }
   };
   }

   // the describe() function should include the declaration of the interact:
   desc.setOverlayInteractDescriptor(new PositionOverlayDescriptor<MyPositionInteractParam>);

   // The position param should be defined is describeInContext() as follows:
   Double2DParamDescriptor* position = desc.defineDouble2DParam(kMyName);
   position->setLabel(kMyLabel, kMyLabel, kMyLabel);
   position->setHint(kMyHint);
   position->setDoubleType(eDoubleTypeXYAbsolute);
   position->setDefaultCoordinateSystem(eCoordinatesNormalised);
   position->setDefault(0.5, 0.5);
   if (page) {
       page->addChild(*position);
   }
 */
template<typename PositionInteractParam>
class PositionInteract
    : public OFX::OverlayInteract
{
public:
    PositionInteract(OfxInteractHandle handle,
                     OFX::ImageEffect* effect)
        : OFX::OverlayInteract(handle)
        , _state(eMouseStateInactive)
        , _position(NULL)
        , _interactive(NULL)
        , _hiDPI(NULL)
        , _interactiveDrag(false)
        , _hasNativeHostPositionHandle(false)
    {
        _position = effect->fetchDouble2DParam( PositionInteractParam::name() );
        assert(_position);
        addParamToSlaveTo(_position);
        if ( PositionInteractParam::interactiveName() ) {
            _interactive = effect->fetchBooleanParam( PositionInteractParam::interactiveName() );
            assert(_interactive);
        }
        if ( effect->paramExists(kParamHiDPI) ) {
            _hiDPI = effect->fetchBooleanParam(kParamHiDPI);
            assert(_hiDPI);
            addParamToSlaveTo(_hiDPI);
        }
        _hasNativeHostPositionHandle = _position->getHostHasNativeOverlayHandle();
        _penPosition.x = _penPosition.y = 0;
    }

private:
    // overridden functions from OFX::Interact to do things
    virtual bool draw(const OFX::DrawArgs &args) OVERRIDE FINAL;
    virtual bool penMotion(const OFX::PenArgs &args) OVERRIDE FINAL;
    virtual bool penDown(const OFX::PenArgs &args) OVERRIDE FINAL;
    virtual bool penUp(const OFX::PenArgs &args) OVERRIDE FINAL;
    virtual void loseFocus(const FocusArgs &args) OVERRIDE FINAL;

private:
    enum MouseStateEnum
    {
        eMouseStateInactive,
        eMouseStatePoised,
        eMouseStatePicked
    };

    MouseStateEnum _state;
    OFX::Double2DParam* _position;
    OFX::BooleanParam* _interactive;
    OFX::BooleanParam* _hiDPI;
    OfxPointD _penPosition;
    bool _interactiveDrag;
    bool _hasNativeHostPositionHandle;

    double pointSize() const
    {
        return 5;
    }

    double pointTolerance() const
    {
        return 6;
    }

    // round to the closest int, 1/10 int, etc
    // this make parameter editing easier
    // pscale is args.pixelScale.x / args.renderScale.x;
    // pscale10 is the power of 10 below pscale
    inline double fround(double val,
                         double pscale)
    {
        double pscale10 = std::pow( 10., std::floor( std::log10(pscale) ) );

        return pscale10 * std::floor(val / pscale10 + 0.5);
    }
};

template <typename ParamName>
bool
PositionInteract<ParamName>::draw(const OFX::DrawArgs &args)
{
    if (_hasNativeHostPositionHandle) {
        return false;
    }
    if ( _position->getIsSecret() ||
         !_position->getIsEnable() ) {
        return false;
    }

    bool hiDPI = _hiDPI ? _hiDPI->getValue() : false;
    double screenPixelRatio = hiDPI ? 2 : 1;
#ifdef OFX_EXTENSIONS_NATRON
    screenPixelRatio *= args.screenPixelRatio;
    hiDPI |= args.screenPixelRatio > 1;
#endif
    TextRenderer::Font font = hiDPI ? TextRenderer::FONT_TIMES_ROMAN_24 : TextRenderer::FONT_HELVETICA_12;

    OfxRGBColourD color = { 0.8, 0.8, 0.8 };
    getSuggestedColour(color);
    //const OfxPointD& pscale = args.pixelScale;
    GLdouble projection[16];
    glGetDoublev( GL_PROJECTION_MATRIX, projection);
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    OfxPointD shadow; // how much to translate GL_PROJECTION to get exactly one pixel on screen
    shadow.x = 2. / (projection[0] * viewport[2]);
    shadow.y = 2. / (projection[5] * viewport[3]);

    OfxRGBColourF col;
    switch (_state) {
    case eMouseStateInactive:
        col.r = (float)color.r; col.g = (float)color.g; col.b = (float)color.b; break;
    case eMouseStatePoised:
        col.r = 0.f; col.g = 1.0f; col.b = 0.0f; break;
    case eMouseStatePicked:
        col.r = 0.f; col.g = 1.0f; col.b = 0.0f; break;
    }

    OfxPointD pos;
    if (_state == eMouseStatePicked) {
        pos = _penPosition;
    } else {
        _position->getValueAtTime(args.time, pos.x, pos.y);
    }
    //glPushAttrib(GL_ALL_ATTRIB_BITS); // caller is responsible for protecting attribs
    glPointSize( (float)pointSize() * screenPixelRatio);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_BLEND);
    glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
    glLineWidth(1.5f * screenPixelRatio);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Draw everything twice
    // l = 0: shadow
    // l = 1: drawing
    for (int l = 0; l < 2; ++l) {
        // shadow (uses GL_PROJECTION)
        glMatrixMode(GL_PROJECTION);
        int direction = (l == 0) ? 1 : -1;
        // translate (1,-1) pixels
        glTranslated(direction * shadow.x, -direction * shadow.y, 0);
        glMatrixMode(GL_MODELVIEW); // Modelview should be used on Nuke

        glEnable(GL_POINT_SMOOTH);
        const double darken = 0.5;
        glColor3f(color.r * l * darken, color.g * l * darken, color.b * l * darken);
        drawPointTrajectory(_position);

        glDisable(GL_POINT_SMOOTH);
        glColor3f(col.r * l, col.g * l, col.b * l);
        glBegin(GL_POINTS);
        glVertex2d(pos.x, pos.y);
        glEnd();
        OFX::TextRenderer::bitmapString( pos.x, pos.y + pointSize() * screenPixelRatio, ParamName::name(), font );
    }

    //glPopAttrib();

    return true;
} // draw

// overridden functions from OFX::Interact to do things
template <typename ParamName>
bool
PositionInteract<ParamName>::penMotion(const OFX::PenArgs &args)
{
    if (_hasNativeHostPositionHandle) {
        return false;
    }
    if ( _position->getIsSecret() ||
         !_position->getIsEnable() ) {
        return false;
    }

    const OfxPointD& pscale = args.pixelScale;
    OfxPointD pos;
    if (_state == eMouseStatePicked) {
        pos = _penPosition;
    } else {
        _position->getValueAtTime(args.time, pos.x, pos.y);
    }

    // pen position is in cannonical coords
    const OfxPointD &penPos = args.penPosition;
    bool redraw = false;
    bool valuesChanged = false;

    switch (_state) {
    case eMouseStateInactive:
    case eMouseStatePoised: {
        // are we in the box, become 'poised'
        MouseStateEnum newState;
        if ( ( std::fabs(penPos.x - pos.x) <= pointTolerance() * pscale.x) &&
             ( std::fabs(penPos.y - pos.y) <= pointTolerance() * pscale.y) ) {
            newState = eMouseStatePoised;
        } else {
            newState = eMouseStateInactive;
        }

        redraw = _state != newState;
        _state = newState;

        break;
    }

    case eMouseStatePicked: {
        _penPosition = args.penPosition;
        valuesChanged = true;
        break;
    }
    }

    if ( (_state != eMouseStateInactive) && _interactiveDrag && valuesChanged ) {
        _position->setValue( fround(_penPosition.x, pscale.x), fround(_penPosition.y, pscale.y) );
    }

    if (redraw || valuesChanged) {
        requestRedraw();
    }

    return valuesChanged;
} // >::penMotion

template <typename ParamName>
bool
PositionInteract<ParamName>::penDown(const OFX::PenArgs &args)
{
    if (_hasNativeHostPositionHandle) {
        return false;
    }
    if (!_position) {
        return false;
    }
    if ( _position->getIsSecret() ||
         !_position->getIsEnable() ) {
        return false;
    }

    bool didSomething = false;
    penMotion(args);
    if (_state == eMouseStatePoised) {
        _state = eMouseStatePicked;
        _penPosition = args.penPosition;
        if (_interactive) {
            _interactive->getValueAtTime(args.time, _interactiveDrag);
        }
        didSomething = true;
    }

    return didSomething;
}

template <typename ParamName>
bool
PositionInteract<ParamName>::penUp(const OFX::PenArgs &args)
{
    if (_hasNativeHostPositionHandle) {
        return false;
    }
    if (!_position) {
        return false;
    }
    if ( _position->getIsSecret() ||
         !_position->getIsEnable() ) {
        return false;
    }

    bool didSomething = false;
    if (_state == eMouseStatePicked) {
        if (!_interactiveDrag) {
            const OfxPointD& pscale = args.pixelScale;
            _position->setValue( fround(_penPosition.x, pscale.x), fround(_penPosition.y, pscale.y) );
        }
        penMotion(args);
        _state = eMouseStateInactive;
        didSomething = true;
    }

    if (didSomething) {
        requestRedraw();
    }

    return didSomething;
}

/** @brief Called when the interact is loses input focus */
template <typename ParamName>
void
PositionInteract<ParamName>::loseFocus(const OFX::FocusArgs & /*args*/)
{
    _interactiveDrag = false;
    if (_state != eMouseStateInactive) {
        _state = eMouseStateInactive;
        requestRedraw();
    }
}

template <typename ParamName>
class PositionOverlayDescriptor
    : public OFX::DefaultEffectOverlayDescriptor<PositionOverlayDescriptor<ParamName>, PositionInteract<ParamName> >
{
};
} // namespace OFX

#endif /* defined(openfx_supportext_ofxsPositionInteract_h) */
