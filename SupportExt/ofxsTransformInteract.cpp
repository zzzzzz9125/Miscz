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
  * OFX TransformInteract.
  */

#include "ofxsTransformInteract.h"
#include "ofxsOGLHiDPI.h"
#include "ofxsCoords.h"

#include <memory>
#include <cmath>
#include <cfloat> // DBL_MAX
#include <algorithm>

#ifdef __APPLE__
#ifndef GL_SILENCE_DEPRECATION
#define GL_SILENCE_DEPRECATION // Yes, we are still doing OpenGL 2.1
#endif
#include <OpenGL/gl.h>
#else
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include <GL/gl.h>
#endif

#ifndef M_PI
#define M_PI        3.14159265358979323846264338327950288   /* pi             */
#endif

#include "ofxsMatrix2D.h"
#include "ofxsTransform3x3.h"
#include "ofxsPositionInteract.h"

using namespace OFX;

#define SCALE_MAX 10000.

#define CIRCLE_RADIUS_BASE 30.
#define CIRCLE_RADIUS_MIN 15.
#define CIRCLE_RADIUS_MAX 300.
#define POINT_SIZE 7.
#define ELLIPSE_N_POINTS 50.

namespace OFX {
    /// add Transform params. page and group are optional
    void
        ofxsTransformDescribeParams(ImageEffectDescriptor& desc,
            PageParamDescriptor* page,
            GroupParamDescriptor* group,
            bool isOpen,
            bool oldParams,
            bool hasAmount,
            bool noTranslate)
    {
        // periodic
        {
            GroupParamDescriptor* group = desc.defineGroupParam(kGroupTransformPeriodic);
            if (group) {
                group->setLabel(kGroupTransformPeriodicLabel);
                group->setAsTab();
                if (page) {
                    page->addChild(*group);
                }
            }

            // periodicRadius
            {
                DoubleParamDescriptor* param = desc.defineDoubleParam(kParamTransformPeriodicRadius);
                param->setLabel(kParamTransformPeriodicRadiusLabel);
                param->setHint(kParamTransformPeriodicRadiusHint);
                param->setDefault(0);
                param->setRange(0, DBL_MAX);
                param->setDisplayRange(0, 1);
                param->setIncrement(0.05);
                if (group) {
                    param->setParent(*group);
                }
                if (page) {
                    page->addChild(*param);
                }
            }

            // periodicFrequency
            {
                DoubleParamDescriptor* param = desc.defineDoubleParam(kParamTransformPeriodicFrequency);
                param->setLabel(kParamTransformPeriodicFrequencyLabel);
                param->setHint(kParamTransformPeriodicFrequencyHint);
                param->setDefault(0);
                param->setRange(-DBL_MAX, DBL_MAX);
                param->setDisplayRange(-5, 5);
                param->setIncrement(0.1);
                if (group) {
                    param->setParent(*group);
                }
                if (page) {
                    page->addChild(*param);
                }
            }

            // periodicFrequencyUnit
            {
                ChoiceParamDescriptor* param = desc.defineChoiceParam(kParamTransformPeriodicFrequencyUnit);
                param->setLabel(kParamTransformPeriodicFrequencyUnitLabel);
                param->setHint(kParamTransformPeriodicFrequencyUnitHint);
                param->setDefault(0);
                param->appendOption("Hz");
                param->appendOption("BPM");
                param->setAnimates(false);
                if (group) {
                    param->setParent(*group);
                }
                if (page) {
                    page->addChild(*param);
                }
            }

            // periodicFrequencyBeat
            {
                ChoiceParamDescriptor* param = desc.defineChoiceParam(kParamTransformPeriodicFrequencyBeat);
                param->setLabel(kParamTransformPeriodicFrequencyBeatLabel);
                param->setHint(kParamTransformPeriodicFrequencyBeatHint);
                param->setDefault(4);
                param->appendOption("Quadruple Wholes");
                param->appendOption("Breve");
                param->appendOption("Whole");
                param->appendOption("Half");
                param->appendOption("Quarter");
                param->appendOption("Eighth");
                param->appendOption("Sixteenth");
                param->appendOption("Thirty-Second");
                param->appendOption("Half Triplet");
                param->appendOption("Quarter Triplet");
                param->appendOption("Eighth Triplet");
                param->appendOption("Sixteenth Triplet");
                param->appendOption("Thirty-Second Triplet");
                param->setAnimates(true);
                param->setIsSecret(true);
                if (group) {
                    param->setParent(*group);
                }
                if (page) {
                    page->addChild(*param);
                }
            }

            // periodicAutorotate
            {
                DoubleParamDescriptor* param = desc.defineDoubleParam(kParamTransformPeriodicAutorotate);
                param->setLabel(kParamTransformPeriodicAutorotateLabel);
                param->setHint(kParamTransformPeriodicAutorotateHint);
                param->setDefault(0);
                param->setRange(-DBL_MAX, DBL_MAX);
                param->setDisplayRange(-10, 10);
                param->setIncrement(1.);
                if (group) {
                    param->setParent(*group);
                }
                if (page) {
                    page->addChild(*param);
                }
            }

            // periodicScale
            {
                DoubleParamDescriptor* param = desc.defineDoubleParam(kParamTransformPeriodicScale);
                param->setLabel(kParamTransformPeriodicScaleLabel);
                param->setHint(kParamTransformPeriodicScaleHint);
                param->setDefault(1);
                param->setRange(-DBL_MAX, DBL_MAX);
                param->setDisplayRange(0, 5);
                param->setIncrement(0.05);
                if (group) {
                    param->setParent(*group);
                }
                if (page) {
                    page->addChild(*param);
                }
            }

            // periodicScaleStep
            {
                DoubleParamDescriptor* param = desc.defineDoubleParam(kParamTransformPeriodicScaleStep);
                param->setLabel(kParamTransformPeriodicScaleStepLabel);
                param->setHint(kParamTransformPeriodicScaleStepHint);
                param->setDefault(0);
                param->setRange(-INT_MAX, INT_MAX);
                param->setDisplayRange(0, 20);
                param->setIncrement(1);
                if (group) {
                    param->setParent(*group);
                }
                if (page) {
                    page->addChild(*param);
                }
            }

            // periodicOffset
            {
                DoubleParamDescriptor* param = desc.defineDoubleParam(kParamTransformPeriodicOffset);
                param->setLabel(kParamTransformPeriodicOffsetLabel);
                param->setHint(kParamTransformPeriodicOffsetHint);
                param->setDefault(0);
                param->setRange(-DBL_MAX, DBL_MAX);
                param->setDisplayRange(0, 1);
                param->setIncrement(0.05);
                if (group) {
                    param->setParent(*group);
                }
                if (page) {
                    page->addChild(*param);
                }
            }

            // periodicSkip
            {
                DoubleParamDescriptor* param = desc.defineDoubleParam(kParamTransformPeriodicSkip);
                param->setLabel(kParamTransformPeriodicSkipLabel);
                param->setHint(kParamTransformPeriodicSkipHint);
                param->setDefault(0);
                param->setRange(-DBL_MAX, DBL_MAX);
                param->setDisplayRange(0, 10);
                param->setIncrement(0.5);
                if (group) {
                    param->setParent(*group);
                }
                if (page) {
                    page->addChild(*param);
                }
            }

            // periodicN
            {
                IntParamDescriptor* param = desc.defineIntParam(kParamTransformPeriodicN);
                param->setLabel(kParamTransformPeriodicNLabel);
                param->setHint(kParamTransformPeriodicNHint);
                param->setDefault(1);
                param->setRange(1, INT_MAX);
                param->setDisplayRange(1, 20);
                if (group) {
                    param->setParent(*group);
                }
                if (page) {
                    page->addChild(*param);
                }
            }

            // periodicInterval
            {
                IntParamDescriptor* param = desc.defineIntParam(kParamTransformPeriodicInterval);
                param->setLabel(kParamTransformPeriodicIntervalLabel);
                param->setHint(kParamTransformPeriodicIntervalHint);
                param->setDefault(1);
                param->setRange(1, INT_MAX);
                param->setDisplayRange(1, 20);
                if (group) {
                    param->setParent(*group);
                }
                if (page) {
                    page->addChild(*param);
                }
            }

            // periodicBend
            {
                DoubleParamDescriptor* param = desc.defineDoubleParam(kParamTransformPeriodicBend);
                param->setLabel(kParamTransformPeriodicBendLabel);
                param->setHint(kParamTransformPeriodicBendHint);
                param->setDefault(0);
                param->setRange(-DBL_MAX, DBL_MAX);
                param->setDisplayRange(-1, 1);
                param->setIncrement(.1);
                if (group) {
                    param->setParent(*group);
                }
                if (page) {
                    page->addChild(*param);
                }
            }

            // periodicRotate
            {
                DoubleParamDescriptor* param = desc.defineDoubleParam(kParamTransformPeriodicRotate);
                param->setLabel(kParamTransformPeriodicRotateLabel);
                param->setHint(kParamTransformPeriodicRotateHint);
                param->setDoubleType(eDoubleTypeAngle);
                param->setDefault(0);
                param->setRange(-DBL_MAX, DBL_MAX);
                param->setDisplayRange(-1800, 1800);
                param->setIncrement(30);
                if (group) {
                    param->setParent(*group);
                }
                if (page) {
                    page->addChild(*param);
                }
            }

            // periodicDeform
            {
                DoubleParamDescriptor* param = desc.defineDoubleParam(kParamTransformPeriodicDeform);
                param->setLabel(kParamTransformPeriodicDeformLabel);
                param->setHint(kParamTransformPeriodicDeformHint);
                param->setDefault(1);
                param->setRange(-DBL_MAX, DBL_MAX);
                param->setDisplayRange(0, 10);
                param->setIncrement(.1);
                if (group) {
                    param->setParent(*group);
                }
                if (page) {
                    page->addChild(*param);
                }
            }

            // periodicCurve
            {
                ChoiceParamDescriptor* param = desc.defineChoiceParam(kParamTransformPeriodicCurve);
                param->setLabel(kParamTransformPeriodicCurveLabel);
                param->setHint(kParamTransformPeriodicCurveHint);
                param->setDefault(1);
                param->appendOption("Custom");
                param->appendOption("Default");
                param->appendOption("Ease");
                param->appendOption("Ease In");
                param->appendOption("Ease Out");
                param->appendOption("Quad");
                param->appendOption("Quad In");
                param->appendOption("Quad Out");
                param->appendOption("Cubic");
                param->appendOption("Cubic In");
                param->appendOption("Cubic Out");
                param->appendOption("Quart");
                param->appendOption("Quart In");
                param->appendOption("Quart Out");
                param->appendOption("Quint");
                param->appendOption("Quint In");
                param->appendOption("Quint Out");
                param->appendOption("Expo");
                param->appendOption("Expo In");
                param->appendOption("Expo Out");
                param->appendOption("Circ");
                param->appendOption("Circ In");
                param->appendOption("Circ Out");
                param->appendOption("Back");
                param->appendOption("Back In");
                param->appendOption("Back Out");
                param->appendOption("Linear");
                param->setAnimates(true);
                if (group) {
                    param->setParent(*group);
                }
                if (page) {
                    page->addChild(*param);
                }
            }

            // periodicBezierP1
            {
                Double2DParamDescriptor* param = desc.defineDouble2DParam(kParamTransformPeriodicBezierP1);
                param->setLabel(kParamTransformPeriodicBezierP1Label);
                param->setHint(kParamTransformPeriodicBezierPHint);
                param->setDefault(0, 0);
                param->setRange(0, -DBL_MAX, 1, DBL_MAX);
                param->setDisplayRange(0, 0, 1, 1);
                param->setIncrement(.1);
                param->setIsSecret(true);
                if (group) {
                    param->setParent(*group);
                }
                if (page) {
                    page->addChild(*param);
                }
            }

            // periodicBezierP2
            {
                Double2DParamDescriptor* param = desc.defineDouble2DParam(kParamTransformPeriodicBezierP2);
                param->setLabel(kParamTransformPeriodicBezierP2Label);
                param->setHint(kParamTransformPeriodicBezierPHint);
                param->setDefault(1, 1);
                param->setRange(0, -DBL_MAX, 1, DBL_MAX);
                param->setDisplayRange(0, 0, 1, 1);
                param->setIncrement(.1);
                param->setIsSecret(true);
                if (group) {
                    param->setParent(*group);
                }
                if (page) {
                    page->addChild(*param);
                }
            }

            // periodicSymmetry
            {
                BooleanParamDescriptor* param = desc.defineBooleanParam(kParamTransformPeriodicSymmetry);
                param->setLabel(kParamTransformPeriodicSymmetryLabel);
                param->setHint(kParamTransformPeriodicSymmetryHint);
                param->setDefault(false);
                param->setAnimates(true);
                if (group) {
                    param->setParent(*group);
                }
                if (page) {
                    page->addChild(*param);
                }
            }
        }

        // translate
        if (!noTranslate) {
            Double2DParamDescriptor* param = desc.defineDouble2DParam(oldParams ? kParamTransformTranslateOld : kParamTransformTranslate);
            param->setLabel(kParamTransformTranslateLabel);
            param->setHint(kParamTransformTranslateHint);
            param->setDoubleType(eDoubleTypeXYAbsolute);
            param->setDefaultCoordinateSystem(eCoordinatesNormalised);
            param->setDefault(0., 0.);
            param->setRange(-DBL_MAX, -DBL_MAX, DBL_MAX, DBL_MAX);
            param->setDisplayRange(-0.5, -0.5, 0.5, 0.5); // Resolve requires display range or values are clamped to (-1,1)
            param->setIncrement(0.05);
            if (group) {
                param->setParent(*group);
            }
            if (page) {
                page->addChild(*param);
            }
        }

        // rotate
        {
            DoubleParamDescriptor* param = desc.defineDoubleParam(oldParams ? kParamTransformRotateOld : kParamTransformRotate);
            param->setLabel(kParamTransformRotateLabel);
            param->setHint(kParamTransformRotateHint);
            param->setDoubleType(eDoubleTypeAngle);
            param->setDefault(0);
            param->setRange(-DBL_MAX, DBL_MAX);
            param->setDisplayRange(-1800, 1800);
            param->setIncrement(30);
            if (group) {
                param->setParent(*group);
            }
            if (page) {
                page->addChild(*param);
            }
        }

        // faceToCenter
        {
            BooleanParamDescriptor* param = desc.defineBooleanParam(kParamTransformFaceToCenter);
            param->setLabel(kParamTransformFaceToCenterLabel);
            param->setHint(kParamTransformFaceToCenterHint);
            param->setDefault(false);
            param->setAnimates(true);
            if (group) {
                param->setParent(*group);
            }
            if (page) {
                page->addChild(*param);
            }
        }

        // scale
        {
            Double2DParamDescriptor* param = desc.defineDouble2DParam(oldParams ? kParamTransformScaleOld : kParamTransformScale);
            param->setLabel(kParamTransformScaleLabel);
            param->setHint(kParamTransformScaleHint);
            param->setDoubleType(eDoubleTypeScale);
            //param->setDimensionLabels("w","h");
            param->setDefault(1, 1);
            param->setRange(-SCALE_MAX, -SCALE_MAX, SCALE_MAX, SCALE_MAX);
            param->setDisplayRange(0, 0, 10, 10);
            param->setIncrement(0.01);
            param->setLayoutHint(eLayoutHintNoNewLine, 1);
            if (group) {
                param->setParent(*group);
            }
            if (page) {
                page->addChild(*param);
            }
        }

        // flop
        {
            BooleanParamDescriptor* param = desc.defineBooleanParam(kParamTransformFlop);
            param->setLabel(kParamTransformFlopLabel);
            param->setHint(kParamTransformFlopHint);
            param->setDefault(false);
            param->setAnimates(true);
            if (group) {
                param->setParent(*group);
            }
            if (page) {
                page->addChild(*param);
            }
        }

        // flip
        {
            BooleanParamDescriptor* param = desc.defineBooleanParam(kParamTransformFlip);
            param->setLabel(kParamTransformFlipLabel);
            param->setHint(kParamTransformFlipHint);
            param->setDefault(false);
            param->setAnimates(true);
            if (group) {
                param->setParent(*group);
            }
            if (page) {
                page->addChild(*param);
            }
        }

        // scaleUniform
        {
            BooleanParamDescriptor* param = desc.defineBooleanParam(oldParams ? kParamTransformScaleUniformOld : kParamTransformScaleUniform);
            param->setLabel(kParamTransformScaleUniformLabel);
            param->setHint(kParamTransformScaleUniformHint);
            param->setDefault(true);
            param->setAnimates(true);
            if (group) {
                param->setParent(*group);
            }
            if (page) {
                page->addChild(*param);
            }
        }

        // skewX
        {
            DoubleParamDescriptor* param = desc.defineDoubleParam(oldParams ? kParamTransformSkewXOld : kParamTransformSkewX);
            param->setLabel(kParamTransformSkewXLabel);
            param->setHint(kParamTransformSkewXHint);
            param->setDefault(0);
            param->setRange(-DBL_MAX, DBL_MAX);
            param->setDisplayRange(-1., 1.);
            param->setIncrement(0.01);
            if (group) {
                param->setParent(*group);
            }
            if (page) {
                page->addChild(*param);
            }
        }

        // skewY
        {
            DoubleParamDescriptor* param = desc.defineDoubleParam(oldParams ? kParamTransformSkewYOld : kParamTransformSkewY);
            param->setLabel(kParamTransformSkewYLabel);
            param->setHint(kParamTransformSkewYHint);
            param->setDefault(0);
            param->setRange(-DBL_MAX, DBL_MAX);
            param->setDisplayRange(-1., 1.);
            param->setIncrement(0.01);
            if (group) {
                param->setParent(*group);
            }
            if (page) {
                page->addChild(*param);
            }
        }

        // skewOrder
        {
            ChoiceParamDescriptor* param = desc.defineChoiceParam(oldParams ? kParamTransformSkewOrderOld : kParamTransformSkewOrder);
            param->setLabel(kParamTransformSkewOrderLabel);
            param->setHint(kParamTransformSkewOrderHint);
            param->setDefault(0);
            param->appendOption("XY");
            param->appendOption("YX");
            param->setAnimates(true);
            if (group) {
                param->setParent(*group);
            }
            if (page) {
                page->addChild(*param);
            }
        }

        // amount
        if (hasAmount) {
            DoubleParamDescriptor* param = desc.defineDoubleParam(kParamTransformAmount);
            param->setLabel(kParamTransformAmountLabel);
            param->setHint(kParamTransformAmountHint);
            param->setDoubleType(eDoubleTypeScale);
            param->setDefault(1.);
            param->setRange(-DBL_MAX, DBL_MAX);
            param->setDisplayRange(0., 1.);
            param->setIncrement(0.01);
            if (group) {
                param->setParent(*group);
            }
            if (page) {
                page->addChild(*param);
            }
        }

        // center
        {
            Double2DParamDescriptor* param = desc.defineDouble2DParam(oldParams ? kParamTransformCenterOld : kParamTransformCenter);
            param->setLabel(kParamTransformCenterLabel);
            param->setHint(kParamTransformCenterHint);
            //param->setDoubleType(eDoubleTypeNormalisedXY); // deprecated in OpenFX 1.2
            //param->setDimensionLabels("x","y");
            param->setDoubleType(eDoubleTypeXYAbsolute);
            param->setDefault(.5, .5);
            param->setDefaultCoordinateSystem(eCoordinatesNormalised);
            param->setRange(-DBL_MAX, -DBL_MAX, DBL_MAX, DBL_MAX);
            param->setDisplayRange(0, 0, 1, 1); // Resolve requires display range or values are clamped to (-1,1)
            param->setIncrement(.1);
            param->setLayoutHint(eLayoutHintNoNewLine, 1);
            if (group) {
                param->setParent(*group);
            }
            if (page) {
                page->addChild(*param);
            }
        }

        // resetCenter
        {
            PushButtonParamDescriptor* param = desc.definePushButtonParam(oldParams ? kParamTransformResetCenterOld : kParamTransformResetCenter);
            param->setLabel(kParamTransformResetCenterLabel);
            param->setHint(kParamTransformResetCenterHint);
            param->setIsSecret(true);
            if (group) {
                param->setParent(*group);
            }
            if (page) {
                page->addChild(*param);
            }
        }

        // centerChanged
        {
            BooleanParamDescriptor* param = desc.defineBooleanParam(kParamTransformCenterChanged);
            param->setDefault(false);
            param->setIsSecretAndDisabled(true);
            param->setAnimates(false);
            param->setEvaluateOnChange(false);
            if (group) {
                param->setParent(*group);
            }
            if (page) {
                page->addChild(*param);
            }
        }

        // interactOpen
        {
            BooleanParamDescriptor* param = desc.defineBooleanParam(kParamTransformInteractOpen);
            param->setLabel(kParamTransformInteractOpenLabel);
            param->setHint(kParamTransformInteractOpenHint);
            param->setDefault(isOpen); // open by default
            param->setIsSecretAndDisabled(true); // secret by default, but this can be changed for specific hosts
            param->setAnimates(false);
            if (group) {
                param->setParent(*group);
            }
            if (page) {
                page->addChild(*param);
            }
        }

        // interactive
        {
            BooleanParamDescriptor* param = desc.defineBooleanParam(oldParams ? kParamTransformInteractiveOld : kParamTransformInteractive);
            param->setLabel(kParamTransformInteractiveLabel);
            param->setHint(kParamTransformInteractiveHint);
            param->setDefault(true);
            param->setIsSecretAndDisabled(true);
            param->setEvaluateOnChange(false);
            if (group) {
                param->setParent(*group);
            }
            if (page) {
                page->addChild(*param);
            }
        }

        // HiDPI
        hiDPIDescribeParams(desc, group, page);
    } // ofxsTransformDescribeParams

    ////////////////////////////////////////////////////////////////////////////////
    // stuff for the interact

    TransformInteractHelper::TransformInteractHelper(ImageEffect* effect,
        Interact* interact,
        bool oldParams)
        : _drawState(eInActive)
        , _mouseState(eReleased)
        , _modifierStateCtrl(0)
        , _modifierStateShift(0)
        , _orientation(eOrientationAllDirections)
        , _effect(effect)
        , _interact(interact)
        , _lastMousePos()
        , _tpDrag()
        , _interactiveDrag(false)
        , _translate(NULL)
        , _rotate(NULL)
        , _faceToCenter(NULL)
        , _periodicRadius(NULL)
        , _periodicRotate(NULL)
        , _periodicDeform(NULL)
        , _periodicBend(NULL)
        , _periodicN(NULL)
        , _periodicInterval(NULL)
        , _periodicBezierP1(NULL)
        , _periodicBezierP2(NULL)
        , _periodicSymmetry(NULL)
        , _periodicFrequency(NULL)
        , _periodicAutorotate(NULL)
        , _periodicScale(NULL)
        , _periodicScaleStep(NULL)
        , _periodicOffset(NULL)
        , _periodicSkip(NULL)
        , _scale(NULL)
        , _flop(NULL)
        , _flip(NULL)
        , _scaleUniform(NULL)
        , _skewX(NULL)
        , _skewY(NULL)
        , _skewOrder(NULL)
        , _center(NULL)
        , _invert(NULL)
        , _interactOpen(NULL)
        , _interactive(NULL)
        , _hiDPI(NULL)
    {

        assert(_effect && _interact);
        _lastMousePos.x = _lastMousePos.y = 0.;
        // NON-GENERIC
        if (oldParams) {
            if (_effect->paramExists(kParamTransformTranslateOld)) {
                _translate = _effect->fetchDouble2DParam(kParamTransformTranslateOld);
                assert(_translate);
            }
            _rotate = _effect->fetchDoubleParam(kParamTransformRotateOld);
            _scale = _effect->fetchDouble2DParam(kParamTransformScaleOld);
            _scaleUniform = _effect->fetchBooleanParam(kParamTransformScaleUniformOld);
            _skewX = _effect->fetchDoubleParam(kParamTransformSkewXOld);
            _skewY = _effect->fetchDoubleParam(kParamTransformSkewYOld);
            _skewOrder = _effect->fetchChoiceParam(kParamTransformSkewOrderOld);
            _center = _effect->fetchDouble2DParam(kParamTransformCenterOld);
            _interactive = _effect->fetchBooleanParam(kParamTransformInteractiveOld);
        }
        else {
            if (_effect->paramExists(kParamTransformTranslate)) {
                _translate = _effect->fetchDouble2DParam(kParamTransformTranslate);
                assert(_translate);
            }
            _rotate = _effect->fetchDoubleParam(kParamTransformRotate);
            _scale = _effect->fetchDouble2DParam(kParamTransformScale);
            _scaleUniform = _effect->fetchBooleanParam(kParamTransformScaleUniform);
            _skewX = _effect->fetchDoubleParam(kParamTransformSkewX);
            _skewY = _effect->fetchDoubleParam(kParamTransformSkewY);
            _skewOrder = _effect->fetchChoiceParam(kParamTransformSkewOrder);
            _center = _effect->fetchDouble2DParam(kParamTransformCenter);
            _interactive = _effect->fetchBooleanParam(kParamTransformInteractive);
        }
        _periodicRadius = _effect->fetchDoubleParam(kParamTransformPeriodicRadius);
        _periodicRotate = _effect->fetchDoubleParam(kParamTransformPeriodicRotate);
        _periodicDeform = _effect->fetchDoubleParam(kParamTransformPeriodicDeform);
        _periodicBend = _effect->fetchDoubleParam(kParamTransformPeriodicBend);
        _periodicN = _effect->fetchIntParam(kParamTransformPeriodicN);
        _periodicInterval = _effect->fetchIntParam(kParamTransformPeriodicInterval);
        _periodicBezierP1 = _effect->fetchDouble2DParam(kParamTransformPeriodicBezierP1);
        _periodicBezierP2 = _effect->fetchDouble2DParam(kParamTransformPeriodicBezierP2);
        _periodicSymmetry = _effect->fetchBooleanParam(kParamTransformPeriodicSymmetry);
        _periodicFrequency = _effect->fetchDoubleParam(kParamTransformPeriodicFrequency);
        _periodicAutorotate = _effect->fetchDoubleParam(kParamTransformPeriodicAutorotate);
        _periodicScale = _effect->fetchDoubleParam(kParamTransformPeriodicScale);
        _periodicScaleStep = _effect->fetchDoubleParam(kParamTransformPeriodicScaleStep);
        _periodicOffset = _effect->fetchDoubleParam(kParamTransformPeriodicOffset);
        _periodicSkip = _effect->fetchDoubleParam(kParamTransformPeriodicSkip);
        _faceToCenter = _effect->fetchBooleanParam(kParamTransformFaceToCenter);
        _flop = _effect->fetchBooleanParam(kParamTransformFlop);
        _flip = _effect->fetchBooleanParam(kParamTransformFlip);
        _interactOpen = _effect->fetchBooleanParam(kParamTransformInteractOpen);
        if (_effect->paramExists(kParamTransform3x3Invert)) {
            _invert = _effect->fetchBooleanParam(kParamTransform3x3Invert);
        }
        if (effect->paramExists(kParamHiDPI)) {
            _hiDPI = effect->fetchBooleanParam(kParamHiDPI);
        }
        assert(_rotate && _scaleUniform && _faceToCenter && _periodicRotate && _periodicDeform && _periodicBend && _periodicN && _periodicInterval && _periodicBezierP1 && _periodicBezierP2 && _periodicSymmetry && _periodicFrequency && _periodicAutorotate && _periodicScale && _periodicScaleStep && _periodicOffset && _periodicSkip && _scale && _scaleUniform && _flop && _flip && _skewX && _skewY && _skewOrder && _center && _interactive);
        if (_translate) {
            _interact->addParamToSlaveTo(_translate);
        }
        if (_rotate) {
            _interact->addParamToSlaveTo(_rotate);
        }
        if (_faceToCenter) {
            _interact->addParamToSlaveTo(_faceToCenter);
        }
        if (_periodicRadius) {
            _interact->addParamToSlaveTo(_periodicRadius);
        }
        if (_periodicRotate) {
            _interact->addParamToSlaveTo(_periodicRotate);
        }
        if (_periodicDeform) {
            _interact->addParamToSlaveTo(_periodicDeform);
        }
        if (_periodicBend) {
            _interact->addParamToSlaveTo(_periodicBend);
        }
        if (_periodicN) {
            _interact->addParamToSlaveTo(_periodicN);
        }
        if (_periodicInterval) {
            _interact->addParamToSlaveTo(_periodicInterval);
        }
        if (_periodicBezierP1) {
            _interact->addParamToSlaveTo(_periodicBezierP1);
        }
        if (_periodicBezierP2) {
            _interact->addParamToSlaveTo(_periodicBezierP2);
        }
        if (_periodicSymmetry) {
            _interact->addParamToSlaveTo(_periodicSymmetry);
        }
        if (_periodicFrequency) {
            _interact->addParamToSlaveTo(_periodicFrequency);
        }
        if (_periodicAutorotate) {
            _interact->addParamToSlaveTo(_periodicAutorotate);
        }
        if (_periodicScale) {
            _interact->addParamToSlaveTo(_periodicScale);
        }
        if (_periodicScaleStep) {
            _interact->addParamToSlaveTo(_periodicScaleStep);
        }
        if (_periodicOffset) {
            _interact->addParamToSlaveTo(_periodicOffset);
        }
        if (_periodicSkip) {
            _interact->addParamToSlaveTo(_periodicSkip);
        }
        if (_scale) {
            _interact->addParamToSlaveTo(_scale);
        }
        if (_flop) {
            _interact->addParamToSlaveTo(_flop);
        }
        if (_flip) {
            _interact->addParamToSlaveTo(_flip);
        }
        if (_skewX) {
            _interact->addParamToSlaveTo(_skewX);
        }
        if (_skewY) {
            _interact->addParamToSlaveTo(_skewY);
        }
        if (_skewOrder) {
            _interact->addParamToSlaveTo(_skewOrder);
        }
        if (_center) {
            _interact->addParamToSlaveTo(_center);
        }
        if (_invert) {
            _interact->addParamToSlaveTo(_invert);
        }
        if (!_translate) {
            _modifierStateCtrl = 1;
        }
    }

    void
        TransformInteractHelper::getTransformParams(double time, TransformParams& tp)
    {
        tp = TransformParams();
        if (_center) {
            _center->getValueAtTime(time, tp.center.x, tp.center.y);
        }
        if (_translate) {
            _translate->getValueAtTime(time, tp.translate.x, tp.translate.y);
        }
        if (_periodicRadius) {
            _periodicRadius->getValueAtTime(time, tp.periodicRadius);
        }
        if (_periodicRotate) {
            _periodicRotate->getValueAtTime(time, tp.periodicRotate);
        }
        if (_periodicDeform) {
            _periodicDeform->getValueAtTime(time, tp.periodicDeform);
        }
        if (_periodicBend) {
            _periodicBend->getValueAtTime(time, tp.periodicBend);
        }
        if (_periodicN) {
            _periodicN->getValueAtTime(time, tp.periodicN);
        }
        if (_periodicInterval) {
            _periodicInterval->getValueAtTime(time, tp.periodicInterval);
        }
        if (_periodicBezierP1) {
            _periodicBezierP1->getValueAtTime(time, tp.periodicBezierP1.x, tp.periodicBezierP1.y);
        }
        if (_periodicBezierP2) {
            _periodicBezierP2->getValueAtTime(time, tp.periodicBezierP2.x, tp.periodicBezierP2.y);
        }
        if (_periodicSymmetry) {
            _periodicSymmetry->getValueAtTime(time, tp.periodicSymmetry);
        }
        if (_periodicFrequency) {
            _periodicFrequency->getValueAtTime(time, tp.periodicFrequency);
        }
        if (_periodicAutorotate) {
            _periodicAutorotate->getValueAtTime(time, tp.periodicAutorotate);
        }
        if (_periodicScale) {
            _periodicScale->getValueAtTime(time, tp.periodicScale);
        }
        if (_periodicScaleStep) {
            _periodicScaleStep->getValueAtTime(time, tp.periodicScaleStep);
        }
        if (_periodicOffset) {
            _periodicOffset->getValueAtTime(time, tp.periodicOffset);
        }
        if (_periodicSkip) {
            _periodicSkip->getValueAtTime(time, tp.periodicSkip);
        }
        if (_scale) {
            _scale->getValueAtTime(time, tp.scale.x, tp.scale.y);
        }
        if (_scaleUniform) {
            _scaleUniform->getValueAtTime(time, tp.scaleUniform);
        }
        if (_flop) {
            _flop->getValueAtTime(time, tp.flop);
        }
        if (_flip) {
            _flip->getValueAtTime(time, tp.flip);
        }
        if (_rotate) {
            _rotate->getValueAtTime(time, tp.rotate);
        }
        if (_faceToCenter) {
            _faceToCenter->getValueAtTime(time, tp.faceToCenter);
        }
        if (_skewX) {
            _skewX->getValueAtTime(time, tp.skewX);
        }
        if (_skewY) {
            _skewY->getValueAtTime(time, tp.skewY);
        }
        if (_skewOrder) {
            _skewOrder->getValueAtTime(time, tp.skewOrder);
        }
        if (_invert) {
            _invert->getValueAtTime(time, tp.inverted);
        }
    }

    static void
        getTargetCenter(const OfxPointD& center,
            const OfxPointD& translate,
            OfxPointD* targetCenter)
    {
        targetCenter->x = center.x + translate.x;
        targetCenter->y = center.y + translate.y;
    }

    static void
        getTargetRadius(const OfxPointD& scale,
            const OfxPointD& pixelScale,
            OfxPointD* targetRadius)
    {
        targetRadius->x = scale.x * CIRCLE_RADIUS_BASE;
        targetRadius->y = scale.y * CIRCLE_RADIUS_BASE;
        // don't draw too small. 15 pixels is the limit
        if ((std::fabs(targetRadius->x) < CIRCLE_RADIUS_MIN) && (std::fabs(targetRadius->y) < CIRCLE_RADIUS_MIN)) {
            targetRadius->x = targetRadius->x >= 0 ? CIRCLE_RADIUS_MIN : -CIRCLE_RADIUS_MIN;
            targetRadius->y = targetRadius->y >= 0 ? CIRCLE_RADIUS_MIN : -CIRCLE_RADIUS_MIN;
        }
        else if ((std::fabs(targetRadius->x) > CIRCLE_RADIUS_MAX) && (std::fabs(targetRadius->y) > CIRCLE_RADIUS_MAX)) {
            targetRadius->x = targetRadius->x >= 0 ? CIRCLE_RADIUS_MAX : -CIRCLE_RADIUS_MAX;
            targetRadius->y = targetRadius->y >= 0 ? CIRCLE_RADIUS_MAX : -CIRCLE_RADIUS_MAX;
        }
        else {
            if (std::fabs(targetRadius->x) < CIRCLE_RADIUS_MIN) {
                if ((targetRadius->x == 0.) && (targetRadius->y != 0.)) {
                    targetRadius->y = targetRadius->y > 0 ? CIRCLE_RADIUS_MAX : -CIRCLE_RADIUS_MAX;
                }
                else {
                    targetRadius->y *= std::fabs(CIRCLE_RADIUS_MIN / targetRadius->x);
                }
                targetRadius->x = targetRadius->x >= 0 ? CIRCLE_RADIUS_MIN : -CIRCLE_RADIUS_MIN;
            }
            if (std::fabs(targetRadius->x) > CIRCLE_RADIUS_MAX) {
                targetRadius->y *= std::fabs(CIRCLE_RADIUS_MAX / targetRadius->x);
                targetRadius->x = targetRadius->x > 0 ? CIRCLE_RADIUS_MAX : -CIRCLE_RADIUS_MAX;
            }
            if (std::fabs(targetRadius->y) < CIRCLE_RADIUS_MIN) {
                if ((targetRadius->y == 0.) && (targetRadius->x != 0.)) {
                    targetRadius->x = targetRadius->x > 0 ? CIRCLE_RADIUS_MAX : -CIRCLE_RADIUS_MAX;
                }
                else {
                    targetRadius->x *= std::fabs(CIRCLE_RADIUS_MIN / targetRadius->y);
                }
                targetRadius->y = targetRadius->y >= 0 ? CIRCLE_RADIUS_MIN : -CIRCLE_RADIUS_MIN;
            }
            if (std::fabs(targetRadius->y) > CIRCLE_RADIUS_MAX) {
                targetRadius->x *= std::fabs(CIRCLE_RADIUS_MAX / targetRadius->x);
                targetRadius->y = targetRadius->y > 0 ? CIRCLE_RADIUS_MAX : -CIRCLE_RADIUS_MAX;
            }
        }
        // the circle axes are not aligned with the images axes, so we cannot use the x and y scales separately
        double meanPixelScale = (pixelScale.x + pixelScale.y) / 2.;
        targetRadius->x *= meanPixelScale;
        targetRadius->y *= meanPixelScale;
    }

    static void
        getTargetPoints(const OfxPointD& targetCenter,
            const OfxPointD& targetRadius,
            OfxPointD* left,
            OfxPointD* bottom,
            OfxPointD* top,
            OfxPointD* right)
    {
        left->x = targetCenter.x - targetRadius.x;
        left->y = targetCenter.y;
        right->x = targetCenter.x + targetRadius.x;
        right->y = targetCenter.y;
        top->x = targetCenter.x;
        top->y = targetCenter.y + targetRadius.y;
        bottom->x = targetCenter.x;
        bottom->y = targetCenter.y - targetRadius.y;
    }

    static void
        drawSquare(const OfxRGBColourD& color,
            const OfxPointD& center,
            const OfxPointD& pixelScale,
            bool hovered,
            bool althovered,
            int l)
    {
        // we are not axis-aligned
        double meanPixelScale = (pixelScale.x + pixelScale.y) / 2.;

        if (hovered) {
            if (althovered) {
                glColor3f(0.f * l, 1.f * l, 0.f * l);
            }
            else {
                glColor3f(1.f * l, 0.f * l, 0.f * l);
            }
        }
        else {
            glColor3f((float)color.r * l, (float)color.g * l, (float)color.b * l);
        }
        double halfWidth = (POINT_SIZE / 2.) * meanPixelScale;
        double halfHeight = (POINT_SIZE / 2.) * meanPixelScale;
        glPushMatrix();
        glTranslated(center.x, center.y, 0.);
        glBegin(GL_POLYGON);
        glVertex2d(-halfWidth, -halfHeight);   // bottom left
        glVertex2d(-halfWidth, +halfHeight);   // top left
        glVertex2d(+halfWidth, +halfHeight);   // bottom right
        glVertex2d(+halfWidth, -halfHeight);   // top right
        glEnd();
        glPopMatrix();
    }

    static void
        drawEllipse(const OfxRGBColourD& color,
            const OfxPointD& center,
            const OfxPointD& targetRadius,
            bool hovered,
            int l)
    {
        if (hovered) {
            glColor3f(1.f * l, 0.f * l, 0.f * l);
        }
        else {
            glColor3f((float)color.r * l, (float)color.g * l, (float)color.b * l);
        }

        glPushMatrix();
        //  center the oval at x_center, y_center
        glTranslatef((float)center.x, (float)center.y, 0.f);
        //  draw the oval using line segments
        glBegin(GL_LINE_LOOP);
        // we don't need to be pixel-perfect here, it's just an interact!
        // 40 segments is enough.
        for (int i = 0; i < 40; ++i) {
            double theta = i * 2 * ofxsPi() / 40.;
            glVertex2d(targetRadius.x * std::cos(theta), targetRadius.y * std::sin(theta));
        }
        glEnd();

        glPopMatrix();
    }

    static void
        drawSkewBar(const OfxRGBColourD& color,
            const OfxPointD& center,
            const OfxPointD& pixelScale,
            double targetRadiusY,
            bool hovered,
            double angle,
            int l)
    {
        if (hovered) {
            glColor3f(1.f * l, 0.f * l, 0.f * l);
        }
        else {
            glColor3f((float)color.r * l, (float)color.g * l, (float)color.b * l);
        }

        // we are not axis-aligned: use the mean pixel scale
        double meanPixelScale = (pixelScale.x + pixelScale.y) / 2.;
        double barHalfSize = targetRadiusY + 20. * meanPixelScale;

        glPushMatrix();
        glTranslatef((float)center.x, (float)center.y, 0.f);
        glRotated(angle, 0, 0, 1);

        glBegin(GL_LINES);
        glVertex2d(0., -barHalfSize);
        glVertex2d(0., +barHalfSize);

        if (hovered) {
            double arrowYPosition = targetRadiusY + 10. * meanPixelScale;
            double arrowXHalfSize = 10 * meanPixelScale;
            double arrowHeadOffsetX = 3 * meanPixelScale;
            double arrowHeadOffsetY = 3 * meanPixelScale;

            ///draw the central bar
            glVertex2d(-arrowXHalfSize, -arrowYPosition);
            glVertex2d(+arrowXHalfSize, -arrowYPosition);

            ///left triangle
            glVertex2d(-arrowXHalfSize, -arrowYPosition);
            glVertex2d(-arrowXHalfSize + arrowHeadOffsetX, -arrowYPosition + arrowHeadOffsetY);

            glVertex2d(-arrowXHalfSize, -arrowYPosition);
            glVertex2d(-arrowXHalfSize + arrowHeadOffsetX, -arrowYPosition - arrowHeadOffsetY);

            ///right triangle
            glVertex2d(+arrowXHalfSize, -arrowYPosition);
            glVertex2d(+arrowXHalfSize - arrowHeadOffsetX, -arrowYPosition + arrowHeadOffsetY);

            glVertex2d(+arrowXHalfSize, -arrowYPosition);
            glVertex2d(+arrowXHalfSize - arrowHeadOffsetX, -arrowYPosition - arrowHeadOffsetY);
        }
        glEnd();
        glPopMatrix();
    }

    static void
        drawRotationBar(const OfxRGBColourD& color,
            const OfxPointD& pixelScale,
            double targetRadiusX,
            bool hovered,
            bool inverted,
            int l)
    {
        // we are not axis-aligned
        double meanPixelScale = (pixelScale.x + pixelScale.y) / 2.;

        if (hovered) {
            glColor3f(1.f * l, 0.f * l, 0.f * l);
        }
        else {
            glColor3d(color.r * l, color.g * l, color.b * l);
        }

        double barExtra = 30. * meanPixelScale;
        glBegin(GL_LINES);
        glVertex2d(0., 0.);
        glVertex2d(0. + targetRadiusX + barExtra, 0.);
        glEnd();

        if (hovered) {
            double arrowCenterX = targetRadiusX + barExtra / 2.;

            ///draw an arrow slightly bended. This is an arc of circle of radius 5 in X, and 10 in Y.
            OfxPointD arrowRadius;
            arrowRadius.x = 5. * meanPixelScale;
            arrowRadius.y = 10. * meanPixelScale;

            glPushMatrix();
            //  center the oval at x_center, y_center
            glTranslatef((float)arrowCenterX, 0.f, 0);
            //  draw the oval using line segments
            glBegin(GL_LINE_STRIP);
            glVertex2d(0, arrowRadius.y);
            glVertex2d(arrowRadius.x, 0.);
            glVertex2d(0, -arrowRadius.y);
            glEnd();


            glBegin(GL_LINES);
            ///draw the top head
            glVertex2d(0., arrowRadius.y);
            glVertex2d(0., arrowRadius.y - 5. * meanPixelScale);

            glVertex2d(0., arrowRadius.y);
            glVertex2d(4. * meanPixelScale, arrowRadius.y - 3. * meanPixelScale); // 5^2 = 3^2+4^2

            ///draw the bottom head
            glVertex2d(0., -arrowRadius.y);
            glVertex2d(0., -arrowRadius.y + 5. * meanPixelScale);

            glVertex2d(0., -arrowRadius.y);
            glVertex2d(4. * meanPixelScale, -arrowRadius.y + 3. * meanPixelScale); // 5^2 = 3^2+4^2

            glEnd();

            glPopMatrix();
        }
        if (inverted) {
            double arrowXPosition = targetRadiusX + barExtra * 1.5;
            double arrowXHalfSize = 10 * meanPixelScale;
            double arrowHeadOffsetX = 3 * meanPixelScale;
            double arrowHeadOffsetY = 3 * meanPixelScale;

            glPushMatrix();
            glTranslatef((float)arrowXPosition, 0, 0);

            glBegin(GL_LINES);
            ///draw the central bar
            glVertex2d(-arrowXHalfSize, 0.);
            glVertex2d(+arrowXHalfSize, 0.);

            ///left triangle
            glVertex2d(-arrowXHalfSize, 0.);
            glVertex2d(-arrowXHalfSize + arrowHeadOffsetX, arrowHeadOffsetY);

            glVertex2d(-arrowXHalfSize, 0.);
            glVertex2d(-arrowXHalfSize + arrowHeadOffsetX, -arrowHeadOffsetY);

            ///right triangle
            glVertex2d(+arrowXHalfSize, 0.);
            glVertex2d(+arrowXHalfSize - arrowHeadOffsetX, arrowHeadOffsetY);

            glVertex2d(+arrowXHalfSize, 0.);
            glVertex2d(+arrowXHalfSize - arrowHeadOffsetX, -arrowHeadOffsetY);
            glEnd();

            glRotated(90., 0., 0., 1.);

            glBegin(GL_LINES);
            ///draw the central bar
            glVertex2d(-arrowXHalfSize, 0.);
            glVertex2d(+arrowXHalfSize, 0.);

            ///left triangle
            glVertex2d(-arrowXHalfSize, 0.);
            glVertex2d(-arrowXHalfSize + arrowHeadOffsetX, arrowHeadOffsetY);

            glVertex2d(-arrowXHalfSize, 0.);
            glVertex2d(-arrowXHalfSize + arrowHeadOffsetX, -arrowHeadOffsetY);

            ///right triangle
            glVertex2d(+arrowXHalfSize, 0.);
            glVertex2d(+arrowXHalfSize - arrowHeadOffsetX, arrowHeadOffsetY);

            glVertex2d(+arrowXHalfSize, 0.);
            glVertex2d(+arrowXHalfSize - arrowHeadOffsetX, -arrowHeadOffsetY);
            glEnd();

            glPopMatrix();
        }
    } // drawRotationBar

    // draw the interact
    bool
        TransformInteractHelper::draw(const DrawArgs& args)
    {
        if (!_interactOpen->getValueAtTime(args.time)) {
            return false;
        }
        const OfxPointD& pscale = args.pixelScale;
        const double time = args.time;

        bool hiDPI = _hiDPI ? _hiDPI->getValue() : false;
        double screenPixelRatio = hiDPI ? 2 : 1;
#ifdef OFX_EXTENSIONS_NATRON
        screenPixelRatio *= args.screenPixelRatio;
        //hiDPI |= args.screenPixelRatio > 1;
#endif
    //TextRenderer::Font font = hiDPI ? TextRenderer::FONT_TIMES_ROMAN_24 : TextRenderer::FONT_HELVETICA_12;

        OfxRGBColourD color = { 0.8, 0.8, 0.8 };
        _interact->getSuggestedColour(color);
        GLdouble projection[16];
        glGetDoublev(GL_PROJECTION_MATRIX, projection);
        GLint viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);
        OfxPointD shadow; // how much to translate GL_PROJECTION to get exactly one pixel on screen
        shadow.x = 2. / (projection[0] * viewport[2]);
        shadow.y = 2. / (projection[5] * viewport[3]);

        TransformParams tp;

        if (_mouseState == eReleased) {
            getTransformParams(time, tp);
        }
        else {
            tp = _tpDrag;
        }

        OfxPointD targetCenter;
        getTargetCenter(tp.center, tp.translate, &targetCenter);

        OfxPointD scale;
        ofxsTransformGetScale(tp.scale, tp.scaleUniform, tp.flop, tp.flip, &scale);

        OfxPointD targetRadius;
        getTargetRadius(scale, pscale, &targetRadius);

        OfxPointD left, right, bottom, top;
        getTargetPoints(targetCenter, targetRadius, &left, &bottom, &top, &right);


        GLdouble skewMatrix[16];
        skewMatrix[0] = (tp.skewOrder ? 1. : (1. + tp.skewX * tp.skewY)); skewMatrix[1] = tp.skewY; skewMatrix[2] = 0.; skewMatrix[3] = 0;
        skewMatrix[4] = tp.skewX; skewMatrix[5] = (tp.skewOrder ? (1. + tp.skewX * tp.skewY) : 1.); skewMatrix[6] = 0.; skewMatrix[7] = 0;
        skewMatrix[8] = 0.; skewMatrix[9] = 0.; skewMatrix[10] = 1.; skewMatrix[11] = 0;
        skewMatrix[12] = 0.; skewMatrix[13] = 0.; skewMatrix[14] = 0.; skewMatrix[15] = 1.;

        //glPushAttrib(GL_ALL_ATTRIB_BITS); // caller is responsible for protecting attribs

        glDisable(GL_LINE_STIPPLE);
        glEnable(GL_LINE_SMOOTH);
        glEnable(GL_POINT_SMOOTH);
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

            // First, draw the motion curve, based on the keyframes for the translate parameter.
            if (_translate) {
                // Draw the motion curve a bit darker
                const double darken = 0.5;
                glColor3d(color.r * darken * l, color.g * darken * l, color.b * darken * l);

                int numKeys = _translate->getNumKeys();

                // Try to draw one point per frame, but no more thankDrawPointTrajectoryMaxPoints
                // in total, and at least kDrawPointTrajectoryMinStepsPerKey between two keys.
                if (numKeys > 0) {
                    OfxPointD targetCenter;
                    OfxPointD center = { 0., 0. };
                    OfxPointD translate = { 0., 0. };
                    int maxStepsPerKey = int(kDrawPointTrajectoryMaxPoints / numKeys);
                    glPointSize(POINT_SIZE * screenPixelRatio);
                    glBegin(GL_POINTS);
                    for (int i = 0; i < numKeys; ++i) {
                        double time = _translate->getKeyTime(i);
                        if (_center) {
                            _center->getValueAtTime(time, center.x, center.y);
                        }
                        if (_translate) {
                            _translate->getValueAtTime(time, translate.x, translate.y);
                        }
                        getTargetCenter(center, translate, &targetCenter);
                        glVertex2d(targetCenter.x, targetCenter.y);

                    }
                    glEnd();
                    glBegin(GL_LINE_STRIP);
                    double time = _translate->getKeyTime(0);
                    for (int i = 1; i < numKeys; ++i) {
                        double timeNext = _translate->getKeyTime(i);
                        int steps = std::max(kDrawPointTrajectoryMinStepsPerKey, std::min(int(timeNext - time), maxStepsPerKey));
                        for (int j = (i == 1 ? 0 : 1); j <= steps; ++j) {
                            double timeStep = time + j * (timeNext - time) / steps;
                            if (_center) {
                                _center->getValueAtTime(timeStep, center.x, center.y);
                            }
                            if (_translate) {
                                _translate->getValueAtTime(timeStep, translate.x, translate.y);
                            }
                            getTargetCenter(center, translate, &targetCenter);
                            glVertex2d(targetCenter.x, targetCenter.y);
                        }
                        time = timeNext;
                    }
                    glEnd();
                }
            }


            glColor3d(color.r * l, color.g * l, color.b * l);

            glPushMatrix();
            glTranslated(targetCenter.x, targetCenter.y, 0.);

            glRotated(tp.rotate, 0, 0., 1.);
            drawRotationBar(color, pscale, targetRadius.x, _mouseState == eDraggingRotationBar || _drawState == eRotationBarHovered, tp.inverted, l);
            glMultMatrixd(skewMatrix);
            glTranslated(-targetCenter.x, -targetCenter.y, 0.);

            drawEllipse(color, targetCenter, targetRadius, _mouseState == eDraggingCircle || _drawState == eCircleHovered, l);

            // add 180 to the angle to draw the arrows on the other side. unfortunately, this requires knowing
            // the mouse position in the ellipse frame
            double flip = 0.;
            if ((_drawState == eSkewXBarHoverered) || (_drawState == eSkewYBarHoverered)) {
                double rot = ofxsToRadians(tp.rotate);
                Matrix3x3 transformscale;
                transformscale = ofxsMatInverseTransformCanonical(0., 0., scale.x, scale.y, tp.skewX, tp.skewY, (bool)tp.skewOrder, rot, targetCenter.x, targetCenter.y);

                Point3D previousPos;
                previousPos.x = _lastMousePos.x;
                previousPos.y = _lastMousePos.y;
                previousPos.z = 1.;
                previousPos = transformscale * previousPos;
                if (previousPos.z != 0) {
                    previousPos.x /= previousPos.z;
                    previousPos.y /= previousPos.z;
                }
                if (((_drawState == eSkewXBarHoverered) && (previousPos.y > targetCenter.y)) ||
                    ((_drawState == eSkewYBarHoverered) && (previousPos.x > targetCenter.x))) {
                    flip = 180.;
                }
            }
            drawSkewBar(color, targetCenter, pscale, targetRadius.y, _mouseState == eDraggingSkewXBar || _drawState == eSkewXBarHoverered, flip, l);
            drawSkewBar(color, targetCenter, pscale, targetRadius.x, _mouseState == eDraggingSkewYBar || _drawState == eSkewYBarHoverered, flip - 90., l);


            drawSquare(color, targetCenter, pscale, _mouseState == eDraggingTranslation || _mouseState == eDraggingCenter || _drawState == eCenterPointHovered, (!_translate || _modifierStateCtrl), l);
            drawSquare(color, left, pscale, _mouseState == eDraggingLeftPoint || _drawState == eLeftPointHovered, false, l);
            drawSquare(color, right, pscale, _mouseState == eDraggingRightPoint || _drawState == eRightPointHovered, false, l);
            drawSquare(color, top, pscale, _mouseState == eDraggingTopPoint || _drawState == eTopPointHovered, false, l);
            drawSquare(color, bottom, pscale, _mouseState == eDraggingBottomPoint || _drawState == eBottomPointHovered, false, l);

            glPopMatrix();
        }
        //glPopAttrib();

        return true;
    } // TransformInteractHelper::draw

    static bool
        squareContains(const Point3D& pos,
            const OfxRectD& rect,
            double toleranceX = 0.,
            double toleranceY = 0.)
    {
        return (pos.x >= (rect.x1 - toleranceX) && pos.x < (rect.x2 + toleranceX)
            && pos.y >= (rect.y1 - toleranceY) && pos.y < (rect.y2 + toleranceY));
    }

    static bool
        isOnEllipseBorder(const Point3D& pos,
            const OfxPointD& targetRadius,
            const OfxPointD& targetCenter,
            double epsilon = 0.1)
    {
        double v = ((pos.x - targetCenter.x) * (pos.x - targetCenter.x) / (targetRadius.x * targetRadius.x) +
            (pos.y - targetCenter.y) * (pos.y - targetCenter.y) / (targetRadius.y * targetRadius.y));

        if ((v <= (1. + epsilon)) && (v >= (1. - epsilon))) {
            return true;
        }

        return false;
    }

    static bool
        isOnSkewXBar(const Point3D& pos,
            double targetRadiusY,
            const OfxPointD& center,
            const OfxPointD& pixelScale,
            double tolerance)
    {
        // we are not axis-aligned
        double meanPixelScale = (pixelScale.x + pixelScale.y) / 2.;
        double barHalfSize = targetRadiusY + (20. * meanPixelScale);

        if ((pos.x >= (center.x - tolerance)) && (pos.x <= (center.x + tolerance)) &&
            (pos.y >= (center.y - barHalfSize - tolerance)) && (pos.y <= (center.y + barHalfSize + tolerance))) {
            return true;
        }

        return false;
    }

    static bool
        isOnSkewYBar(const Point3D& pos,
            double targetRadiusX,
            const OfxPointD& center,
            const OfxPointD& pixelScale,
            double tolerance)
    {
        // we are not axis-aligned
        double meanPixelScale = (pixelScale.x + pixelScale.y) / 2.;
        double barHalfSize = targetRadiusX + (20. * meanPixelScale);

        if ((pos.y >= (center.y - tolerance)) && (pos.y <= (center.y + tolerance)) &&
            (pos.x >= (center.x - barHalfSize - tolerance)) && (pos.x <= (center.x + barHalfSize + tolerance))) {
            return true;
        }

        return false;
    }

    static bool
        isOnRotationBar(const Point3D& pos,
            double targetRadiusX,
            const OfxPointD& center,
            const OfxPointD& pixelScale,
            double tolerance)
    {
        // we are not axis-aligned
        double meanPixelScale = (pixelScale.x + pixelScale.y) / 2.;
        double barExtra = 30. * meanPixelScale;

        if ((pos.x >= (center.x - tolerance)) && (pos.x <= (center.x + targetRadiusX + barExtra + tolerance)) &&
            (pos.y >= (center.y - tolerance)) && (pos.y <= (center.y + tolerance))) {
            return true;
        }

        return false;
    }

    static OfxRectD
        rectFromCenterPoint(const OfxPointD& center,
            const OfxPointD& pixelScale)
    {
        // we are not axis-aligned
        double meanPixelScale = (pixelScale.x + pixelScale.y) / 2.;
        OfxRectD ret;

        ret.x1 = center.x - (POINT_SIZE / 2.) * meanPixelScale;
        ret.x2 = center.x + (POINT_SIZE / 2.) * meanPixelScale;
        ret.y1 = center.y - (POINT_SIZE / 2.) * meanPixelScale;
        ret.y2 = center.y + (POINT_SIZE / 2.) * meanPixelScale;

        return ret;
    }

    // round to the closest int, 1/10 int, etc
    // this make parameter editing easier
    // pscale is args.pixelScale.x / args.renderScale.x;
    // pscale10 is the power of 10 below pscale
    static double
        fround(double val,
            double pscale)
    {
        double pscale10 = std::pow(10., std::floor(std::log10(pscale)));

        return pscale10 * std::floor(val / pscale10 + 0.5);
    }

    // overridden functions from Interact to do things
    bool
        TransformInteractHelper::penMotion(const PenArgs& args)
    {
        if (!_interactOpen->getValueAtTime(args.time)) {
            return false;
        }
        const OfxPointD& pscale = args.pixelScale;
        const double time = args.time;
        TransformParams tpCurrent;
        bool tpCurrentSet = false;
        TransformParams tp;

        if (_mouseState == eReleased) {
            getTransformParams(time, tpCurrent);
            tpCurrentSet = true;
            tp = tpCurrent;
        }
        else {
            tp = _tpDrag;
        }

        bool redraw = false;
        bool centerChanged = false;
        bool translateChanged = false;
        bool scaleChanged = false;
        bool rotateChanged = false;
        bool skewXChanged = false;
        bool skewYChanged = false;
        OfxPointD targetCenter;
        getTargetCenter(tp.center, tp.translate, &targetCenter);

        OfxPointD scale;
        ofxsTransformGetScale(tp.scale, tp.scaleUniform, tp.flop, tp.flip, &scale);

        OfxPointD targetRadius;
        getTargetRadius(scale, pscale, &targetRadius);

        OfxPointD left, right, bottom, top;
        getTargetPoints(targetCenter, targetRadius, &left, &bottom, &top, &right);

        OfxRectD centerPoint = rectFromCenterPoint(targetCenter, pscale);
        OfxRectD leftPoint = rectFromCenterPoint(left, pscale);
        OfxRectD rightPoint = rectFromCenterPoint(right, pscale);
        OfxRectD topPoint = rectFromCenterPoint(top, pscale);
        OfxRectD bottomPoint = rectFromCenterPoint(bottom, pscale);


        //double dx = args.penPosition.x - _lastMousePos.x;
        //double dy = args.penPosition.y - _lastMousePos.y;
        double rot = ofxsToRadians(tp.rotate);
        Point3D penPos, prevPenPos, rotationPos, transformedPos, previousPos, currentPos;
        penPos.x = args.penPosition.x;
        penPos.y = args.penPosition.y;
        penPos.z = 1.;
        prevPenPos.x = _lastMousePos.x;
        prevPenPos.y = _lastMousePos.y;
        prevPenPos.z = 1.;

        Matrix3x3 rotation, transform, transformscale;
        ////for the rotation bar/translation/center dragging we dont use the same transform, we don't want to undo the rotation transform
        if ((_mouseState != eDraggingTranslation) && (_mouseState != eDraggingCenter)) {
            ///undo skew + rotation to the current position
            rotation = ofxsMatInverseTransformCanonical(0., 0., 1., 1., 0., 0., false, rot, targetCenter.x, targetCenter.y);
            transform = ofxsMatInverseTransformCanonical(0., 0., 1., 1., tp.skewX, tp.skewY, (bool)tp.skewOrder, rot, targetCenter.x, targetCenter.y);
            transformscale = ofxsMatInverseTransformCanonical(0., 0., scale.x, scale.y, tp.skewX, tp.skewY, (bool)tp.skewOrder, rot, targetCenter.x, targetCenter.y);
        }
        else {
            rotation = ofxsMatInverseTransformCanonical(0., 0., 1., 1., 0., 0., false, 0., targetCenter.x, targetCenter.y);
            transform = ofxsMatInverseTransformCanonical(0., 0., 1., 1., tp.skewX, tp.skewY, (bool)tp.skewOrder, 0., targetCenter.x, targetCenter.y);
            transformscale = ofxsMatInverseTransformCanonical(0., 0., scale.x, scale.y, tp.skewX, tp.skewY, (bool)tp.skewOrder, 0., targetCenter.x, targetCenter.y);
        }

        rotationPos = rotation * penPos;
        if (rotationPos.z != 0) {
            rotationPos.x /= rotationPos.z;
            rotationPos.y /= rotationPos.z;
        }

        transformedPos = transform * penPos;
        if (transformedPos.z != 0) {
            transformedPos.x /= transformedPos.z;
            transformedPos.y /= transformedPos.z;
        }

        previousPos = transformscale * prevPenPos;
        if (previousPos.z != 0) {
            previousPos.x /= previousPos.z;
            previousPos.y /= previousPos.z;
        }

        currentPos = transformscale * penPos;
        if (currentPos.z != 0) {
            currentPos.x /= currentPos.z;
            currentPos.y /= currentPos.z;
        }

        if (_mouseState == eReleased) {
            // we are not axis-aligned
            double meanPixelScale = (pscale.x + pscale.y) / 2.;
            double hoverTolerance = (POINT_SIZE / 2.) * meanPixelScale;
            DrawStateEnum newState = _drawState;
            if (squareContains(transformedPos, centerPoint)) {
                newState = eCenterPointHovered;
            }
            else if (squareContains(transformedPos, leftPoint)) {
                newState = eLeftPointHovered;
            }
            else if (squareContains(transformedPos, rightPoint)) {
                newState = eRightPointHovered;
            }
            else if (squareContains(transformedPos, topPoint)) {
                newState = eTopPointHovered;
            }
            else if (squareContains(transformedPos, bottomPoint)) {
                newState = eBottomPointHovered;
            }
            else if (isOnEllipseBorder(transformedPos, targetRadius, targetCenter)) {
                newState = eCircleHovered;
            }
            else if (isOnRotationBar(rotationPos, targetRadius.x, targetCenter, pscale, hoverTolerance)) {
                newState = eRotationBarHovered;
            }
            else if (isOnSkewXBar(transformedPos, targetRadius.y, targetCenter, pscale, hoverTolerance)) {
                newState = eSkewXBarHoverered;
            }
            else if (isOnSkewYBar(transformedPos, targetRadius.x, targetCenter, pscale, hoverTolerance)) {
                newState = eSkewYBarHoverered;
            }
            else {
                newState = eInActive;
            }
            redraw = _drawState != newState;
            _drawState = newState;
        }
        else if (_mouseState == eDraggingCircle) {
            double minX, minY, maxX, maxY;
            _scale->getRange(minX, minY, maxX, maxY);

            // we need to compute the backtransformed points with the scale

            // the scale ratio is the ratio of distances to the center
            double prevDistSq = (targetCenter.x - previousPos.x) * (targetCenter.x - previousPos.x) + (targetCenter.y - previousPos.y) * (targetCenter.y - previousPos.y);
            if (prevDistSq != 0.) {
                const double distSq = (targetCenter.x - currentPos.x) * (targetCenter.x - currentPos.x) + (targetCenter.y - currentPos.y) * (targetCenter.y - currentPos.y);
                const double distRatio = std::sqrt((std::max)(distSq / prevDistSq, 0.));
                scale.x *= distRatio;
                scale.y *= distRatio;
                tp.scale = scale;
                scaleChanged = true;
            }
        }
        else if ((_mouseState == eDraggingLeftPoint) || (_mouseState == eDraggingRightPoint)) {
            // avoid division by zero
            if (targetCenter.x != previousPos.x) {
                double minX, minY, maxX, maxY;
                _scale->getRange(minX, minY, maxX, maxY);
                const double scaleRatio = (targetCenter.x - currentPos.x) / (targetCenter.x - previousPos.x);
                OfxPointD newScale;
                newScale.x = scale.x * scaleRatio * ((tp.flop) ? -1 : 1);
                newScale.x = (std::max)(minX, (std::min)(newScale.x, maxX));
                newScale.y = tp.scaleUniform ? newScale.x : scale.y * ((tp.flip) ? -1 : 1);
                tp.scale = newScale;
                scaleChanged = true;
            }
        }
        else if ((_mouseState == eDraggingTopPoint) || (_mouseState == eDraggingBottomPoint)) {
            // avoid division by zero
            if (targetCenter.y != previousPos.y) {
                double minX, minY, maxX, maxY;
                _scale->getRange(minX, minY, maxX, maxY);
                const double scaleRatio = (targetCenter.y - currentPos.y) / (targetCenter.y - previousPos.y);
                OfxPointD newScale;
                newScale.y = scale.y * scaleRatio * ((tp.flip) ? -1 : 1);
                newScale.y = (std::max)(minY, (std::min)(newScale.y, maxY));
                newScale.x = tp.scaleUniform ? newScale.y : scale.x * ((tp.flop) ? -1 : 1);
                tp.scale = newScale;
                scaleChanged = true;
            }
        }
        else if (_mouseState == eDraggingTranslation) {
            double dx = args.penPosition.x - _lastMousePos.x;
            double dy = args.penPosition.y - _lastMousePos.y;

            if ((_orientation == eOrientationNotSet) && (_modifierStateShift > 0)) {
                _orientation = std::abs(dx) > std::abs(dy) ? eOrientationHorizontal : eOrientationVertical;
            }

            dx = _orientation == eOrientationVertical ? 0 : dx;
            dy = _orientation == eOrientationHorizontal ? 0 : dy;

            double newx = tp.translate.x + dx;
            double newy = tp.translate.y + dy;

            // round newx/y to the closest int, 1/10 int, etc
            // this make parameter editing easier
            newx = fround(newx, pscale.x);
            newy = fround(newy, pscale.y);
            tp.translate.x = newx;
            tp.translate.y = newy;
            translateChanged = true;
        }
        else if (_mouseState == eDraggingCenter) {
            OfxPointD currentCenter = tp.center;
            Matrix3x3 R = ofxsMatScale(1. / scale.x, 1. / scale.y) * ofxsMatSkewXY(-tp.skewX, -tp.skewY, !tp.skewOrder) * ofxsMatRotation(rot);
            double dx = args.penPosition.x - _lastMousePos.x;
            double dy = args.penPosition.y - _lastMousePos.y;

            if ((_orientation == eOrientationNotSet) && (_modifierStateShift > 0)) {
                _orientation = std::abs(dx) > std::abs(dy) ? eOrientationHorizontal : eOrientationVertical;
            }

            dx = _orientation == eOrientationVertical ? 0 : dx;
            dy = _orientation == eOrientationHorizontal ? 0 : dy;

            double dxrot, dyrot;
            if (!_translate) {
                dxrot = dx;
                dyrot = dy;
            }
            else {
                // if there is a _translate param (i.e. this is Transform/DirBlur and not GodRays),
                // compensate the rotation, because the
                // interact is visualized on the transformed image
                Point3D dRot;
                dRot.x = dx;
                dRot.y = dy;
                dRot.z = 1.;
                dRot = R * dRot;
                if (dRot.z != 0) {
                    dRot.x /= dRot.z;
                    dRot.y /= dRot.z;
                }
                dxrot = dRot.x;
                dyrot = dRot.y;
            }
            double newx = currentCenter.x + dxrot;
            double newy = currentCenter.y + dyrot;
            // round newx/y to the closest int, 1/10 int, etc
            // this make parameter editing easier
            newx = fround(newx, pscale.x);
            newy = fround(newy, pscale.y);
            tp.center.x = newx;
            tp.center.y = newy;
            centerChanged = true;
            if (_translate) {
                // recompute dxrot,dyrot after rounding
                Matrix3x3 Rinv;
                if (R.inverse(&Rinv)) {
                    dxrot = newx - currentCenter.x;
                    dyrot = newy - currentCenter.y;
                    Point3D dRot;
                    dRot.x = dxrot;
                    dRot.y = dyrot;
                    dRot.z = 1;
                    dRot = Rinv * dRot;
                    if (dRot.z != 0) {
                        dRot.x /= dRot.z;
                        dRot.y /= dRot.z;
                    }
                    dx = dRot.x;
                    dy = dRot.y;
                    OfxPointD newTranslation;
                    newTranslation.x = tp.translate.x + dx - dxrot;
                    newTranslation.y = tp.translate.y + dy - dyrot;
                    tp.translate = newTranslation;
                    translateChanged = true;
                }
            }
        }
        else if (_mouseState == eDraggingRotationBar) {
            OfxPointD diffToCenter;
            ///the current mouse position (untransformed) is doing has a certain angle relative to the X axis
            ///which can be computed by : angle = arctan(opposite / adjacent)
            diffToCenter.y = rotationPos.y - targetCenter.y;
            diffToCenter.x = rotationPos.x - targetCenter.x;
            double angle = std::atan2(diffToCenter.y, diffToCenter.x);
            double angledegrees = tp.rotate + ofxsToDegrees(angle);
            double closest90 = 90. * std::floor((angledegrees + 45.) / 90.);
            if (std::fabs(angledegrees - closest90) < 5.) {
                // snap to closest multiple of 90.
                angledegrees = closest90;
            }
            tp.rotate = angledegrees;
            rotateChanged = true;
        }
        else if (_mouseState == eDraggingSkewXBar) {
            // avoid division by zero
            if ((scale.y != 0.) && (targetCenter.y != previousPos.y)) {
                const double addSkew = (scale.x / scale.y) * (currentPos.x - previousPos.x) / (currentPos.y - targetCenter.y);
                tp.skewX = tp.skewX + addSkew;
                skewXChanged = true;
            }
        }
        else if (_mouseState == eDraggingSkewYBar) {
            // avoid division by zero
            if ((scale.x != 0.) && (targetCenter.x != previousPos.x)) {
                const double addSkew = (scale.y / scale.x) * (currentPos.y - previousPos.y) / (currentPos.x - targetCenter.x);
                tp.skewY = tp.skewY + addSkew;
                skewYChanged = true;
            }
        }
        else {
            assert(false);
        }

        _tpDrag = tp;

        bool valuesChanged = (centerChanged || translateChanged || scaleChanged || rotateChanged || skewXChanged || skewYChanged);

        if ((_mouseState != eReleased) && _interactiveDrag && valuesChanged) {
            // no need to redraw overlay since it is slave to the paramaters

            // Change from 2021/06/01:
            // Value changes from the interact should set a keyframe on all keyframed values, for more clarity.
            // see https://github.com/NatronGitHub/Natron/issues/630#issuecomment-852301906
            // Change from 2022/11/12:
            // Only setValue on parameters that make sense (eg setValue would actually change their value)
            if (!tpCurrentSet) {
                getTransformParams(time, tpCurrent);
                tpCurrentSet = true;
            }
            centerChanged = centerChanged && (tpCurrent.center.x != tp.center.x || tpCurrent.center.y != tp.center.y);
            translateChanged = translateChanged && (tpCurrent.translate.x != tp.translate.x || tpCurrent.translate.y != tp.translate.y);
            scaleChanged = scaleChanged && (tpCurrent.scale.x != tp.scale.x || tpCurrent.scale.y != tp.scale.y);
            rotateChanged = rotateChanged && (tpCurrent.rotate != tp.rotate);
            skewXChanged = skewXChanged && (tpCurrent.skewX != tp.skewX);
            skewYChanged = skewYChanged && (tpCurrent.skewY != tp.skewY);

            bool editBlockNecessary = (centerChanged + translateChanged + scaleChanged + rotateChanged + skewXChanged + skewYChanged) > 1;
            ImageEffect::EditBlock eb(*_effect, "Set Transform", editBlockNecessary);
            if (centerChanged) {
                _center->setValue(tp.center.x, tp.center.y);
            }
            if (translateChanged) {
                _translate->setValue(tp.translate.x, tp.translate.y);
            }
            if (scaleChanged) {
                _scale->setValue(tp.scale.x, tp.scale.y);
            }
            if (rotateChanged) {
                _rotate->setValue(tp.rotate);
            }
            if (skewXChanged) {
                _skewX->setValue(tp.skewX);
            }
            if (skewYChanged) {
                _skewY->setValue(tp.skewY);
            }
        }
        else if (redraw || valuesChanged) {
            _interact->requestRedraw();
        }

        _lastMousePos = args.penPosition;

        return valuesChanged;
    } // TransformInteractHelper::penMotion

    bool
        TransformInteractHelper::penDown(const PenArgs& args)
    {
        if (!_interactOpen->getValueAtTime(args.time)) {
            return false;
        }

        const OfxPointD& pscale = args.pixelScale;
        const double time = args.time;
        TransformParams tp;

        if (_mouseState == eReleased) {
            getTransformParams(time, tp);
            if (_interactive) {
                _interactive->getValueAtTime(args.time, _interactiveDrag);
            }
        }
        else {
            tp = _tpDrag;
        }

        OfxPointD targetCenter;
        getTargetCenter(tp.center, tp.translate, &targetCenter);

        OfxPointD scale;
        ofxsTransformGetScale(tp.scale, tp.scaleUniform, tp.flop, tp.flip, &scale);

        OfxPointD targetRadius;
        getTargetRadius(scale, pscale, &targetRadius);

        OfxPointD left, right, bottom, top;
        getTargetPoints(targetCenter, targetRadius, &left, &bottom, &top, &right);

        OfxRectD centerPoint = rectFromCenterPoint(targetCenter, pscale);
        OfxRectD leftPoint = rectFromCenterPoint(left, pscale);
        OfxRectD rightPoint = rectFromCenterPoint(right, pscale);
        OfxRectD topPoint = rectFromCenterPoint(top, pscale);
        OfxRectD bottomPoint = rectFromCenterPoint(bottom, pscale);
        Point3D transformedPos, rotationPos;
        transformedPos.x = args.penPosition.x;
        transformedPos.y = args.penPosition.y;
        transformedPos.z = 1.;

        double rot = ofxsToRadians(tp.rotate);

        ///now undo skew + rotation to the current position
        Matrix3x3 rotation, transform;
        rotation = ofxsMatInverseTransformCanonical(0., 0., 1., 1., 0., 0., false, rot, targetCenter.x, targetCenter.y);
        transform = ofxsMatInverseTransformCanonical(0., 0., 1., 1., tp.skewX, tp.skewY, (bool)tp.skewOrder, rot, targetCenter.x, targetCenter.y);

        rotationPos = rotation * transformedPos;
        if (rotationPos.z != 0) {
            rotationPos.x /= rotationPos.z;
            rotationPos.y /= rotationPos.z;
        }
        transformedPos = transform * transformedPos;
        if (transformedPos.z != 0) {
            transformedPos.x /= transformedPos.z;
            transformedPos.y /= transformedPos.z;
        }

        _orientation = eOrientationAllDirections;

        double pressToleranceX = 5 * pscale.x;
        double pressToleranceY = 5 * pscale.y;
        bool didSomething = false;
        MouseStateEnum newState = _mouseState;
        if (squareContains(transformedPos, centerPoint, pressToleranceX, pressToleranceY)) {
            newState = ((!_translate || _modifierStateCtrl) ? eDraggingCenter : eDraggingTranslation);
            if (_modifierStateShift > 0) {
                _orientation = eOrientationNotSet;
            }
        }
        else if (squareContains(transformedPos, leftPoint, pressToleranceX, pressToleranceY)) {
            newState = eDraggingLeftPoint;
        }
        else if (squareContains(transformedPos, rightPoint, pressToleranceX, pressToleranceY)) {
            newState = eDraggingRightPoint;
        }
        else if (squareContains(transformedPos, topPoint, pressToleranceX, pressToleranceY)) {
            newState = eDraggingTopPoint;
        }
        else if (squareContains(transformedPos, bottomPoint, pressToleranceX, pressToleranceY)) {
            newState = eDraggingBottomPoint;
        }
        else if (isOnEllipseBorder(transformedPos, targetRadius, targetCenter)) {
            newState = eDraggingCircle;
        }
        else if (isOnRotationBar(rotationPos, targetRadius.x, targetCenter, pscale, pressToleranceY)) {
            newState = eDraggingRotationBar;
        }
        else if (isOnSkewXBar(transformedPos, targetRadius.y, targetCenter, pscale, pressToleranceY)) {
            newState = eDraggingSkewXBar;
        }
        else if (isOnSkewYBar(transformedPos, targetRadius.x, targetCenter, pscale, pressToleranceX)) {
            newState = eDraggingSkewYBar;
        }
        else {
            newState = eReleased;
        }
        didSomething = _mouseState != newState;
        _mouseState = newState;

        _lastMousePos = args.penPosition;

        _tpDrag = tp;

        if (didSomething) {
            _interact->requestRedraw();
        }

        return didSomething;
    } // TransformInteractHelper::penDown

    bool
        TransformInteractHelper::penUp(const PenArgs& args)
    {
        if (!_interactOpen->getValueAtTime(args.time)) {
            return false;
        }
        bool ret = _mouseState != eReleased;

        if (!_interactiveDrag && ret) {
            const TransformParams& tp = _tpDrag;
            OfxPointD scale;
            ofxsTransformGetScale(tp.scale, tp.scaleUniform, tp.flop, tp.flip, &scale);
            ImageEffect::EditBlock eb(*_effect, "Set Transform");
            _center->setValue(tp.center.x, tp.center.y);
            _translate->setValue(tp.translate.x, tp.translate.y);
            _scale->setValue(scale.x, scale.y);
            _rotate->setValue(tp.rotate);
            _skewX->setValue(tp.skewX);
            _skewY->setValue(tp.skewY);
        }

        if (ret) {
            // Request redraw, because items may be highlighted in previous state.
            _interact->requestRedraw();
        }

        _mouseState = eReleased;
        _lastMousePos = args.penPosition;

        return ret;
    }

    // keyDown just updates the modifier state
    bool
        TransformInteractHelper::keyDown(const KeyArgs& args)
    {
        // Always process, even if interact is not open, since this concerns modifiers
        //if (!_interactOpen->getValueAtTime(args.time)) {
        //    return false;
        //}

        // Note that on the Mac:
        // cmd/apple/cloverleaf is kOfxKey_Control_L
        // ctrl is kOfxKey_Meta_L
        // alt/option is kOfxKey_Alt_L
        bool mustRedraw = false;

        // the two control keys may be pressed consecutively, be aware about this
        if (_translate && ((args.keySymbol == kOfxKey_Control_L) || (args.keySymbol == kOfxKey_Control_R))) {
            mustRedraw = (_modifierStateCtrl == 0);
            ++_modifierStateCtrl;
        }
        if ((args.keySymbol == kOfxKey_Shift_L) || (args.keySymbol == kOfxKey_Shift_R)) {
            mustRedraw = (_modifierStateShift == 0);
            ++_modifierStateShift;
            if (_modifierStateShift > 0) {
                _orientation = eOrientationNotSet;
            }
        }
        if (mustRedraw) {
            _interact->requestRedraw();
        }
        //std::cout << std::hex << args.keySymbol << std::endl;

        // modifiers are not "caught"
        return false;
    }

    // keyUp just updates the modifier state
    bool
        TransformInteractHelper::keyUp(const KeyArgs& args)
    {
        // Always process, even if interact is not open, since this concerns modifiers
        //if (!_interactOpen->getValueAtTime(args.time)) {
        //    return false;
        //}

        bool mustRedraw = false;

        if (_translate && ((args.keySymbol == kOfxKey_Control_L) || (args.keySymbol == kOfxKey_Control_R))) {
            // we may have missed a keypress
            if (_modifierStateCtrl > 0) {
                --_modifierStateCtrl;
                mustRedraw = (_modifierStateCtrl == 0);
            }
        }
        if ((args.keySymbol == kOfxKey_Shift_L) || (args.keySymbol == kOfxKey_Shift_R)) {
            if (_modifierStateShift > 0) {
                --_modifierStateShift;
                mustRedraw = (_modifierStateShift == 0);
            }
            if (_modifierStateShift == 0) {
                _orientation = eOrientationAllDirections;
            }
        }
        if (mustRedraw) {
            _interact->requestRedraw();
        }

        // modifiers are not "caught"
        return false;
    }

    /** @brief Called when the interact is loses input focus */
    void
        TransformInteractHelper::loseFocus(const FocusArgs& /*args*/)
    {
        // reset the modifiers state
        if (_translate) {
            _modifierStateCtrl = 0;
        }
        _modifierStateShift = 0;
        _interactiveDrag = false;
        _mouseState = eReleased;
        _drawState = eInActive;
    }
} // namespace OFX
