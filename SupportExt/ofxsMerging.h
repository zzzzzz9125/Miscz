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
 * OFX Merge helpers
 */

#ifndef Misc_Merging_helper_h
#define Misc_Merging_helper_h

#include <cmath>
#include <cfloat>
#include <algorithm>

#include "ofxsImageEffect.h"

#ifndef M_PI
#define M_PI        3.14159265358979323846264338327950288   /* pi             */
#endif

namespace OFX {
// References:
//
// SVG Compositing Specification:
//   http://www.w3.org/TR/SVGCompositing/
// PDF Reference v1.7:
//   http://www.adobe.com/content/dam/Adobe/en/devnet/acrobat/pdfs/pdf_reference_1-7.pdf
//   http://www.adobe.com/devnet/pdf/pdf_reference_archive.html
// Adobe photoshop blending modes:
//   http://helpx.adobe.com/en/photoshop/using/blending-modes.html
//   http://www.deepskycolors.com/archive/2010/04/21/formulas-for-Photoshop-blending-modes.html
// ImageMagick:
//   http://www.imagemagick.org/Usage/compose/
//
// Note about the Soft-Light operation:
// Soft-light as implemented in Nuke comes from the SVG 2004 specification, which is wrong.
// In SVG 2004, 'Soft_Light' did not work as expected, producing a brightening for any non-gray shade
// image overlay.
// It was fixed in the March 2009 SVG specification, which was used for this implementation.
// The formula in SVG Compositing 2015 (https://www.w3.org/TR/compositing-1/) is unchanged.

namespace MergeImages2D {
// please keep this long list sorted alphabetically
enum MergingFunctionEnum
{
    eMergeATop = 0,
    eMergeAverage,
    eMergeColor,
    eMergeColorBurn,
    eMergeColorDodge,
    eMergeConjointOver,
    eMergeCopy,
    eMergeDifference,
    eMergeDisjointOver,
    eMergeDivide,
    eMergeExclusion,
    eMergeFreeze,
    eMergeFrom,
    eMergeGeometric,
    eMergeGrainExtract,
    eMergeGrainMerge,
    eMergeHardLight,
    eMergeHue,
    eMergeHypot,
    eMergeIn,
    //eMergeInterpolated,
    eMergeLuminosity,
    eMergeMask,
    eMergeMatte,
    eMergeMax,
    eMergeMin,
    eMergeMinus,
    eMergeMultiply,
    eMergeOut,
    eMergeOver,
    eMergeOverlay,
    eMergePinLight,
    eMergePlus,
    eMergeReflect,
    eMergeSaturation,
    eMergeScreen,
    eMergeSoftLight,
    eMergeStencil,
    eMergeUnder,
    eMergeXOR,
};

inline bool
isMaskable(MergingFunctionEnum operation)
{
    switch (operation) {
    case eMergeAverage:
    case eMergeColorBurn:
    case eMergeColorDodge:
    case eMergeDifference:
    case eMergeDivide:
    case eMergeExclusion:
    case eMergeFrom:
    case eMergeFreeze:
    case eMergeGeometric:
    case eMergeGrainExtract:
    case eMergeGrainMerge:
    case eMergeHardLight:
    case eMergeHypot:
    //case eMergeInterpolated:
    case eMergeMax:
    case eMergeMin:
    case eMergeMinus:
    case eMergeMultiply:
    case eMergeOverlay:
    case eMergePinLight:
    case eMergePlus:
    case eMergeReflect:
    case eMergeSoftLight:

        return true;
    case eMergeATop:
    case eMergeConjointOver:
    case eMergeCopy:
    case eMergeDisjointOver:
    case eMergeIn:
    case eMergeMask:
    case eMergeMatte:
    case eMergeOut:
    case eMergeOver:
    case eMergeScreen:
    case eMergeStencil:
    case eMergeUnder:
    case eMergeXOR:
    case eMergeHue:
    case eMergeSaturation:
    case eMergeColor:
    case eMergeLuminosity:

        return false;
    }

    return true;
} // isMaskable

// if Aa is black and transparent, does the operator give Bb?
inline bool
isIdentityForBOnly(MergingFunctionEnum operation)
{
    switch (operation) {
    case eMergeATop: //"Ab + B(1 - a) (a.k.a. src-atop)";
    case eMergeExclusion: //"A+B-2AB";
    case eMergeMatte: //"Aa + B(1-a) (unpremultiplied over)";
    case eMergeOver: //"A+B(1-a) (a.k.a. src-over)";
    case eMergePlus: //"A+B (a.k.a. add)";
    case eMergeScreen: //"A+B-AB if A or B <= 1, otherwise max(A, B)";
    case eMergeStencil: //"B(1-a) (a.k.a. dst-out)";
    case eMergeUnder: //"A(1-b)+B (a.k.a. dst-over)";
    case eMergeXOR: //"A(1-b)+B(1-a)";

        return true;

    case eMergeAverage: // "(A + B) / 2";
    case eMergeColor: // "SetLum(A, Lum(B))";
    case eMergeColorBurn: // "darken B towards A";
    case eMergeColorDodge: // "brighten B towards A";
    case eMergeConjointOver: // "A + B(1-a)/b, A if a > b";
    case eMergeCopy: // "A (a.k.a. src)";
    case eMergeDifference: // "abs(A-B) (a.k.a. absminus)";
    case eMergeDisjointOver: // "A+B(1-a)/b, A+B if a+b < 1";
    case eMergeDivide: // "A/B, 0 if A < 0 and B < 0";
    case eMergeFreeze: // "1-sqrt(1-A)/B";
    case eMergeFrom: // "B-A (a.k.a. subtract)";
    case eMergeGeometric: // "2AB/(A+B)";
    case eMergeGrainExtract: // "B - A + 0.5";
    case eMergeGrainMerge: // "B + A - 0.5";
    case eMergeHardLight: // "multiply(2*A, B) if A < 0.5, screen(2*A - 1, B) if A > 0.5";
    case eMergeHue: // "SetLum(SetSat(A, Sat(B)), Lum(B))";
    case eMergeHypot: // "sqrt(A*A+B*B)";
    case eMergeIn: // "Ab (a.k.a. src-in)";
    //case eMergeInterpolated: // "(like average but better and slower)";
    case eMergeLuminosity: // "SetLum(B, Lum(A))";
    case eMergeMask: // "Ba (a.k.a dst-in)";
    case eMergeMax: // "max(A, B) (a.k.a. lighten only)";
    case eMergeMin: // "min(A, B) (a.k.a. darken only)";
    case eMergeMinus: // "A-B";
    case eMergeMultiply: // "AB, A if A < 0 and B < 0";
    case eMergeOut: // "A(1-b) (a.k.a. src-out)";
    case eMergeOverlay: // "multiply(A, 2*B) if B < 0.5, screen(A, 2*B - 1) if B > 0.5";
    case eMergePinLight: // "if B >= 0.5 then max(A, 2*B - 1), min(A, B * 2) else";
    case eMergeReflect: // "A*A / (1 - B)";
    case eMergeSaturation: // "SetLum(SetSat(B, Sat(A)), Lum(B))";
    case eMergeSoftLight: // "burn-in if A < 0.5, lighten if A > 0.5";
    //default: // do not enable the default case, so that we can catch warnings when adding a new operator

        return false;
    } // switch
} // isIdentityForBOnly

// is the operator separable for R,G,B components, or do they have to be processed simultaneously?
inline bool
isSeparable(MergingFunctionEnum operation)
{
    switch (operation) {
    case eMergeHue: // "SetLum(SetSat(A, Sat(B)), Lum(B))";
    case eMergeSaturation: // "SetLum(SetSat(B, Sat(A)), Lum(B))";
    case eMergeColor: // "SetLum(A, Lum(B))";
    case eMergeLuminosity: // "SetLum(B, Lum(A))";

        return false;

    case eMergeATop: //"Ab + B(1 - a) (a.k.a. src-atop)";
    case eMergeAverage: // "(A + B) / 2";
    case eMergeColorBurn: // "darken B towards A";
    case eMergeColorDodge: // "brighten B towards A";
    case eMergeConjointOver: // "A + B(1-a)/b, A if a > b";
    case eMergeCopy: // "A (a.k.a. src)";
    case eMergeDifference: // "abs(A-B) (a.k.a. absminus)";
    case eMergeDisjointOver: // "A+B(1-a)/b, A+B if a+b < 1";
    case eMergeDivide: // "A/B, 0 if A < 0 and B < 0";
    case eMergeExclusion: //"A+B-2AB";
    case eMergeFreeze: // "1-sqrt(1-A)/B";
    case eMergeFrom: // "B-A (a.k.a. subtract)";
    case eMergeGeometric: // "2AB/(A+B)";
    case eMergeGrainExtract: // "B - A + 0.5";
    case eMergeGrainMerge: // "B + A - 0.5";
    case eMergeHardLight: // "multiply(2*A, B) if A < 0.5, screen(2*A - 1, B) if A > 0.5";
    case eMergeHypot: // "sqrt(A*A+B*B)";
    case eMergeIn: // "Ab (a.k.a. src-in)";
    case eMergeMask: // "Ba (a.k.a dst-in)";
    case eMergeMatte: //"Aa + B(1-a) (unpremultiplied over)";
    case eMergeMax: // "max(A, B) (a.k.a. lighten only)";
    case eMergeMin: // "min(A, B) (a.k.a. darken only)";
    case eMergeMinus: // "A-B";
    case eMergeMultiply: // "AB, A if A < 0 and B < 0";
    case eMergeOut: // "A(1-b) (a.k.a. src-out)";
    case eMergeOver: //"A+B(1-a) (a.k.a. src-over)";
    case eMergeOverlay: // "multiply(A, 2*B) if B < 0.5, screen(A, 2*B - 1) if B > 0.5";
    case eMergePinLight: // "if B >= 0.5 then max(A, 2*B - 1), min(A, B * 2) else";
    case eMergePlus: //"A+B (a.k.a. add)";
    case eMergeReflect: // "A*A / (1 - B)";
    case eMergeScreen: //"A+B-AB if A or B <= 1, otherwise max(A, B)";
    case eMergeSoftLight: // "burn-in if A < 0.5, lighten if A > 0.5";
    case eMergeStencil: //"B(1-a) (a.k.a. dst-out)";
    case eMergeUnder: //"A(1-b)+B (a.k.a. dst-over)";
    case eMergeXOR: //"A(1-b)+B(1-a)";
    //default: // do not enable the default case, so that we can catch warnings when adding a new operator

        return true;
    }
}

inline std::string
getOperationString(MergingFunctionEnum operation)
{
    switch (operation) {
    case eMergeATop:

        return "atop";

    case eMergeAverage:

        return "average";

    case eMergeColor:

        return "color";

    case eMergeColorBurn:

        return "color-burn";

    case eMergeColorDodge:

        return "color-dodge";

    case eMergeConjointOver:

        return "conjoint-over";

    case eMergeCopy:

        return "copy";

    case eMergeDifference:

        return "difference";

    case eMergeDisjointOver:

        return "disjoint-over";

    case eMergeDivide:

        return "divide";

    case eMergeExclusion:

        return "exclusion";

    case eMergeFreeze:

        return "freeze";

    case eMergeFrom:

        return "from";

    case eMergeGeometric:

        return "geometric";

    case eMergeGrainExtract:

        return "grain-extract";

    case eMergeGrainMerge:

        return "grain-merge";

    case eMergeHardLight:

        return "hard-light";

    case eMergeHue:

        return "hue";

    case eMergeHypot:

        return "hypot";

    case eMergeIn:

        return "in";

    //case eMergeInterpolated:
    //    return "interpolated";

    case eMergeLuminosity:

        return "luminosity";

    case eMergeMask:

        return "mask";

    case eMergeMatte:

        return "matte";

    case eMergeMax:

        return "max";

    case eMergeMin:

        return "min";

    case eMergeMinus:

        return "minus";

    case eMergeMultiply:

        return "multiply";

    case eMergeOut:

        return "out";

    case eMergeOver:

        return "over";

    case eMergeOverlay:

        return "overlay";

    case eMergePinLight:

        return "pinlight";

    case eMergePlus:

        return "plus";

    case eMergeReflect:

        return "reflect";

    case eMergeSaturation:

        return "saturation";

    case eMergeScreen:

        return "screen";

    case eMergeSoftLight:

        return "soft-light";

    case eMergeStencil:

        return "stencil";

    case eMergeUnder:

        return "under";

    case eMergeXOR:

        return "xor";
    } // switch

    return "unknown";
} // getOperationString

inline std::string
getOperationDescription(MergingFunctionEnum operation)
{
    switch (operation) {
    case eMergeATop:

        return "Ab + B(1 - a) (a.k.a. src-atop)";

    case eMergeAverage:

        return "(A + B) / 2";

    case eMergeColor:

        return "SetLum(A, Lum(B))";

    case eMergeColorBurn:

        return "darken B towards A";

    case eMergeColorDodge:

        return "brighten B towards A";

    case eMergeConjointOver:

        return "A + B(1-a)/b, A if a > b";

    case eMergeCopy:

        return "A (a.k.a. src)";

    case eMergeDifference:

        return "abs(A-B) (a.k.a. absminus)";

    case eMergeDisjointOver:

        return "A+B(1-a)/b, A+B if a+b < 1";

    case eMergeDivide:

        return "A/B, 0 if A < 0 and B < 0";

    case eMergeExclusion:

        return "A+B-2AB";

    case eMergeFreeze:

        return "1-sqrt(1-A)/B";

    case eMergeFrom:

        return "B-A (a.k.a. subtract)";

    case eMergeGeometric:

        return "2AB/(A+B)";

    case eMergeGrainExtract:

        return "B - A + 0.5";

    case eMergeGrainMerge:

        return "B + A - 0.5";

    case eMergeHardLight:

        return "multiply(2*A, B) if A < 0.5, screen(2*A - 1, B) if A > 0.5";

    case eMergeHue:

        return "SetLum(SetSat(A, Sat(B)), Lum(B))";

    case eMergeHypot:

        return "sqrt(A*A+B*B)";

    case eMergeIn:

        return "Ab (a.k.a. src-in)";

    //case eMergeInterpolated:
    //    return "(like average but better and slower)";

    case eMergeLuminosity:

        return "SetLum(B, Lum(A))";

    case eMergeMask:

        return "Ba (a.k.a dst-in)";

    case eMergeMatte:

        return "Aa + B(1-a) (unpremultiplied over)";

    case eMergeMax:

        return "max(A, B) (a.k.a. lighten only)";

    case eMergeMin:

        return "min(A, B) (a.k.a. darken only)";

    case eMergeMinus:

        return "A-B";

    case eMergeMultiply:

        return "AB, A if A < 0 and B < 0";

    case eMergeOut:

        return "A(1-b) (a.k.a. src-out)";

    case eMergeOver:

        return "A+B(1-a) (a.k.a. src-over)";

    case eMergeOverlay:

        return "multiply(A, 2*B) if B < 0.5, screen(A, 2*B - 1) if B > 0.5";

    case eMergePinLight:

        return "if B >= 0.5 then max(A, 2*B - 1), min(A, B * 2) else";

    case eMergePlus:

        return "A+B (a.k.a. add)";

    case eMergeReflect:

        return "A*A / (1 - B)";

    case eMergeSaturation:

        return "SetLum(SetSat(B, Sat(A)), Lum(B))";

    case eMergeScreen:

        return "A+B-AB if A or B <= 1, otherwise max(A, B)";

    case eMergeSoftLight:

        return "burn-in if A < 0.5, lighten if A > 0.5";

    case eMergeStencil:

        return "B(1-a) (a.k.a. dst-out)";

    case eMergeUnder:

        return "A(1-b)+B (a.k.a. dst-over)";

    case eMergeXOR:

        return "A(1-b)+B(1-a)";
    } // switch

    return "unknown";
} // getOperationString

inline std::string
getOperationHelp(MergingFunctionEnum operation, bool markdown)
{
    if (!markdown) {
        return getOperationString(operation) + ": " + getOperationDescription(operation);
    }
    std::string escaped = getOperationString(operation) + ": ";
    std::string plain = getOperationDescription(operation);
    // the following chars must be backslash-escaped in markdown:
    // \    backslash
    // `    backtick
    // *    asterisk
    // _    underscore
    // {}   curly braces
    // []   square brackets
    // ()   parentheses
    // #    hash mark
    // +    plus sign
    // -    minus sign (hyphen)
    // .    dot
    // !    exclamation mark
    for (unsigned i = 0; i < plain.size(); ++i) {
        if (plain[i] == '\\' ||
            plain[i] == '`' ||
            plain[i] == '*' ||
            plain[i] == '_' ||
            plain[i] == '{' ||
            plain[i] == '}' ||
            plain[i] == '[' ||
            plain[i] == ']' ||
            plain[i] == '(' ||
            plain[i] == ')' ||
            plain[i] == '#' ||
            plain[i] == '+' ||
            plain[i] == '-' ||
            plain[i] == '.' ||
            plain[i] == '!') {
            escaped += '\\';
        }
        escaped += plain[i];
    }
    return escaped;
}

inline std::string
getOperationGroupString(MergingFunctionEnum operation)
{
    switch (operation) {
    // Porter Duff Compositing Operators
    // missing: clear
    case eMergeCopy:     // src
    // missing: dst
    case eMergeOver:     // src-over
    case eMergeUnder:     // dst-over
    case eMergeIn:     // src-in
    case eMergeMask:     // dst-in
    case eMergeOut:     // src-out
    case eMergeStencil:     // dst-out
    case eMergeATop:     // src-atop
    case eMergeXOR:     // xor
        return "Operator";

    // Blend modes, see https://en.wikipedia.org/wiki/Blend_modes

    // Multiply and screen
    case eMergeMultiply:
    case eMergeScreen:
    case eMergeOverlay:
    case eMergeHardLight:
    case eMergeSoftLight:

        return "Multiply and Screen";

    // Dodge and burn
    case eMergeColorDodge:
    case eMergeColorBurn:
    case eMergePinLight:
    //case eMergeDifference:
    case eMergeExclusion:

        //case eMergeDivide:
        return "Dodge and Burn";

    // Simple arithmetic blend modes
    case eMergeDivide:
    case eMergePlus:
    case eMergeFrom:
    case eMergeMinus:
    case eMergeDifference:
    case eMergeMin:
    case eMergeMax:

        return "HSL";

    // Hue, saturation, luminosity
    case eMergeHue:
    case eMergeSaturation:
    case eMergeColor:
    case eMergeLuminosity:

        return "HSL";

    case eMergeAverage:
    case eMergeConjointOver:
    case eMergeDisjointOver:
    case eMergeFreeze:
    case eMergeGeometric:
    case eMergeGrainExtract:
    case eMergeGrainMerge:
    case eMergeHypot:
    //case eMergeInterpolated:
    case eMergeMatte:
    case eMergeReflect:

        return "Other";
    } // switch

    return "unknown";
} // getOperationGroupString

template <typename PIX>
PIX
averageFunc(PIX A,
            PIX B)
{
    return (A + B) / 2;
}

// https://www.w3.org/TR/compositing-1/#porterduffcompositingoperators_src
template <typename PIX>
PIX
copyFunc(PIX A,
         PIX /*B*/)
{
    return A;
}

// https://www.w3.org/TR/compositing-1/#porterduffcompositingoperators_plus
template <typename PIX>
PIX
plusFunc(PIX A,
         PIX B)
{
    return A + B;
}

template <typename PIX, int maxValue>
PIX
grainExtractFunc(PIX A,
                 PIX B)
{
    return (B - A + (PIX)maxValue / 2);
}

template <typename PIX, int maxValue>
PIX
grainMergeFunc(PIX A,
               PIX B)
{
    return (B + A - (PIX)maxValue / 2);
}

// https://www.w3.org/TR/compositing-1/#blendingdifference
template <typename PIX>
PIX
differenceFunc(PIX A,
               PIX B)
{
    return std::abs(A - B);
}

template <typename PIX>
PIX
divideFunc(PIX A,
           PIX B)
{
    if (B <= 0) {
        return 0;
    }

    return A / B;
}

// https://www.w3.org/TR/compositing-1/#blendingexclusion
template <typename PIX, int maxValue>
PIX
exclusionFunc(PIX A,
              PIX B)
{
    return PIX(A + B - 2 * A * B / (double)maxValue);
}

template <typename PIX>
PIX
fromFunc(PIX A,
         PIX B)
{
    return B - A;
}

template <typename PIX>
PIX
geometricFunc(PIX A,
              PIX B)
{
    double sum = (double)A + (double)B;

    if (sum == 0) {
        return 0;
    } else {
        return 2 * A * B / sum;
    }
}

// https://www.w3.org/TR/compositing-1/#blendingmultiply
template <typename PIX, int maxValue>
PIX
multiplyFunc(PIX A,
             PIX B)
{
    if ( (A < 0) && (B < 0) ) {
        return A;
    } else {
        return PIX(A * B / (double)maxValue);
    }
}

// https://www.w3.org/TR/compositing-1/#blendingscreen
template <typename PIX, int maxValue>
PIX
screenFunc(PIX A,
           PIX B)
{
    if ( (A <= maxValue) || (B <= maxValue) ) {
        return PIX(A + B - A * B / (double)maxValue);
    } else {
        return (std::max)(A, B);
    }
}

// https://www.w3.org/TR/compositing-1/#valdef-blend-mode-hard-light
template <typename PIX, int maxValue>
PIX
hardLightFunc(PIX A,
              PIX B)
{
    if ( 2 * A < maxValue ) {
        return multiplyFunc<PIX,maxValue>(2*A, B);
    } else {
        return screenFunc<PIX,maxValue>(2*A-maxValue, B);
    }
}

// https://www.w3.org/TR/compositing-1/#blendingsoftlight
template <typename PIX, int maxValue>
PIX
softLightFunc(PIX A,
              PIX B)
{
    double An = A / (double)maxValue;
    double Bn = B / (double)maxValue;

    // Formula from SVG Compositing (2015): https://www.w3.org/TR/compositing-1/#blendingsoftlight
    // Formula from SVG Compositing (2009): https://www.w3.org/TR/2009/WD-SVGCompositing-20090430/
    // Wrong formula, from SVG 1.2 (2004): https://www.w3.org/TR/2004/WD-SVG12-20041027/rendering.html
    if (2 * An <= 1) {
        return PIX( maxValue * ( Bn - (1 - 2 * An) * Bn * (1 - Bn) ) );
    } else if (4 * Bn <= 1) {
        // SVG Compositing 2009 version:
        //return PIX( maxValue * ( Bn + (2 * An - 1) * (4 * Bn * (4 * Bn + 1) * (Bn - 1) + 7 * Bn) ) );
        // SVG Compositing 2015 version (strictly equal to the SVG 2009 version, less multiplications):
        return PIX( maxValue * ( Bn + (2 * An - 1) * (((16 * Bn - 12) * Bn + 4) * Bn - Bn) ) );
        // Proof (enter the following code in https://sagecell.sagemath.org):
        /*
          Bn = PolynomialRing(RationalField(), 'Bn').gen()
          f = 4 * Bn * (4 * Bn + 1) * (Bn - 1) + 7 * Bn
          print(f.factor())
          g = ((16 * Bn - 12) * Bn + 4) * Bn - Bn
          print(g.factor())
          print(f-g)
         */
    } else {
        return PIX( maxValue * ( Bn + (2 * An - 1) * (sqrt(Bn) - Bn) ) );
    }
}

template <typename PIX>
PIX
hypotFunc(PIX A,
          PIX B)
{
    return PIX( std::sqrt( (double)(A * A + B * B) ) );
}

template <typename PIX>
PIX
minusFunc(PIX A,
          PIX B)
{
    return A - B;
}

// https://www.w3.org/TR/compositing-1/#blendingdarken
template <typename PIX>
PIX
darkenFunc(PIX A,
           PIX B)
{
    return (std::min)(A, B);
}

// https://www.w3.org/TR/compositing-1/#blendinglighten
template <typename PIX>
PIX
lightenFunc(PIX A,
            PIX B)
{
    return (std::max)(A, B);
}

// https://www.w3.org/TR/compositing-1/#blendingoverlay
template <typename PIX, int maxValue>
PIX
overlayFunc(PIX A,
            PIX B)
{
    return hardLightFunc<PIX,maxValue>(B,A);
}

// https://www.w3.org/TR/compositing-1/#blendingcolordodge
template <typename PIX, int maxValue>
PIX
colorDodgeFunc(PIX A,
               PIX B)
{
    if (A >= maxValue) {
        return A;
    } else {
        return PIX( maxValue * (std::min)( 1., B / (maxValue - (double)A) ) );
    }
}

// https://www.w3.org/TR/compositing-1/#blendingcolorburn
template <typename PIX, int maxValue>
PIX
colorBurnFunc(PIX A,
              PIX B)
{
    if (A <= 0) {
        return A;
    } else {
        return PIX( maxValue * ( 1. - (std::min)(1., (maxValue - B) / (double)A) ) );
    }
}

template <typename PIX, int maxValue>
PIX
pinLightFunc(PIX A,
             PIX B)
{
    PIX max2 = PIX( (double)maxValue / 2. );

    return A >= max2 ? (std::max)(B, (A - max2) * 2) : (std::min)(B, A * 2);
}

template <typename PIX, int maxValue>
PIX
reflectFunc(PIX A,
            PIX B)
{
    if (B >= maxValue) {
        return maxValue;
    } else {
        return PIX( (std::min)( (double)maxValue, A * A / (double)(maxValue - B) ) );
    }
}

template <typename PIX, int maxValue>
PIX
freezeFunc(PIX A,
           PIX B)
{
    if (B <= 0) {
        return 0;
    } else {
        double An = A / (double)maxValue;
        double Bn = B / (double)maxValue;

        return PIX( (std::max)( 0., maxValue * (1 - std::sqrt( (std::max)(0., 1. - An) ) / Bn) ) );
    }
}

// This functions seems wrong. Is it a confusion with cosine interpolation?
// see http://paulbourke.net/miscellaneous/interpolation/
template <typename PIX, int maxValue>
PIX
interpolatedFunc(PIX A,
                 PIX B)
{
    double An = A / (double)maxValue;
    double Bn = B / (double)maxValue;

    return PIX( maxValue * ( 0.5 - 0.25 * ( std::cos(M_PI * An) - std::cos(M_PI * Bn) ) ) );
}

// https://www.w3.org/TR/compositing-1/#porterduffcompositingoperators_srcatop
template <typename PIX, int maxValue>
PIX
atopFunc(PIX A,
         PIX B,
         PIX alphaA,
         PIX alphaB)
{
    return PIX( A * alphaB / (double)maxValue + B * (1. - alphaA / (double)maxValue) );
}

template <typename PIX, int maxValue>
PIX
conjointOverFunc(PIX A,
                 PIX B,
                 PIX alphaA,
                 PIX alphaB)
{
    if (alphaA > alphaB) {
        return A;
    } else if (alphaB <= 0) {
        return A + B;
    } else {
        return A + B * ( 1. - (alphaA / (double)alphaB) );
    }
}

template <typename PIX, int maxValue>
PIX
disjointOverFunc(PIX A,
                 PIX B,
                 PIX alphaA,
                 PIX alphaB)
{
    if (alphaA >= maxValue) {
        return A;
    } else if ( (alphaA + alphaB) < maxValue ) {
        return A + B;
    } else if (alphaB <= 0) {
        return A + B * (1 - alphaA / (double)maxValue);
    } else {
        return A + B * (maxValue - alphaA) / alphaB;
    }
}

// https://www.w3.org/TR/compositing-1/#porterduffcompositingoperators_srcin
template <typename PIX, int maxValue>
PIX
inFunc(PIX A,
       PIX /*B*/,
       PIX /*alphaA*/,
       PIX alphaB)
{
    return PIX(A * alphaB / (double)maxValue);
}

template <typename PIX, int maxValue>
PIX
matteFunc(PIX A,
          PIX B,
          PIX alphaA,
          PIX /*alphaB*/)
{
    return PIX( A * alphaA / (double)maxValue + B * (1. - alphaA / (double)maxValue) );
}

// https://www.w3.org/TR/compositing-1/#porterduffcompositingoperators_dstin
template <typename PIX, int maxValue>
PIX
maskFunc(PIX /*A*/,
         PIX B,
         PIX alphaA,
         PIX /*alphaB*/)
{
    return PIX(B * alphaA / (double)maxValue);
}

// https://www.w3.org/TR/compositing-1/#porterduffcompositingoperators_srcout
template <typename PIX, int maxValue>
PIX
outFunc(PIX A,
        PIX /*B*/,
        PIX /*alphaA*/,
        PIX alphaB)
{
    return PIX( A * (1. - alphaB / (double)maxValue) );
}

// https://www.w3.org/TR/compositing-1/#porterduffcompositingoperators_srcover
template <typename PIX, int maxValue>
PIX
overFunc(PIX A,
         PIX B,
         PIX alphaA,
         PIX /*alphaB*/)
{
    return PIX( A + B * (1 - alphaA / (double)maxValue) );
}

template <typename PIX, int maxValue>
PIX
stencilFunc(PIX /*A*/,
            PIX B,
            PIX alphaA,
            PIX /*alphaB*/)
{
    return PIX( B * (1 - alphaA / (double)maxValue) );
}

// https://www.w3.org/TR/compositing-1/#porterduffcompositingoperators_dstover
template <typename PIX, int maxValue>
PIX
underFunc(PIX A,
          PIX B,
          PIX /*alphaA*/,
          PIX alphaB)
{
    return PIX(A * (1 - alphaB / (double)maxValue) + B);
}

// https://www.w3.org/TR/compositing-1/#porterduffcompositingoperators_xor
template <typename PIX, int maxValue>
PIX
xorFunc(PIX A,
        PIX B,
        PIX alphaA,
        PIX alphaB)
{
    return PIX( A * (1 - alphaB / (double)maxValue) + B * (1 - alphaA / (double)maxValue) );
}

///////////////////////////////////////////////////////////////////////////////
//
// Code from pixman-combine-float.c
#define OFX_PIXMAN_USE_DOUBLE
#ifdef OFX_PIXMAN_USE_DOUBLE
typedef double pixman_float_t;
#define PIXMAN_FLT_MIN DBL_MIN
#else
typedef float pixman_float_t;
#define PIXMAN_FLT_MIN FLT_MIN
#endif

// START
/*
 * Copyright © 2010, 2012 Soren Sandmann Pedersen
 * Copyright © 2010, 2012 Red Hat, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Author: Soren Sandmann Pedersen (sandmann@cs.au.dk)
 */
/*
 * PDF nonseperable blend modes are implemented using the following functions
 * to operate in Hsl space, with Cmax, Cmid, Cmin referring to the max, mid
 * and min value of the red, green and blue components.
 *
 * LUM (C) = 0.3 × Cred + 0.59 × Cgreen + 0.11 × Cblue
 *
 * clip_color (C):
 *     l = LUM (C)
 *     min = Cmin
 *     max = Cmax
 *     if n < 0.0
 *         C = l + (((C – l) × l) ⁄ (l – min))
 *     if x > 1.0
 *         C = l + (((C – l) × (1 – l) ) ⁄ (max – l))
 *     return C
 *
 * set_lum (C, l):
 *     d = l – LUM (C)
 *     C += d
 *     return clip_color (C)
 *
 * SAT (C) = CH_MAX (C) - CH_MIN (C)
 *
 * set_sat (C, s):
 *     if Cmax > Cmin
 *         Cmid = ( ( ( Cmid – Cmin ) × s ) ⁄ ( Cmax – Cmin ) )
 *         Cmax = s
 *     else
 *         Cmid = Cmax = 0.0
 *         Cmin = 0.0
 *     return C
 */

/* For premultiplied colors, we need to know what happens when C is
 * multiplied by a real number. LUM and SAT are linear:
 *
 *     LUM (r × C) = r × LUM (C)	SAT (r * C) = r * SAT (C)
 *
 * If we extend clip_color with an extra argument a and change
 *
 *     if x >= 1.0
 *
 * into
 *
 *     if x >= a
 *
 * then clip_color is also linear:
 *
 *     r * clip_color (C, a) = clip_color (r * C, r * a);
 *
 * for positive r.
 *
 * Similarly, we can extend set_lum with an extra argument that is just passed
 * on to clip_color:
 *
 *       r * set_lum (C, l, a)
 *
 *     = r × clip_color (C + l - LUM (C), a)
 *
 *     = clip_color (r * C + r × l - r * LUM (C), r * a)
 *
 *     = set_lum (r * C, r * l, r * a)
 *
 * Finally, set_sat:
 *
 *       r * set_sat (C, s) = set_sat (x * C, r * s)
 *
 * The above holds for all non-zero x, because the x'es in the fraction for
 * C_mid cancel out. Specifically, it holds for x = r:
 *
 *       r * set_sat (C, s) = set_sat (r * C, r * s)
 *
 */
typedef struct
{
    pixman_float_t r;
    pixman_float_t g;
    pixman_float_t b;
} pixman_rgb_t;

/*
 https://cgit.freedesktop.org/pixman/commit/pixman/pixman-combine-float.c?id=4dfda2adfe2eb1130fc27b1da35df778284afd91
 float-combiner.c: Change tests for x == 0.0 tests to - FLT_MIN < x < FLT_MIN

pixman-float-combiner.c currently uses checks like these:

    if (x == 0.0f)
        ...
    else
        ... / x;

to prevent division by 0. In theory this is correct: a division-by-zero
exception is only supposed to happen when the floating point numerator is
exactly equal to a positive or negative zero.

However, in practice, the combination of x87 and gcc optimizations
causes issues. The x87 registers are 80 bits wide, which means the
initial test:

    if (x == 0.0f)

may be false when x is an 80 bit floating point number, but when x is
rounded to a 32 bit single precision number, it becomes equal to
0.0. In principle, gcc should compensate for this quirk of x87, and
there are some options such as -ffloat-store, -fexcess-precision=standard,
and -std=c99 that will make it do so, but these all have a performance
cost. It is also possible to set the FPU to a mode that makes it do
all computation with single or double precision, but that would
require pixman to save the existing mode before doing anything with
floating point and restore it afterwards.

Instead, this patch side-steps the issue by replacing exact checks for
equality with zero with a new macro that checkes whether the value is
between -FLT_MIN and FLT_MIN.

There is extensive reading material about this issue linked off the
infamous gcc bug 323:

    http://gcc.gnu.org/bugzilla/show_bug.cgi?id=323
*/
#define PIXMAN_IS_ZERO(f) (-PIXMAN_FLT_MIN < (f) && (f) < PIXMAN_FLT_MIN)

inline pixman_float_t
channel_min (const pixman_rgb_t *c)
{
    return (std::min)((std::min)(c->r, c->g), c->b);
}

inline pixman_float_t
channel_max (const pixman_rgb_t *c)
{
    return (std::max)((std::max)(c->r, c->g), c->b);
}

inline pixman_float_t
get_lum (const pixman_rgb_t *c)
{
    return c->r * 0.3f + c->g * 0.59f + c->b * 0.11f;
}

inline pixman_float_t
get_sat (const pixman_rgb_t *c)
{
    return channel_max(c) - channel_min(c);
}

inline void
clip_color (pixman_rgb_t *color,
            pixman_float_t a)
{
    pixman_float_t l = get_lum(color);
    pixman_float_t n = channel_min(color);
    pixman_float_t x = channel_max(color);
    pixman_float_t t;

    if (n < 0.0f) {
        t = l - n;
        if ( PIXMAN_IS_ZERO(t) ) {
            color->r = 0.0f;
            color->g = 0.0f;
            color->b = 0.0f;
        } else {
            color->r = l + ( ( (color->r - l) * l ) / t );
            color->g = l + ( ( (color->g - l) * l ) / t );
            color->b = l + ( ( (color->b - l) * l ) / t );
        }
    }
    if (x > a) {
        t = x - l;
        if ( PIXMAN_IS_ZERO(t) ) {
            color->r = a;
            color->g = a;
            color->b = a;
        } else {
            color->r = l + ( ( (color->r - l) * (a - l) / t ) );
            color->g = l + ( ( (color->g - l) * (a - l) / t ) );
            color->b = l + ( ( (color->b - l) * (a - l) / t ) );
        }
    }
}

static void
set_lum (pixman_rgb_t *color,
         pixman_float_t sa,
         pixman_float_t l)
{
    pixman_float_t d = l - get_lum(color);

    color->r = color->r + d;
    color->g = color->g + d;
    color->b = color->b + d;

    clip_color(color, sa);
}

inline void
set_sat (pixman_rgb_t *src,
         pixman_float_t sat)
{
    pixman_float_t *max, *mid, *min;
    pixman_float_t t;

    if (src->r > src->g) {
        if (src->r > src->b) {
            max = &(src->r);

            if (src->g > src->b) {
                mid = &(src->g);
                min = &(src->b);
            } else {
                mid = &(src->b);
                min = &(src->g);
            }
        } else {
            max = &(src->b);
            mid = &(src->r);
            min = &(src->g);
        }
    } else {
        if (src->r > src->b) {
            max = &(src->g);
            mid = &(src->r);
            min = &(src->b);
        } else {
            min = &(src->r);

            if (src->g > src->b) {
                max = &(src->g);
                mid = &(src->b);
            } else {
                max = &(src->b);
                mid = &(src->g);
            }
        }
    }

    t = *max - *min;

    if ( PIXMAN_IS_ZERO(t) ) {
        *mid = *max = 0.0f;
    } else {
        *mid = ( (*mid - *min) * sat ) / t;
        *max = sat;
    }

    *min = 0.0f;
} // set_sat

/* Hue:
 *
 *       as * ad * B(s/as, d/as)
 *     = as * ad * set_lum (set_sat (s/as, SAT (d/ad)), LUM (d/ad), 1)
 *     = set_lum (set_sat (ad * s, as * SAT (d)), as * LUM (d), as * ad)
 *
 */
inline void
blend_hsl_hue (pixman_rgb_t *res,
               const pixman_rgb_t *dest,
               pixman_float_t da,
               const pixman_rgb_t *src,
               pixman_float_t sa)
{
    res->r = src->r * da;
    res->g = src->g * da;
    res->b = src->b * da;

    set_sat(res, get_sat(dest) * sa);
    set_lum(res, sa * da, get_lum(dest) * sa);
}

/*
 * Saturation
 *
 *     as * ad * B(s/as, d/ad)
 *   = as * ad * set_lum (set_sat (d/ad, SAT (s/as)), LUM (d/ad), 1)
 *   = set_lum (as * ad * set_sat (d/ad, SAT (s/as)),
 *                                       as * LUM (d), as * ad)
 *   = set_lum (set_sat (as * d, ad * SAT (s), as * LUM (d), as * ad))
 */
inline void
blend_hsl_saturation (pixman_rgb_t *res,
                      const pixman_rgb_t *dest,
                      pixman_float_t da,
                      const pixman_rgb_t *src,
                      pixman_float_t sa)
{
    res->r = dest->r * sa;
    res->g = dest->g * sa;
    res->b = dest->b * sa;

    set_sat(res, get_sat(src) * da);
    set_lum(res, sa * da, get_lum(dest) * sa);
}

/*
 * Color
 *
 *     as * ad * B(s/as, d/as)
 *   = as * ad * set_lum (s/as, LUM (d/ad), 1)
 *   = set_lum (s * ad, as * LUM (d), as * ad)
 */
inline void
blend_hsl_color (pixman_rgb_t *res,
                 const pixman_rgb_t *dest,
                 pixman_float_t da,
                 const pixman_rgb_t *src,
                 pixman_float_t sa)
{
    res->r = src->r * da;
    res->g = src->g * da;
    res->b = src->b * da;

    set_lum(res, sa * da, get_lum(dest) * sa);
}

/*
 * Luminosity
 *
 *     as * ad * B(s/as, d/ad)
 *   = as * ad * set_lum (d/ad, LUM (s/as), 1)
 *   = set_lum (as * d, ad * LUM (s), as * ad)
 */
inline void
blend_hsl_luminosity (pixman_rgb_t *res,
                      const pixman_rgb_t *dest,
                      pixman_float_t da,
                      const pixman_rgb_t *src,
                      pixman_float_t sa)
{
    res->r = dest->r * sa;
    res->g = dest->g * sa;
    res->b = dest->b * sa;

    set_lum (res, sa * da, get_lum (src) * da);
}

// END
// Code from pixman-combine-float.c
///////////////////////////////////////////////////////////////////////////////

/**
 * @brief Global wrapper templated by the blending operator.
 * A and B are respectively the color of the image A and B and is assumed to of size nComponents, 
 * nComponents being at most 4
 **/
template <MergingFunctionEnum f, typename PIX, int nComponents, int maxValue>
void
mergePixel(bool doAlphaMasking,
           const PIX *A,
           PIX a,
           const PIX *B,
           PIX b,
           PIX* dst)
{
    doAlphaMasking = (f == eMergeMatte) || (doAlphaMasking && isMaskable(f));

    ///When doAlphaMasking is enabled and we're in RGBA the output alpha is set to alphaA+alphaB-alphaA*alphaB
    int maxComp = nComponents;
    if ( !isSeparable(f) ) {
        // HSL modes
        pixman_rgb_t src, dest, res;
        if (PIXMAN_IS_ZERO(a) || nComponents < 3) {
            src.r = src.g = src.b = 0;
        } else {
            src.r = A[0] / (pixman_float_t)a;
            src.g = A[1] / (pixman_float_t)a;
            src.b = A[2] / (pixman_float_t)a;
        }
        if (PIXMAN_IS_ZERO(b) || nComponents < 3) {
            dest.r = dest.g = dest.b = 0;
        } else {
            dest.r = B[0] / (pixman_float_t)b;
            dest.g = B[1] / (pixman_float_t)b;
            dest.b = B[2] / (pixman_float_t)b;
        }
        pixman_float_t sa = a / (pixman_float_t)maxValue;
        pixman_float_t da = b / (pixman_float_t)maxValue;

        switch (f) {
        case eMergeHue: // "SetLum(SetSat(A, Sat(B)), Lum(B))";
            blend_hsl_hue(&res, &dest, da, &src, sa);
            break;

        case eMergeSaturation: // "SetLum(SetSat(B, Sat(A)), Lum(B))";
            blend_hsl_saturation(&res, &dest, da, &src, sa);
            break;

        case eMergeColor: // "SetLum(A, Lum(B))";
            blend_hsl_color(&res, &dest, da, &src, sa);
            break;

        case eMergeLuminosity: // "SetLum(B, Lum(A))";
            blend_hsl_luminosity(&res, &dest, da, &src, sa);
            break;

        case eMergeATop: //"Ab + B(1 - a) (a.k.a. src-atop)";
        case eMergeAverage: // "(A + B) / 2";
        case eMergeColorBurn: // "darken B towards A";
        case eMergeColorDodge: // "brighten B towards A";
        case eMergeConjointOver: // "A + B(1-a)/b, A if a > b";
        case eMergeCopy: // "A (a.k.a. src)";
        case eMergeDifference: // "abs(A-B) (a.k.a. absminus)";
        case eMergeDisjointOver: // "A+B(1-a)/b, A+B if a+b < 1";
        case eMergeDivide: // "A/B, 0 if A < 0 and B < 0";
        case eMergeExclusion: //"A+B-2AB";
        case eMergeFreeze: // "1-sqrt(1-A)/B";
        case eMergeFrom: // "B-A (a.k.a. subtract)";
        case eMergeGeometric: // "2AB/(A+B)";
        case eMergeGrainExtract: // "B - A + 0.5";
        case eMergeGrainMerge: // "B + A - 0.5";
        case eMergeHardLight: // "multiply(2*A, B) if A < 0.5, screen(2*A - 1, B) if A > 0.5";
        case eMergeHypot: // "sqrt(A*A+B*B)";
        case eMergeIn: // "Ab (a.k.a. src-in)";
        case eMergeMask: // "Ba (a.k.a dst-in)";
        case eMergeMatte: //"Aa + B(1-a) (unpremultiplied over)";
        case eMergeMax: // "max(A, B) (a.k.a. lighten only)";
        case eMergeMin: // "min(A, B) (a.k.a. darken only)";
        case eMergeMinus: // "A-B";
        case eMergeMultiply: // "AB, A if A < 0 and B < 0";
        case eMergeOut: // "A(1-b) (a.k.a. src-out)";
        case eMergeOver: //"A+B(1-a) (a.k.a. src-over)";
        case eMergeOverlay: // "multiply(A, 2*B) if B < 0.5, screen(A, 2*B - 1) if B > 0.5";
        case eMergePinLight: // "if B >= 0.5 then max(A, 2*B - 1), min(A, B * 2) else";
        case eMergePlus: //"A+B (a.k.a. add)";
        case eMergeReflect: // "A*A / (1 - B)";
        case eMergeScreen: //"A+B-AB if A or B <= 1, otherwise max(A, B)";
        case eMergeSoftLight: // "burn-in if A < 0.5, lighten if A > 0.5";
        case eMergeStencil: //"B(1-a) (a.k.a. dst-out)";
        case eMergeUnder: //"A(1-b)+B (a.k.a. dst-over)";
        case eMergeXOR: //"A(1-b)+B(1-a)";
        //default: // do not enable the default case, so that we can catch warnings when adding a new operator
            res.r = res.g = res.b = 0;
            assert(false);
            break;
        }
        pixman_float_t R[3] = { res.r, res.g, res.b };
        for (int i = 0; i < (std::min)(nComponents, 3); ++i) {
            dst[i] = PIX( (1 - sa) * B[i] + (1 - da) * A[i] + R[i] * maxValue );
        }
        if (nComponents == 4) {
            dst[3] = PIX(a + b - a * b / (double)maxValue);
        }

        return;
    }

    // separable modes
    if ( doAlphaMasking && (nComponents == 4) ) {
        maxComp = 3;
        dst[3] = PIX(a + b - a * b / (double)maxValue);
    }
    for (int i = 0; i < maxComp; ++i) {
        switch (f) {
        case eMergeATop:
            dst[i] = atopFunc<PIX, maxValue>(A[i], B[i], a, b);
            break;
        case eMergeAverage:
            dst[i] = averageFunc(A[i], B[i]);
            break;
        case eMergeColorBurn:
            dst[i] = colorBurnFunc<PIX, maxValue>(A[i], B[i]);
            break;
        case eMergeColorDodge:
            dst[i] = colorDodgeFunc<PIX, maxValue>(A[i], B[i]);
            break;
        case eMergeConjointOver:
            dst[i] = conjointOverFunc<PIX, maxValue>(A[i], B[i], a, b);
            break;
        case eMergeCopy:
            dst[i] = copyFunc(A[i], B[i]);
            break;
        case eMergeDifference:
            dst[i] = differenceFunc(A[i], B[i]);
            break;
        case eMergeDisjointOver:
            dst[i] = disjointOverFunc<PIX, maxValue>(A[i], B[i], a, b);
            break;
        case eMergeDivide:
            dst[i] = divideFunc(A[i], B[i]);
            break;
        case eMergeExclusion:
            dst[i] = exclusionFunc<PIX, maxValue>(A[i], B[i]);
            break;
        case eMergeFreeze:
            dst[i] = freezeFunc<PIX, maxValue>(A[i], B[i]);
            break;
        case eMergeFrom:
            dst[i] = fromFunc(A[i], B[i]);
            break;
        case eMergeGeometric:
            dst[i] = geometricFunc(A[i], B[i]);
            break;
        case eMergeGrainExtract:
            dst[i] = grainExtractFunc<PIX, maxValue>(A[i], B[i]);
            break;
        case eMergeGrainMerge:
            dst[i] = grainMergeFunc<PIX, maxValue>(A[i], B[i]);
            break;
        case eMergeHardLight:
            dst[i] = hardLightFunc<PIX, maxValue>(A[i], B[i]);
            break;
        case eMergeHypot:
            dst[i] = hypotFunc(A[i], B[i]);
            break;
        case eMergeIn:
            dst[i] = inFunc<PIX, maxValue>(A[i], B[i], a, b);
            break;
        //case eMergeInterpolated:
        //    dst[i] = interpolatedFunc<PIX, maxValue>(A[i], B[i]);
            //    break;
        case eMergeMask:
            dst[i] = maskFunc<PIX, maxValue>(A[i], B[i], a, b);
            break;
        case eMergeMatte:
            dst[i] = matteFunc<PIX, maxValue>(A[i], B[i], a, b);
            break;
        case eMergeMax:
            dst[i] = lightenFunc(A[i], B[i]);
            break;
        case eMergeMin:
            dst[i] = darkenFunc(A[i], B[i]);
            break;
        case eMergeMinus:
            dst[i] = minusFunc(A[i], B[i]);
            break;
        case eMergeMultiply:
            dst[i] = multiplyFunc<PIX, maxValue>(A[i], B[i]);
            break;
        case eMergeOut:
            dst[i] = outFunc<PIX, maxValue>(A[i], B[i], a, b);
            break;
        case eMergeOver:
            dst[i] = overFunc<PIX, maxValue>(A[i], B[i], a, b);
            break;
        case eMergeOverlay:
            dst[i] = overlayFunc<PIX, maxValue>(A[i], B[i]);
            break;
        case eMergePinLight:
            dst[i] = pinLightFunc<PIX, maxValue>(A[i], B[i]);
            break;
        case eMergePlus:
            dst[i] = plusFunc(A[i], B[i]);
            break;
        case eMergeReflect:
            dst[i] = reflectFunc<PIX, maxValue>(A[i], B[i]);
            break;
        case eMergeScreen:
            dst[i] = screenFunc<PIX, maxValue>(A[i], B[i]);
            break;
        case eMergeSoftLight:
            dst[i] = softLightFunc<PIX, maxValue>(A[i], B[i]);
            break;
        case eMergeStencil:
            dst[i] = stencilFunc<PIX, maxValue>(A[i], B[i], a, b);
            break;
        case eMergeUnder:
            dst[i] = underFunc<PIX, maxValue>(A[i], B[i], a, b);
            break;
        case eMergeXOR:
            dst[i] = xorFunc<PIX, maxValue>(A[i], B[i], a, b);
            break;
        default:
            dst[i] = 0;
            assert(false);
            break;
        } // switch
    }
} // mergePixel
} // MergeImages2D
} // OFX


#endif // Misc_Merging_helper_h
