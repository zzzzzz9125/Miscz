/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of openfx-misc <https://github.com/NatronGitHub/openfx-misc>,
 * (C) 2018-2021 The Natron Developers
 * (C) 2013-2018 INRIA
 *
 * openfx-misc is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * openfx-misc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with openfx-Miscz.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

/*
 * OFX Transform & DirBlur plugins.
 */

#include <cmath>
#include <iostream>
#include <vector>

#include "ofxsTransform3x3.h"
#include "ofxsTransformInteract.h"
#include "ofxsCoords.h"
#include "ofxsThreadSuite.h"
#include "exprtk.hpp"

using namespace OFX;

OFXS_NAMESPACE_ANONYMOUS_ENTER

#define kPluginName "Mz_TransformOFX"
#define kPluginMaskedName "Mz_TransformOFX"
#define kPluginGrouping "MisczOFX"
#define kPluginDescription "Translate / Rotate / Scale a 2D image.\n" \
    "This plugin concatenates transforms.\n" \
    "See also https://web.archive.org/web/20220627030948/http://www.opticalenquiry.com/nuke/index.php?title=Transform"

#define kPluginMaskedDescription "Translate / Rotate / Scale a 2D image, with optional masking.\n" \
    "This plugin concatenates transforms upstream."
#define kPluginIdentifier "net.sf.openfx.MzTransformPlugin"
#define kPluginMaskedIdentifier "net.sf.openfx.MzTransformPlugin"
//#define kPluginDirBlurName "Mz_DirBlurOFX"
//#define kPluginDirBlurGrouping "Filter"
//#define kPluginDirBlurDescription "Apply directional blur to an image.\n" \
//    "This plugin concatenates transforms upstream."
//#define kPluginDirBlurIdentifier "net.sf.openfx.MzDirBlur"
#define kPluginVersionMajor 2 // Incrementing this number means that you have broken backwards compatibility of the plug-in.
#define kPluginVersionMinor 2 // Increment this when you have fixed a bug or made it faster.

#define kParamSrcClipChanged "srcClipChanged"

#ifndef M_PI
#define M_PI        3.14159265358979323846264338327950288   /* pi             */
#endif

#ifndef M_E
#define M_E         2.71828182845904523536028747135266249   /* e              */
#endif

enum CurveTypeEnum
{
    eCurveTypeCustom,
    eCurveTypeDefault,
    eCurveTypeEase,
    eCurveTypeEaseIn,
    eCurveTypeEaseOut,
    eCurveTypeQuad,
    eCurveTypeQuadIn,
    eCurveTypeQuadOut,
    eCurveTypeCubic,
    eCurveTypeCubicIn,
    eCurveTypeCubicOut,
    eCurveTypeQuart,
    eCurveTypeQuartIn,
    eCurveTypeQuartOut,
    eCurveTypeQuint,
    eCurveTypeQuintIn,
    eCurveTypeQuintOut,
    eCurveTypeExpo,
    eCurveTypeExpoIn,
    eCurveTypeExpoOut,
    eCurveTypeCirc,
    eCurveTypeCircIn,
    eCurveTypeCircOut,
    eCurveTypeBack,
    eCurveTypeBackIn,
    eCurveTypeBackOut,
    eCurveTypeLinear,
    eCurveTypeUniform
};

inline static void
getCurveValue(const CurveTypeEnum curveType,
    double* x1,
    double* y1,
    double* x2,
    double* y2)
{
    switch (curveType) {
    case eCurveTypeDefault:
        *x1 = 0.50; *y1 = 0.00; *x2 = 0.50; *y2 = 1.00; break;
    case eCurveTypeLinear:
        *x1 = 0.00; *y1 = 0.00; *x2 = 1.00; *y2 = 1.00; break;
    case eCurveTypeEase:
        *x1 = 0.42; *y1 = 0.00; *x2 = 0.58; *y2 = 1.00; break;
    case eCurveTypeEaseIn:
        *x1 = 0.42; *y1 = 0.00; *x2 = 1.00; *y2 = 1.00; break;
    case eCurveTypeEaseOut:
        *x1 = 0.00; *y1 = 0.00; *x2 = 0.58; *y2 = 1.00; break;
    case eCurveTypeQuad:
        *x1 = 0.48; *y1 = 0.04; *x2 = 0.52; *y2 = 0.96; break;
    case eCurveTypeQuadIn:
        *x1 = 0.26; *y1 = 0.00; *x2 = 0.60; *y2 = 0.20; break;
    case eCurveTypeQuadOut:
        *x1 = 0.40; *y1 = 0.80; *x2 = 0.74; *y2 = 1.00; break;
    case eCurveTypeCubic:
        *x1 = 0.66; *y1 = 0.00; *x2 = 0.34; *y2 = 1.00; break;
    case eCurveTypeCubicIn:
        *x1 = 0.40; *y1 = 0.00; *x2 = 0.68; *y2 = 0.06; break;
    case eCurveTypeCubicOut:
        *x1 = 0.32; *y1 = 0.94; *x2 = 0.60; *y2 = 1.00; break;
    case eCurveTypeQuart:
        *x1 = 0.76; *y1 = 0.00; *x2 = 0.24; *y2 = 1.00; break;
    case eCurveTypeQuartIn:
        *x1 = 0.52; *y1 = 0.00; *x2 = 0.74; *y2 = 0.00; break;
    case eCurveTypeQuartOut:
        *x1 = 0.26; *y1 = 1.00; *x2 = 0.48; *y2 = 1.00; break;
    case eCurveTypeQuint:
        *x1 = 0.84; *y1 = 0.00; *x2 = 0.16; *y2 = 1.00; break;
    case eCurveTypeQuintIn:
        *x1 = 0.64; *y1 = 0.00; *x2 = 0.78; *y2 = 0.00; break;
    case eCurveTypeQuintOut:
        *x1 = 0.22; *y1 = 1.00; *x2 = 0.36; *y2 = 1.00; break;
    case eCurveTypeExpo:
        *x1 = 0.90; *y1 = 0.00; *x2 = 0.10; *y2 = 1.00; break;
    case eCurveTypeExpoIn:
        *x1 = 0.66; *y1 = 0.00; *x2 = 0.86; *y2 = 0.00; break;
    case eCurveTypeExpoOut:
        *x1 = 0.14; *y1 = 1.00; *x2 = 0.34; *y2 = 1.00; break;
    case eCurveTypeCirc:
        *x1 = 0.88; *y1 = 0.14; *x2 = 0.12; *y2 = 0.86; break;
    case eCurveTypeCircIn:
        *x1 = 0.54; *y1 = 0.00; *x2 = 1.00; *y2 = 0.44; break;
    case eCurveTypeCircOut:
        *x1 = 0.00; *y1 = 0.56; *x2 = 0.46; *y2 = 1.00; break;
    case eCurveTypeBack:
        *x1 = 0.68; *y1 = -0.55; *x2 = 0.27; *y2 = 1.55; break;
    case eCurveTypeBackIn:
        *x1 = 0.60; *y1 = -0.28; *x2 = 0.73; *y2 = 0.04; break;
    case eCurveTypeBackOut:
        *x1 = 0.17; *y1 = 0.89; *x2 = 0.32; *y2 = 1.27; break;
    default:
        break;
    }
}

enum FrequencyUnitEnum
{
    eFrequencyUnitHz,
    eFrequencyUnitBPM
};

enum BeatTypeEnum
{
    eBPMTypeQuadrupleWholes,
    eBPMTypeBreve,
    eBPMTypeWhole,
    eBPMTypeHalf,
    eBPMTypeQuarter,
    eBPMTypeEighth,
    eBPMTypeSixteenth,
    eBPMTypeThirtySecond,
    eBPMTypeHalfTriplet,
    eBPMTypeQuarterTriplet,
    eBPMTypeEighthTriplet,
    eBPMTypeSixteenthTriplet,
    eBPMTypeThirtySecondTriplet
};

enum RoundTripEnum
{
    eRoundTripNone,
    eRoundTripOnly,
    eRoundTripVertical,
    eRoundTripHorizontal,
    eRoundTripBoth
};

inline static void
convertFrequency(FrequencyUnitEnum unitInput, FrequencyUnitEnum unitOutput, const BeatTypeEnum beatType, double* f)
{
    if (unitInput == unitOutput) {
        return;
    }

    double n = 1. / 60;
    switch (beatType) {
    case eBPMTypeQuadrupleWholes:
        n *= 1. / 16; break;
    case eBPMTypeBreve:
        n *= 1. / 8; break;
    case eBPMTypeWhole:
        n *= 1. / 4; break;
    case eBPMTypeHalf:
        n *= 1. / 2; break;
    case eBPMTypeQuarter:
        n *= 1. / 1; break;
    case eBPMTypeEighth:
        n *= 2. / 1; break;
    case eBPMTypeSixteenth:
        n *= 4. / 1; break;
    case eBPMTypeThirtySecond:
        n *= 8. / 1; break;
    case eBPMTypeHalfTriplet:
        n *= 1. / 6; break;
    case eBPMTypeQuarterTriplet:
        n *= 1. / 3; break;
    case eBPMTypeEighthTriplet:
        n *= 2. / 3; break;
    case eBPMTypeSixteenthTriplet:
        n *= 4. / 3; break;
    case eBPMTypeThirtySecondTriplet:
        n *= 8. / 3; break;
    default:
        break;
    }

    if (unitInput == eFrequencyUnitBPM && unitOutput == eFrequencyUnitHz) { *f *= n; } else
    if (unitInput == eFrequencyUnitHz && unitOutput == eFrequencyUnitBPM) { *f /= n; }
}

////////////////////////////////////////////////////////////////////////////////
/** @brief The plugin that does our work */
class TransformPlugin
    : public Transform3x3Plugin
{
public:
    /** @brief ctor */
    TransformPlugin(OfxImageEffectHandle handle,
                    bool masked,
                    bool isDirBlur)
        : Transform3x3Plugin(handle, masked, isDirBlur ? eTransform3x3ParamsTypeDirBlur : eTransform3x3ParamsTypeMotionBlur)
        , _translate(NULL)
        , _periodicRadius(NULL)
        , _periodicRotate(NULL)
        , _periodicDeform(NULL)
        , _periodicBend(NULL)
        , _periodicN(NULL)
        , _periodicInterval(NULL)
        , _periodicCurve(NULL)
        , _periodicBezierP1(NULL)
        , _periodicBezierP2(NULL)
        , _periodicSymmetry(NULL)
        , _periodicFrequency(NULL)
        , _periodicFrequencyUnit(NULL)
        , _periodicFrequencyBeat(NULL)
        , _periodicAutorotate(NULL)
        , _periodicScale(NULL)
        , _periodicScaleStep(NULL)
        , _periodicOffset(NULL)
        , _periodicSkip(NULL)
        , _functionFrequency(NULL)
        , _functionExpression(NULL)
        , _functionDomain(NULL)
        , _functionUnit(NULL)
        , _functionRoundTrip(NULL)
        , _functionRotate(NULL)
        , _functionCurve(NULL)
        , _functionBezierP1(NULL)
        , _functionBezierP2(NULL)
        , _functionSymmetry(NULL)
        , _rotate(NULL)
        , _faceToCenter(NULL)
        , _scale(NULL)
        , _flop(NULL)
        , _flip(NULL)
        , _scaleUniform(NULL)
        , _skewX(NULL)
        , _skewY(NULL)
        , _skewOrder(NULL)
        , _transformAmount(NULL)
        , _center(NULL)
        , _interactive(NULL)
        , _srcClipChanged(NULL)
    {
        // NON-GENERIC
        if (isDirBlur) {
            _dirBlurAmount = fetchDoubleParam(kParamTransform3x3DirBlurAmount);
            _dirBlurCentered = fetchBooleanParam(kParamTransform3x3DirBlurCentered);
            _dirBlurFading = fetchDoubleParam(kParamTransform3x3DirBlurFading);
        }

        _translate = fetchDouble2DParam(kParamTransformTranslateOld);
        _periodicRadius = fetchDoubleParam(kParamTransformPeriodicRadius);
        _periodicRotate = fetchDoubleParam(kParamTransformPeriodicRotate);
        _periodicDeform = fetchDoubleParam(kParamTransformPeriodicDeform);
        _periodicBend = fetchDoubleParam(kParamTransformPeriodicBend);
        _periodicN = fetchIntParam(kParamTransformPeriodicN);
        _periodicInterval = fetchIntParam(kParamTransformPeriodicInterval);
        _periodicCurve = fetchChoiceParam(kParamTransformPeriodicCurve);
        _periodicBezierP1 = fetchDouble2DParam(kParamTransformPeriodicBezierP1);
        _periodicBezierP2 = fetchDouble2DParam(kParamTransformPeriodicBezierP2);
        _periodicSymmetry = fetchBooleanParam(kParamTransformPeriodicSymmetry);
        _periodicFrequency = fetchDoubleParam(kParamTransformPeriodicFrequency);
        _periodicFrequencyUnit = fetchChoiceParam(kParamTransformPeriodicFrequencyUnit);
        _periodicFrequencyBeat = fetchChoiceParam(kParamTransformPeriodicFrequencyBeat);
        _periodicAutorotate = fetchDoubleParam(kParamTransformPeriodicAutorotate);
        _periodicScale = fetchDoubleParam(kParamTransformPeriodicScale);
        _periodicScaleStep = fetchDoubleParam(kParamTransformPeriodicScaleStep);
        _periodicOffset = fetchDoubleParam(kParamTransformPeriodicOffset);
        _periodicSkip = fetchDoubleParam(kParamTransformPeriodicSkip);
        _functionFrequency = fetchDoubleParam(kParamTransformFunctionFrequency);
        _functionExpression = fetchStringParam(kParamTransformFunctionExpression);
        _functionDomain = fetchDouble2DParam(kParamTransformFunctionDomain);
        _functionUnit = fetchDoubleParam(kParamTransformFunctionUnit);
        _functionRoundTrip = fetchChoiceParam(kParamTransformFunctionRoundTrip);
        _functionRotate = fetchDoubleParam(kParamTransformFunctionRotate);
        _functionCurve = fetchChoiceParam(kParamTransformFunctionCurve);
        _functionBezierP1 = fetchDouble2DParam(kParamTransformFunctionBezierP1);
        _functionBezierP2 = fetchDouble2DParam(kParamTransformFunctionBezierP2);
        _functionSymmetry = fetchBooleanParam(kParamTransformFunctionSymmetry);
        _rotate = fetchDoubleParam(kParamTransformRotateOld);
        _faceToCenter = fetchBooleanParam(kParamTransformFaceToCenter);
        _scale = fetchDouble2DParam(kParamTransformScaleOld);
        _flop = fetchBooleanParam(kParamTransformFlop);
        _flip = fetchBooleanParam(kParamTransformFlip);
        _scaleUniform = fetchBooleanParam(kParamTransformScaleUniformOld);
        _skewX = fetchDoubleParam(kParamTransformSkewXOld);
        _skewY = fetchDoubleParam(kParamTransformSkewYOld);
        _skewOrder = fetchChoiceParam(kParamTransformSkewOrderOld);
        if (!isDirBlur) {
            _transformAmount = fetchDoubleParam(kParamTransformAmount);
        }
        _center = fetchDouble2DParam(kParamTransformCenterOld);
        _centerChanged = fetchBooleanParam(kParamTransformCenterChanged);
        _interactive = fetchBooleanParam(kParamTransformInteractiveOld);
        assert(_translate && _periodicRadius && _periodicRotate && _periodicDeform && _periodicBend && _periodicN && _periodicInterval &&  _periodicCurve && _periodicBezierP1 && _periodicBezierP2 && _periodicSymmetry && _periodicFrequency && _periodicFrequencyUnit && _periodicAutorotate && _periodicScale && _periodicScaleStep && _periodicOffset && _periodicSkip && _functionFrequency && _functionExpression && _functionDomain && _functionUnit && _functionRoundTrip && _functionRotate && _functionCurve && _functionBezierP1 && _functionBezierP2 && _functionSymmetry && _rotate && _faceToCenter && _scale && _scaleUniform && _flop && _flip && _skewX && _skewY && _skewOrder && _center && _interactive);
        _srcClipChanged = fetchBooleanParam(kParamSrcClipChanged);
        assert(_srcClipChanged);
        // On Natron, hide the uniform parameter if it is false and not animated,
        // since uniform scaling is easy through Natron's GUI.
        // The parameter is kept for backward compatibility.
        // Fixes https://github.com/MrKepzie/Natron/issues/1204
        if ( getImageEffectHostDescription()->isNatron &&
             !_scaleUniform->getValue() &&
             ( _scaleUniform->getNumKeys() == 0) ) {
            _scaleUniform->setIsSecretAndDisabled(true);
        }
    }

private:
    virtual bool isIdentity(double time) OVERRIDE FINAL;
    virtual bool getInverseTransformCanonical(double time, int view, double amount, bool invert, Matrix3x3* invtransform) const OVERRIDE FINAL;

    void resetCenter(double time);

    virtual void changedParam(const InstanceChangedArgs &args, const std::string &paramName) OVERRIDE FINAL;
    virtual void getClipPreferences(ClipPreferencesSetter& clipPreferences) OVERRIDE FINAL;

    /** @brief called when a clip has just been changed in some way (a rewire maybe) */
    virtual void changedClip(const InstanceChangedArgs &args, const std::string &clipName) OVERRIDE FINAL;

    // NON-GENERIC
    Double2DParam* _translate;
    DoubleParam* _periodicRadius;
    DoubleParam* _periodicRotate;
    DoubleParam* _periodicDeform;
    DoubleParam* _periodicBend;
    IntParam* _periodicN;
    IntParam* _periodicInterval;
    ChoiceParam* _periodicCurve;
    Double2DParam* _periodicBezierP1;
    Double2DParam* _periodicBezierP2;
    BooleanParam* _periodicSymmetry;
    DoubleParam* _periodicFrequency;
    ChoiceParam* _periodicFrequencyUnit;
    ChoiceParam* _periodicFrequencyBeat;
    DoubleParam* _periodicAutorotate;
    DoubleParam* _periodicScale;
    DoubleParam* _periodicScaleStep;
    DoubleParam* _periodicOffset;
    DoubleParam* _periodicSkip;
    DoubleParam* _functionFrequency;
    StringParam* _functionExpression;
    Double2DParam* _functionDomain;
    DoubleParam* _functionUnit;
    ChoiceParam* _functionRoundTrip;
    DoubleParam* _functionRotate;
    ChoiceParam* _functionCurve;
    Double2DParam* _functionBezierP1;
    Double2DParam* _functionBezierP2;
    BooleanParam* _functionSymmetry;
    DoubleParam* _rotate;
    BooleanParam* _faceToCenter;
    Double2DParam* _scale;
    BooleanParam* _flop;
    BooleanParam* _flip;
    BooleanParam* _scaleUniform;
    DoubleParam* _skewX;
    DoubleParam* _skewY;
    ChoiceParam* _skewOrder;
    DoubleParam* _transformAmount;
    Double2DParam* _center;
    BooleanParam* _centerChanged;
    BooleanParam* _interactive;
    BooleanParam* _srcClipChanged; // set to true the first time the user connects src
};

// overridden is identity
bool
TransformPlugin::isIdentity(double time)
{
    // NON-GENERIC
    if (_paramsType != eTransform3x3ParamsTypeDirBlur) {
        double amount = _transformAmount->getValueAtTime(time);
        if (amount == 0.) {
            return true;
        }
    }

    OfxPointD scaleParam = { 1., 1. };

    if (_scale) {
        _scale->getValueAtTime(time, scaleParam.x, scaleParam.y);
    }
    bool scaleUniform = false;
    if (_scaleUniform) {
        _scaleUniform->getValueAtTime(time, scaleUniform);
    }
    bool flop = false;
    if (_flop) {
        _flop->getValueAtTime(time, flop);
    }
    bool flip = false;
    if (_flip) {
        _flip->getValueAtTime(time, flip);
    }
    OfxPointD scale = { 1., 1. };
    ofxsTransformGetScale(scaleParam, scaleUniform, flop, flip, &scale);
    OfxPointD translate = { 0., 0. };
    if (_translate) {
        _translate->getValueAtTime(time, translate.x, translate.y);
    }
    double periodicRadius = 0.;
    if (_periodicRadius) {
        periodicRadius = _periodicRadius->getValueAtTime(time);
    }
    double periodicRotate = 0.;
    if (_periodicRotate) {
        periodicRotate = _periodicRotate->getValueAtTime(time);
    }
    double periodicDeform = 1.;
    if (_periodicDeform) {
        periodicDeform = _periodicDeform->getValueAtTime(time);
    }
    double periodicBend = 1.;
    if (_periodicBend) {
        periodicBend = _periodicBend->getValueAtTime(time);
    }
    int periodicN = 1;
    if (_periodicN) {
        periodicN = _periodicN->getValueAtTime(time);
    }
    int periodicInterval = 1;
    if (_periodicInterval) {
        periodicInterval = _periodicInterval->getValueAtTime(time);
    }
    OfxPointD periodicBezierP1 = { 0., 0. };
    if (_periodicBezierP1) {
        _periodicBezierP1->getValueAtTime(time, periodicBezierP1.x, periodicBezierP1.y);
    }
    OfxPointD periodicBezierP2 = { 1., 1. };
    if (_periodicBezierP2) {
        _periodicBezierP2->getValueAtTime(time, periodicBezierP2.x, periodicBezierP2.y);
    }
    bool periodicSymmetry = false;
    if (_periodicSymmetry) {
        periodicSymmetry = _periodicSymmetry->getValueAtTime(time);
    }
    double periodicFrequency = 0.;
    if (_periodicFrequency) {
        periodicFrequency = _periodicFrequency->getValueAtTime(time);
    }
    double periodicAutorotate = 0.;
    if (_periodicAutorotate) {
        periodicAutorotate = _periodicAutorotate->getValueAtTime(time);
    }
    double periodicScale = 1.;
    if (_periodicScale) {
        periodicScale = _periodicScale->getValueAtTime(time);
    }
    double periodicScaleStep = 0.;
    if (_periodicScaleStep) {
        periodicScaleStep = _periodicScaleStep->getValueAtTime(time);
    }
    double periodicOffset = 0.;
    if (_periodicOffset) {
        periodicOffset = _periodicOffset->getValueAtTime(time);
    }
    double periodicSkip = 0.;
    if (_periodicSkip) {
        periodicSkip = _periodicSkip->getValueAtTime(time);
    }
    double functionFrequency = 1.;
    if (_functionFrequency) {
        if (_functionFrequency->getIsAnimating()) {
            functionFrequency = 0;
            for (int i = 0; i <= time; i++) { functionFrequency += _functionFrequency->getValueAtTime(i); }
            functionFrequency /= std::floor(time) + 1;
        } else { functionFrequency = _functionFrequency->getValueAtTime(time); }
    }
    std::string functionExpression = "";
    if (_functionExpression) {
        functionExpression = _functionExpression->getValueAtTime(time);
    }
    OfxPointD functionDomain = { -1., 1. };
    if (_functionDomain) {
        _functionDomain->getValueAtTime(time, functionDomain.x, functionDomain.y);
    }
    double functionUnit = 0.5;
    if (_functionUnit) {
        functionUnit = _functionUnit->getValueAtTime(time);
    }
    double functionRotate = 0.;
    if (_functionRotate) {
        functionRotate = _functionRotate->getValueAtTime(time);
    }
    double rotate = 0.;
    if (_rotate) {
        _rotate->getValueAtTime(time, rotate);
    }
    bool faceToCenter = false;
    if (_faceToCenter) {
        _faceToCenter->getValueAtTime(time, faceToCenter);
    }
    double skewX = 0.;
    if (_skewX) {
        _skewX->getValueAtTime(time, skewX);
    }
    double skewY = 0.;
    if (_skewY) {
        _skewY->getValueAtTime(time, skewY);
    }

    if ( (scale.x == 1.) && (scale.y == 1.) && (translate.x == 0.) && (translate.y == 0.) && (periodicFrequency == 0. || periodicAutorotate == 0.) && (periodicScale == 1.) && (periodicRadius == 0.) && (functionExpression == ""  || functionUnit == 0. || functionFrequency == 0.) && (rotate == 0.) && (skewX == 0.) && (skewY == 0.)) {
        return true;
    }

    return false;
}

void
TransformPlugin::getClipPreferences(ClipPreferencesSetter& clipPreferences)
{
    clipPreferences.setOutputFrameVarying(true);
}

static double bezierX(double t,
                      OfxPointD p1,
                      OfxPointD p2) {
    return 3 * (1 - t) * (1 - t) * t * p1.x + 3 * (1 - t) * t * t * p2.x + t * t * t;
}

static double bezierY(double t,
                      OfxPointD p1,
                      OfxPointD p2) {
    return 3 * (1 - t) * (1 - t) * t * p1.y + 3 * (1 - t) * t * t * p2.y + t * t * t;
}

static double findTForX(double m,
                        OfxPointD p1,
                        OfxPointD p2) {
    double t_low = 0.0;
    double t_high = 1.0;
    double t_mid = (t_low + t_high) / 2.0;

    while ((t_high - t_low) / 2.0 > 1e-6) {
        if (bezierX(t_mid, p1, p2) < m) {
            t_low = t_mid;
        }
        else {
            t_high = t_mid;
        }
        t_mid = (t_low + t_high) / 2.0;
    }

    return t_mid;
}

static double findBezierY(double m,
                          OfxPointD p1,
                          OfxPointD p2) {
    double t = findTForX(m, p1, p2);
    return bezierY(t, p1, p2);
}

bool
TransformPlugin::getInverseTransformCanonical(double time,
                                              int /*view*/,
                                              double amount,
                                              bool invert,
                                              Matrix3x3* invtransform) const
{
    // NON-GENERIC
    OfxPointD center = { 0., 0. };
    if (_center) {
        _center->getValueAtTime(time, center.x, center.y);
    }
    OfxPointD translate = { 0., 0. };
    if (_translate) {
        _translate->getValueAtTime(time, translate.x, translate.y);
    }
    double periodicRadius = 0.;
    if (_periodicRadius) {
        periodicRadius = _periodicRadius->getValueAtTime(time);
    }
    double periodicRotate = 0.;
    if (_periodicRotate) {
        periodicRotate = _periodicRotate->getValueAtTime(time);
    }
    double periodicDeform = 1.;
    if (_periodicDeform) {
        periodicDeform = _periodicDeform->getValueAtTime(time);
    }
    double periodicBend = 0.;
    if (_periodicBend) {
        periodicBend = _periodicBend->getValueAtTime(time);
    }
    int periodicN = 1;
    if (_periodicN) {
        periodicN = _periodicN->getValueAtTime(time);
    }
    int periodicInterval = 1;
    if (_periodicInterval) {
        periodicInterval = _periodicInterval->getValueAtTime(time);
    }
    CurveTypeEnum periodicCurve = eCurveTypeDefault;
    if (_periodicCurve) {
        periodicCurve = (CurveTypeEnum)_periodicCurve->getValueAtTime(time);
    }
    FrequencyUnitEnum periodicFrequencyUnit = eFrequencyUnitHz;
    if (_periodicFrequencyUnit) {
        periodicFrequencyUnit = (FrequencyUnitEnum)_periodicFrequencyUnit->getValue();
    }
    BeatTypeEnum periodicFrequencyBeat = eBPMTypeQuarter;
    if (_periodicFrequencyBeat) {
        periodicFrequencyBeat = (BeatTypeEnum)_periodicFrequencyBeat->getValueAtTime(time);
    }
    OfxPointD periodicBezierP1 = { 0., 0. };
    if (_periodicBezierP1) {
        _periodicBezierP1->getValueAtTime(time, periodicBezierP1.x, periodicBezierP1.y);
    }
    OfxPointD periodicBezierP2 = { 1., 1. };
    if (_periodicBezierP2) {
        _periodicBezierP2->getValueAtTime(time, periodicBezierP2.x, periodicBezierP2.y);
    }
    bool periodicSymmetry = false;
    if (_periodicSymmetry) {
        periodicSymmetry = _periodicSymmetry->getValueAtTime(time);
    }
    double periodicFrequency = 0.;
    if (_periodicFrequency) {
        if (_periodicFrequency->getIsAnimating() || _periodicFrequencyBeat->getIsAnimating()) {
            for (int i = 0; i <= time; i++) {
                double f = _periodicFrequency->getValueAtTime(i);
                convertFrequency(periodicFrequencyUnit, eFrequencyUnitHz, (BeatTypeEnum)_periodicFrequencyBeat->getValueAtTime(i), &f);
                periodicFrequency += f;
            }
            periodicFrequency /= std::floor(time) + 1;
            periodicFrequencyUnit = eFrequencyUnitHz;
        } else { periodicFrequency = _periodicFrequency->getValueAtTime(time); }
    }
    double periodicAutorotate = 0.;
    if (_periodicAutorotate) {
        if (_periodicAutorotate->getIsAnimating()) {
            for (int i = 0; i <= time; i++) { periodicAutorotate += _periodicAutorotate->getValueAtTime(i); }
            periodicAutorotate /= std::floor(time) + 1;
        } else { periodicAutorotate = _periodicAutorotate->getValueAtTime(time); }
    }
    double periodicScale = 1.;
    if (_periodicScale) {
        periodicScale = _periodicScale->getValueAtTime(time);
    }
    double periodicScaleStep = 0.;
    if (_periodicScaleStep) {
        periodicScaleStep = _periodicScaleStep->getValueAtTime(time);
    }
    double periodicOffset = 0.;
    if (_periodicOffset) {
        periodicOffset = _periodicOffset->getValueAtTime(time);
    }
    double periodicSkip = 0.;
    if (_periodicSkip) {
        periodicSkip = _periodicSkip->getValueAtTime(time);
    }
    double functionFrequency = 1.;
    if (_functionFrequency) {
        functionFrequency = _functionFrequency->getValueAtTime(time);
    }
    std::string functionExpression = "";
    if (_functionExpression) {
        functionExpression = _functionExpression->getValueAtTime(time);
    }
    OfxPointD functionDomain = { -1., 1. };
    if (_functionDomain) {
        _functionDomain->getValueAtTime(time, functionDomain.x, functionDomain.y);
    }
    double functionUnit = 0.5;
    if (_functionUnit) {
        functionUnit = _functionUnit->getValueAtTime(time);
    }
    RoundTripEnum functionRoundTrip = eRoundTripNone;
    if (_functionRoundTrip) {
        functionRoundTrip = (RoundTripEnum)_functionRoundTrip->getValueAtTime(time);
    }
    double functionRotate = 0.;
    if (_functionRotate) {
        functionRotate = _functionRotate->getValueAtTime(time);
    }
    CurveTypeEnum functionCurve = eCurveTypeLinear;
    if (_functionCurve) {
        functionCurve = (CurveTypeEnum)_functionCurve->getValueAtTime(time);
    }
    OfxPointD functionBezierP1 = { 0., 0. };
    if (_functionBezierP1) {
        _functionBezierP1->getValueAtTime(time, functionBezierP1.x, functionBezierP1.y);
    }
    OfxPointD functionBezierP2 = { 1., 1. };
    if (_functionBezierP2) {
        _functionBezierP2->getValueAtTime(time, functionBezierP2.x, functionBezierP2.y);
    }
    bool functionSymmetry = false;
    if (_functionSymmetry) {
        functionSymmetry = _functionSymmetry->getValueAtTime(time);
    }
    OfxPointD scaleParam = { 1., 1. };
    if (_scale) {
        _scale->getValueAtTime(time, scaleParam.x, scaleParam.y);
    }
    bool scaleUniform = false;
    if (_scaleUniform) {
        scaleUniform = _scaleUniform->getValueAtTime(time);
    }
    bool flop = false;
    if (_flop) {
        flop = _flop->getValueAtTime(time);
    }
    bool flip = false;
    if (_flip) {
        flip = _flip->getValueAtTime(time);
    }
    double rotate = 0.;
    if (_rotate) {
        rotate = _rotate->getValueAtTime(time);
    }
    bool faceToCenter = false;
    if (_faceToCenter) {
        faceToCenter = _faceToCenter->getValueAtTime(time);
    }
    double skewX = 0.;
    if (_skewX) {
        skewX = _skewX->getValueAtTime(time);
    }
    double skewY = 0.;
    if (_skewY) {
        skewY = _skewY->getValueAtTime(time);
    }
    int skewOrder = 0;
    if (_skewOrder) {
        skewOrder = _skewOrder->getValueAtTime(time);
    }
    if (_transformAmount) {
        amount *= _transformAmount->getValueAtTime(time);
    }

    OfxPointD scale = { 1., 1. };
    ofxsTransformGetScale(scaleParam, scaleUniform, flop, flip, &scale);

    if (amount != 1.) {
        translate.x *= amount;
        translate.y *= amount;
        if (scale.x <= 0. || amount <= 0.) {
            // linear interpolation
            scale.x = 1. + (scale.x - 1.) * amount;
        } else {
            // geometric interpolation
            scale.x = std::pow(scale.x, amount);
        }
        if (scale.y <= 0 || amount <= 0.) {
            // linear interpolation
            scale.y = 1. + (scale.y - 1.) * amount;
        } else {
            // geometric interpolation
            scale.y = std::pow(scale.y, amount);
        }
        periodicRadius *= amount;
        rotate *= amount;
        skewX *= amount;
        skewY *= amount;
    }


    OfxPointD size = getProjectSize();
    OfxPointD offset = getProjectOffset();
    //OfxRectD rod = _srcClip->getRegionOfDefinition(time);
    //size.x = std::abs(rod.x2 - rod.x1);
    //size.y = std::abs(rod.y2 - rod.y1);
    translate.x *= size.x;
    translate.y *= size.y;
    center = { center.x * size.x + offset.x, center.y * size.y + offset.y };
    periodicRadius *= std::max(size.x, size.y) / 2;

    double functionOffset = periodicOffset;
    periodicRotate = ofxsToRadians(periodicRotate);
    if (periodicFrequency == 0. || (periodicInterval % periodicN == 0 && periodicN != 1 && periodicAutorotate == 0. && periodicScale == 1) && (functionExpression == "" || functionFrequency == 0. || functionUnit == 0.)) {
        translate.x += periodicRadius * std::sin(periodicRotate) * periodicDeform;
        translate.y += periodicRadius * std::cos(periodicRotate);
    } else {
        convertFrequency(periodicFrequencyUnit, eFrequencyUnitHz, periodicFrequencyBeat, &periodicFrequency);
        periodicOffset *= periodicN == 1 ? periodicInterval : periodicN;
        periodicOffset += periodicFrequency * time / getFrameRate();
        functionOffset = periodicOffset;
        if (periodicSkip != 0.) {
            periodicOffset += std::floor(periodicOffset / periodicSkip) * periodicSkip;
        }

        double b = periodicOffset - std::floor(periodicOffset);
        if (periodicCurve != eCurveTypeCustom) {
            getCurveValue(periodicCurve, &periodicBezierP1.x, &periodicBezierP1.y, &periodicBezierP2.x, &periodicBezierP2.y);
        }
        if (periodicBezierP1.x != periodicBezierP1.y || periodicBezierP2.x != periodicBezierP2.y) {
            periodicOffset -= b;
            bool isReversed = periodicSymmetry && (int)periodicOffset % 2 != 0;
            if (isReversed) {
                b = 1 - b;
            }
            b = findBezierY(b, periodicBezierP1, periodicBezierP2);
            if (isReversed) {
                b = 1 - b;
            }
            periodicOffset += std::floor(b);
            b = b - std::floor(b);
            periodicOffset += b;
        }

        double periodicRot = 2 * M_PI * periodicOffset / periodicInterval;
        OfxPointD p = { std::sin(periodicRot), std::cos(periodicRot) };
        if (periodicN > 1) {
            double angleA = 2 * M_PI * periodicInterval / periodicN * std::floor(periodicOffset);
            double angleB = 2 * M_PI * periodicInterval / periodicN * (std::floor(periodicOffset) + 1);
            OfxPointD p_start = { std::sin(angleA), std::cos(angleA) };
            OfxPointD p_end  = { std::sin(angleB), std::cos(angleB) };
            double angleC = - std::atan2(p_start.x - p_end.x, p_start.y - p_end.y);
            if (periodicInterval % periodicN == 0) {
                periodicBend = 0;
            }
            OfxPointD p_mid = { periodicN == 2 ? M_E * periodicBend : ((p_start.x + p_end.x) / 2 + std::cos(angleC) * periodicBend) * std::pow(M_E, periodicBend),
                                                                      ((p_start.y + p_end.y) / 2 + std::sin(angleC) * periodicBend) * std::pow(M_E, periodicBend) };
            p = { p_start.x * (1 - b) * (1 - b) + p_mid.x * (1 - b) * b * 2 + p_end.x * b * b,
                  p_start.y * (1 - b) * (1 - b) + p_mid.y * (1 - b) * b * 2 + p_end.y * b * b };
        }
        p.x *= periodicN != 2 ? periodicDeform : 1;
        p = { p.x * std::cos(periodicRotate) + p.y * std::sin(periodicRotate),
              p.y * std::cos(periodicRotate) - p.x * std::sin(periodicRotate) };
        translate.x += periodicRadius * p.x * (periodicN == 2 ? periodicDeform : 1);
        translate.y += periodicRadius * p.y;

        if (periodicScaleStep != 0. && periodicFrequency != 0.) {
            double f = periodicOffset * 2 / periodicScaleStep;
            if (_periodicScaleStep->getIsAnimating()) {
                double ff = 0;
                for (int i = 0; i <= time; i++) {
                    double s = _periodicScaleStep->getValueAtTime(i);
                    ff += s != 0 ? 1 / s : 0;
                }
                f = periodicOffset * 2 * ff / (std::floor(time) + 1);
            }
            periodicScale = (periodicScale - 1) * std::abs(f - std::floor(f) + (int)f % 2 - 1) + 1;
        }

        if (functionExpression != "" && functionFrequency != 0. && functionUnit != 0.) {
            functionUnit *= size.x;
            double l = functionDomain.y - functionDomain.x;
            double dis = 0;

            if (l == 0) {
                functionOffset = functionDomain.x;
            } else {
                functionOffset *= functionFrequency;

                if (functionCurve == eCurveTypeUniform) {
                    if (periodicCurve != eCurveTypeCustom) {
                        getCurveValue(periodicCurve, &functionBezierP1.x, &functionBezierP1.y, &functionBezierP2.x, &functionBezierP2.y);
                    } else {
                        functionBezierP1 = periodicBezierP1;
                        functionBezierP2 = periodicBezierP2;
                    }
                } else if (functionCurve != eCurveTypeCustom) {
                    getCurveValue(functionCurve, &functionBezierP1.x, &functionBezierP1.y, &functionBezierP2.x, &functionBezierP2.y);
                }
                if (functionBezierP1.x != functionBezierP1.y || functionBezierP2.x != functionBezierP2.y) {
                    b = functionOffset - std::floor(functionOffset);
                    functionOffset -= b;
                    bool isReversed = (functionCurve == eCurveTypeUniform ? periodicSymmetry : functionSymmetry) && (int)functionOffset % 2 != 0;
                    if (isReversed) {
                        b = 1 - b;
                    }
                    b = findBezierY(b, functionBezierP1, functionBezierP2);
                    if (isReversed) {
                        b = 1 - b;
                    }
                    functionOffset += b;
                }

                int sgn = functionFrequency < 0 ? -1 : 1;
                dis = ((int)functionOffset / 2 * 2 + sgn - functionOffset) * l;
                
                functionOffset = functionRoundTrip == eRoundTripNone ? (functionOffset - std::floor(functionOffset)) * l + functionDomain.x
                                                                     : (sgn < 0 ? std::min(functionDomain.x, functionDomain.y) + std::abs(dis) : std::max(functionDomain.x, functionDomain.y) - std::abs(dis));
            }
            typedef exprtk::symbol_table<double> symbol_table_t;
            typedef exprtk::expression<double>   expression_t;
            typedef exprtk::parser<double>       parser_t;
            symbol_table_t symbol_table;
            expression_t   expression;
            parser_t       parser;
            symbol_table_t glbl_const_symbol_table;
            glbl_const_symbol_table.add_constants();
            symbol_table.add_constant("e", exprtk::details::numeric::constant::e);
            expression.register_symbol_table(glbl_const_symbol_table);
            symbol_table.add_variable("x", functionOffset);
            expression.register_symbol_table(symbol_table);
            parser.compile(functionExpression, expression);
            p = { functionOffset  *  ((functionRoundTrip == eRoundTripHorizontal || functionRoundTrip == eRoundTripBoth) && dis < 0 ? -1 : 1),
                  expression.value() * ((functionRoundTrip == eRoundTripVertical || functionRoundTrip == eRoundTripBoth) && dis < 0 ? -1 : 1) };
            functionRotate = ofxsToRadians(functionRotate);
            p = { p.x * std::cos(functionRotate) + p.y * std::sin(functionRotate),
                  p.y * std::cos(functionRotate) - p.x * std::sin(functionRotate) };
            translate.x += functionUnit * p.x;
            translate.y += functionUnit * p.y;
        }
    }

    scale.x *= periodicScale;
    scale.y *= periodicScale;

    double rot = ofxsToRadians(rotate) - 2 * M_PI * periodicOffset / periodicInterval / periodicN * periodicAutorotate - (faceToCenter ? std::atan2(translate.x, translate.y) : 0);

    if (!invert) {
        *invtransform = ofxsMatInverseTransformCanonical(translate.x, translate.y, scale.x, scale.y, skewX, skewY, (bool)skewOrder, rot, center.x, center.y);
    } else {
        *invtransform = ofxsMatTransformCanonical(translate.x, translate.y, scale.x, scale.y, skewX, skewY, (bool)skewOrder, rot, center.x, center.y);
    }

    return true;
} // TransformPlugin::getInverseTransformCanonical

void
TransformPlugin::resetCenter(double time)
{
    if (!_srcClip || !_srcClip->isConnected()) {
        return;
    }
    OfxRectD rod = _srcClip->getRegionOfDefinition(time);
    if ( (rod.x1 <= kOfxFlagInfiniteMin) || (kOfxFlagInfiniteMax <= rod.x2) ||
         ( rod.y1 <= kOfxFlagInfiniteMin) || ( kOfxFlagInfiniteMax <= rod.y2) ) {
        return;
    }
    if ( Coords::rectIsEmpty(rod) ) {
        // default to project window
        OfxPointD offset = getProjectOffset();
        OfxPointD size = getProjectSize();
        rod.x1 = offset.x;
        rod.x2 = offset.x + size.x;
        rod.y1 = offset.y;
        rod.y2 = offset.y + size.y;
    }
    double currentRotation = 0.;
    if (_rotate) {
        _rotate->getValueAtTime(time, currentRotation);
    }
    double rot = ofxsToRadians(currentRotation);
    double skewX = 0.;
    double skewY = 0.;
    double periodicRadius = 0.;
    double periodicRotate = 0.;
    double periodicDeform = 1.;
    double periodicBend = 0.;
    int periodicN = 1;
    int periodicInterval = 1;
    double periodicFrequency = 0.;
    double periodicAutorotate = 0.;
    double periodicScale = 1.;
    double periodicScaleStep = 0.;
    double periodicOffset = 0.;
    double periodicSkip = 0.;
    double functionFrequency = 1.;
    std::string functionExpression = "";
    double functionUnit = 0.5;
    double functionRotate = 0.;
    int skewOrder = 0;
    if (_skewX) {
        _skewX->getValueAtTime(time, skewX);
    }
    if (_skewY) {
        _skewY->getValueAtTime(time, skewY);
    }
    if (_skewOrder) {
        _skewOrder->getValueAtTime(time, skewOrder);
    }
    if (_periodicRadius) {
        _periodicRadius->getValueAtTime(time, periodicRadius);
    }
    if (_periodicRotate) {
        _periodicRotate->getValueAtTime(time, periodicRotate);
    }
    if (_periodicDeform) {
        _periodicDeform->getValueAtTime(time, periodicDeform);
    }
    if (_periodicBend) {
        _periodicBend->getValueAtTime(time, periodicBend);
    }
    if (_periodicN) {
        _periodicN->getValueAtTime(time, periodicN);
    }
    if (_periodicInterval) {
        _periodicInterval->getValueAtTime(time, periodicInterval);
    }
    OfxPointD periodicBezierP1 = { 0., 0. };
    if (_periodicBezierP1) {
        _periodicBezierP1->getValueAtTime(time, periodicBezierP1.x, periodicBezierP1.y);
    }
    OfxPointD functionDomain = { -1., 1. };
    if (_functionDomain) {
        _functionDomain->getValueAtTime(time, functionDomain.x, functionDomain.y);
    }
    if (_functionUnit) {
        _functionUnit->getValueAtTime(time, functionUnit);
    }
    bool periodicSymmetry = false;
    if (_periodicSymmetry) {
        _periodicSymmetry->getValueAtTime(time, periodicSymmetry);
    }
    if (_periodicFrequency) {
        _periodicFrequency->getValueAtTime(time, periodicFrequency);
    }
    if (_periodicAutorotate) {
        _periodicAutorotate->getValueAtTime(time, periodicAutorotate);
    }
    if (_periodicScale) {
        _periodicScale->getValueAtTime(time, periodicScale);
    }
    if (_periodicScaleStep) {
        _periodicScaleStep->getValueAtTime(time, periodicScaleStep);
    }
    if (_periodicOffset) {
        _periodicOffset->getValueAtTime(time, periodicOffset);
    }
    if (_periodicSkip) {
        _periodicSkip->getValueAtTime(time, periodicSkip);
    }
    if (_functionFrequency) {
        _functionFrequency->getValueAtTime(time, functionFrequency);
    }
    if (_functionExpression) {
        _functionExpression->getValueAtTime(time, functionExpression);
    }
    if (_functionRotate) {
        _functionRotate->getValueAtTime(time, functionRotate);
    }
    OfxPointD scaleParam = { 1., 1. };
    if (_scale) {
        _scale->getValueAtTime(time, scaleParam.x, scaleParam.y);
    }
    bool scaleUniform = true;
    if (_scaleUniform) {
        _scaleUniform->getValueAtTime(time, scaleUniform);
    }
    bool flop = true;
    if (_flop) {
        flop = _flop->getValueAtTime(time);
    }
    bool flip = true;
    if (_flip) {
        flip = _flip->getValueAtTime(time);
    }
    OfxPointD scale = { 1., 1. };
    ofxsTransformGetScale(scaleParam, scaleUniform, flop, flip, &scale);

    OfxPointD translate = {0., 0. };
    if (_translate) {
        _translate->getValueAtTime(time, translate.x, translate.y);
    }
    OfxPointD center = {0., 0. };
    if (_center) {
        _center->getValueAtTime(time, center.x, center.y);
    }
    bool faceToCenter = false;
    if (_faceToCenter) {
        _faceToCenter->getValueAtTime(time, faceToCenter);
    }

    Matrix3x3 Rinv = ( ofxsMatRotation(-rot) *
                       ofxsMatSkewXY(skewX, skewY, skewOrder) *
                       ofxsMatScale(scale.x, scale.y) );
    OfxPointD newCenter;
    newCenter.x = (rod.x1 + rod.x2) / 2;
    newCenter.y = (rod.y1 + rod.y2) / 2;
    if (newCenter.x != center.x || newCenter.y != center.y) {
        bool editBlockNecessary = false;
        OfxPointD newTranslate = {0., 0.};
        if (_translate) {
            double dxrot = newCenter.x - center.x;
            double dyrot = newCenter.y - center.y;
            Point3D dRot;
            dRot.x = dxrot;
            dRot.y = dyrot;
            dRot.z = 1;
            dRot = Rinv * dRot;
            if (dRot.z != 0) {
                dRot.x /= dRot.z;
                dRot.y /= dRot.z;
            }
            double dx = dRot.x;
            double dy = dRot.y;
            newTranslate.x = translate.x + dx - dxrot;
            newTranslate.y = translate.y + dy - dyrot;
            if (newTranslate.x != translate.x || newTranslate.y != translate.y) {
                editBlockNecessary = true;
            }
        }
        EditBlock eb(*this, "resetCenter", editBlockNecessary);
        if (_center) {
            _center->setValue(newCenter.x, newCenter.y);
        }
        if (_translate) {
            _translate->setValue(newTranslate.x, newTranslate.y);
        }
    }
} // TransformPlugin::resetCenter

void
TransformPlugin::changedParam(const InstanceChangedArgs &args,
                              const std::string &paramName)
{
    if (paramName == kParamTransformResetCenterOld) {
        resetCenter(args.time);
        // Only set if necessary
        if (_centerChanged->getValue()) {
            _centerChanged->setValue(false);
        }
    } else if (paramName == kParamTransformPeriodicCurve) {
        CurveTypeEnum periodicCurve = (CurveTypeEnum)_periodicCurve->getValue();
        if (periodicCurve == eCurveTypeCustom) {
            _periodicBezierP1->setIsSecret(false);
            _periodicBezierP2->setIsSecret(false);
        } else {
            _periodicBezierP1->setIsSecret(true);
            _periodicBezierP2->setIsSecret(true);
            double x1 = 0, y1 = 0., x2 = 1., y2 = 1.;
            getCurveValue(periodicCurve, &x1, &y1, &x2, &y2);
            _periodicBezierP1->setValue(x1, y1);
            _periodicBezierP2->setValue(x2, y2);
            changedTransform(args);
        }
    } else if (paramName == kParamTransformPeriodicFrequencyUnit) {
        FrequencyUnitEnum periodicFrequencyUnit = (FrequencyUnitEnum)_periodicFrequencyUnit->getValue();
        FrequencyUnitEnum periodicFrequencyUnitBefore = _periodicFrequencyBeat->getIsSecret() ? eFrequencyUnitHz : eFrequencyUnitBPM;
        if (periodicFrequencyUnit == periodicFrequencyUnitBefore) {
            return;
        }

        if (periodicFrequencyUnit == eFrequencyUnitBPM) {
            _periodicFrequency->setDisplayRange(-240, 240);
            _periodicFrequency->setIncrement(5.);
            _periodicFrequencyBeat->setIsSecret(false);
        } else if (periodicFrequencyUnit == eFrequencyUnitHz) {
            _periodicFrequency->setDisplayRange(-5, 5);
            _periodicFrequency->setIncrement(0.1);
            _periodicFrequencyBeat->setIsSecret(true);
        }

        int keysFCount = _periodicFrequency->getNumKeys();
        int keysBCount = _periodicFrequencyBeat->getNumKeys();
        if (keysFCount == 0 && keysBCount == 0) {
            double f = _periodicFrequency->getValue();
            convertFrequency(periodicFrequencyUnitBefore, periodicFrequencyUnit, (BeatTypeEnum)_periodicFrequencyBeat->getValue(), &f);
            if (std::abs(f) <= 240) {
                _periodicFrequency->setValue(f);
            }
        } else {
            std::vector<double> keysTime;
            for (int i = 0; i < keysFCount; i++) { keysTime.insert(keysTime.end(), _periodicFrequency->getKeyTime(i)); }
            for (int i = 0; i < keysBCount; i++) { keysTime.insert(keysTime.end(), _periodicFrequencyBeat->getKeyTime(i)); }
            sort(begin(keysTime), end(keysTime));
            keysTime.erase(unique(keysTime.begin(), keysTime.end()), keysTime.end());
            for (double t:keysTime) {
                double f = _periodicFrequency->getValueAtTime(t);
                BeatTypeEnum b = (BeatTypeEnum)_periodicFrequencyBeat->getValueAtTime(t);
                convertFrequency(periodicFrequencyUnitBefore, periodicFrequencyUnit, b, &f);
                if (std::abs(f) > 240) {
                    return;
                }
                _periodicFrequency->setValueAtTime(t, f);
            }
            if (keysBCount != 0) { _periodicFrequencyBeat->resetToDefault(); }
        }
    } else if (paramName == kParamTransformFunctionCurve) {
        CurveTypeEnum functionCurve = (CurveTypeEnum)_functionCurve->getValue();
        if (functionCurve == eCurveTypeUniform) {
            _functionSymmetry->setIsSecret(true);
        } else if (functionCurve == eCurveTypeCustom) {
            _functionBezierP1->setIsSecret(false);
            _functionBezierP2->setIsSecret(false);
            _functionSymmetry->setIsSecret(false);
        } else {
            _functionBezierP1->setIsSecret(true);
            _functionBezierP2->setIsSecret(true);
            _functionSymmetry->setIsSecret(false);
            double x1 = 0, y1 = 0., x2 = 1., y2 = 1.;
            getCurveValue(functionCurve, &x1, &y1, &x2, &y2);
            _functionBezierP1->setValue(x1, y1);
            _functionBezierP2->setValue(x2, y2);
            changedTransform(args);
        }
    } else if ( (paramName == kParamTransformTranslateOld) ||
                ( paramName == kParamTransformRotateOld) ||
                ( paramName == kParamTransformScaleOld) ||
                ( paramName == kParamTransformScaleUniformOld) ||
                ( paramName == kParamTransformSkewXOld) ||
                ( paramName == kParamTransformSkewYOld) ||
                ( paramName == kParamTransformSkewOrderOld) ||
                ( paramName == kParamTransformCenterOld) ) {
        if ( (paramName == kParamTransformCenterOld) &&
             ( (args.reason == eChangeUserEdit) || (args.reason == eChangePluginEdit) ) ) {
            // Only set if necessary
            if (!_centerChanged->getValue()) {
                _centerChanged->setValue(true);
            }
        }
        changedTransform(args);
    } else if ( (paramName == kParamPremult) && (args.reason == eChangeUserEdit) ) {
        // Only set if necessary
        if (!_srcClipChanged->getValue()) {
            _srcClipChanged->setValue(true);
        }
    } else {
        Transform3x3Plugin::changedParam(args, paramName);
    }
}

void
TransformPlugin::changedClip(const InstanceChangedArgs &args,
                             const std::string &clipName)
{
    if ( (clipName == kOfxImageEffectSimpleSourceClipName) &&
         _srcClip && _srcClip->isConnected() &&
         !_centerChanged->getValue() &&
         ( args.reason == eChangeUserEdit) ) {
        resetCenter(args.time);
    }
}

mDeclarePluginFactory(TransformPluginFactory, {ofxsThreadSuiteCheck();}, {});
static
void
TransformPluginDescribeInContext(ImageEffectDescriptor &desc,
                                 ContextEnum /*context*/,
                                 PageParamDescriptor *page)
{
    // NON-GENERIC PARAMETERS
    //
    ofxsTransformDescribeParams(desc, page, NULL, /*isOpen=*/ true, /*oldParams=*/ true, /*hasAmount=*/ true, /*noTranslate=*/ false);
}

void
TransformPluginFactory::describe(ImageEffectDescriptor &desc)
{
    // basic labels
    desc.setLabel(kPluginName);
    desc.setPluginGrouping(kPluginGrouping);
    desc.setPluginDescription(kPluginDescription);

    Transform3x3Describe(desc, false);

    desc.setOverlayInteractDescriptor(new TransformOverlayDescriptorOldParams);
}

void
TransformPluginFactory::describeInContext(ImageEffectDescriptor &desc,
                                          ContextEnum context)
{
    // make some pages and to things in
    PageParamDescriptor *page = Transform3x3DescribeInContextBegin(desc, context, false);

    TransformPluginDescribeInContext(desc, context, page);

    Transform3x3DescribeInContextEnd(desc, context, page, false, Transform3x3Plugin::eTransform3x3ParamsTypeMotionBlur);

    {
        BooleanParamDescriptor* param = desc.defineBooleanParam(kParamSrcClipChanged);
        param->setDefault(false);
        param->setIsSecretAndDisabled(true);
        param->setAnimates(false);
        param->setEvaluateOnChange(false);
        if (page) {
            page->addChild(*param);
        }
    }
}

ImageEffect*
TransformPluginFactory::createInstance(OfxImageEffectHandle handle,
                                       ContextEnum /*context*/)
{
    return new TransformPlugin(handle, false, false);
}

mDeclarePluginFactory(TransformMaskedPluginFactory, {ofxsThreadSuiteCheck();}, {});
void
TransformMaskedPluginFactory::describe(ImageEffectDescriptor &desc)
{
    // basic labels
    desc.setLabel(kPluginMaskedName);
    desc.setPluginGrouping(kPluginGrouping);
    desc.setPluginDescription(kPluginMaskedDescription);

    Transform3x3Describe(desc, true);

    desc.setOverlayInteractDescriptor(new TransformOverlayDescriptorOldParams);
}

void
TransformMaskedPluginFactory::describeInContext(ImageEffectDescriptor &desc,
                                                ContextEnum context)
{
    // make some pages and to things in
    PageParamDescriptor *page = Transform3x3DescribeInContextBegin(desc, context, true);

    TransformPluginDescribeInContext(desc, context, page);

    Transform3x3DescribeInContextEnd(desc, context, page, true, Transform3x3Plugin::eTransform3x3ParamsTypeMotionBlur);

    {
        BooleanParamDescriptor* param = desc.defineBooleanParam(kParamSrcClipChanged);
        param->setDefault(false);
        param->setIsSecretAndDisabled(true);
        param->setAnimates(false);
        param->setEvaluateOnChange(false);
        if (page) {
            page->addChild(*param);
        }
    }
}

ImageEffect*
TransformMaskedPluginFactory::createInstance(OfxImageEffectHandle handle,
                                             ContextEnum /*context*/)
{
    return new TransformPlugin(handle, true, false);
}

//mDeclarePluginFactory(DirBlurPluginFactory, {ofxsThreadSuiteCheck();}, {});
//void
//DirBlurPluginFactory::describe(ImageEffectDescriptor &desc)
//{
//    // basic labels
//    desc.setLabel(kPluginDirBlurName);
//    desc.setPluginGrouping(kPluginDirBlurGrouping);
//    desc.setPluginDescription(kPluginDirBlurDescription);
//
//    Transform3x3Describe(desc, true);
//
//    desc.setOverlayInteractDescriptor(new TransformOverlayDescriptorOldParams);
//}
//
//void
//DirBlurPluginFactory::describeInContext(ImageEffectDescriptor &desc,
//                                        ContextEnum context)
//{
//    // make some pages and to things in
//    PageParamDescriptor *page = Transform3x3DescribeInContextBegin(desc, context, true);
//
//    TransformPluginDescribeInContext(desc, context, page);
//
//    Transform3x3DescribeInContextEnd(desc, context, page, true, Transform3x3Plugin::eTransform3x3ParamsTypeDirBlur);
//
//    {
//        BooleanParamDescriptor* param = desc.defineBooleanParam(kParamSrcClipChanged);
//        param->setDefault(false);
//        param->setIsSecretAndDisabled(true);
//        param->setAnimates(false);
//        param->setEvaluateOnChange(false);
//        if (page) {
//            page->addChild(*param);
//        }
//    }
//}
//
//ImageEffect*
//DirBlurPluginFactory::createInstance(OfxImageEffectHandle handle,
//                                     ContextEnum /*context*/)
//{
//    return new TransformPlugin(handle, true, true);
//}
//
//static TransformPluginFactory p1(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
static TransformMaskedPluginFactory p2(kPluginMaskedIdentifier, kPluginVersionMajor, kPluginVersionMinor);
//static DirBlurPluginFactory p3(kPluginDirBlurIdentifier, kPluginVersionMajor, kPluginVersionMinor);
//mRegisterPluginFactoryInstance(p1)
mRegisterPluginFactoryInstance(p2)
//mRegisterPluginFactoryInstance(p3)

OFXS_NAMESPACE_ANONYMOUS_EXIT
