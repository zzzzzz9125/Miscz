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
 * OFX SpriteSheet plugin.
 */

#include <cmath>
#include <cfloat> // DBL_MAX
#include <algorithm>

#include "ofxsProcessing.H"
#include "ofxsCoords.h"
#include "ofxsMacros.h"
#include "ofxsThreadSuite.h"

using namespace OFX;

OFXS_NAMESPACE_ANONYMOUS_ENTER

#define kPluginName "Mz_SpriteSheet"
#define kPluginGrouping "MisczOFX"
#define kPluginDescription "Read individual frames from a sprite sheet. A sprite sheet is a series of images (usually animation frames) combined into a larger image (or images). For example, an animation consisting of eight 100x100 images could be combined into a single 400x200 sprite sheet (4 frames across by 2 high). The sprite with index 0 is at the top-left of the source image, and sprites are ordered left-to-right and top-to-bottom. The output is an animated sprite that repeats the sprites given in the sprite range. The ContactSheet effect can be used to make a spritesheet from a series of images or a video."
#define kPluginIdentifier "net.sf.openfx.MzSpriteSheet"
#define kPluginVersionMajor 2 // Incrementing this number means that you have broken backwards compatibility of the plug-in.
#define kPluginVersionMinor 0 // Increment this when you have fixed a bug or made it faster.

#define kSupportsTiles 1
#define kSupportsMultiResolution 1
#define kSupportsRenderScale 1
#define kSupportsMultipleClipPARs false
#define kSupportsMultipleClipDepths false
#define kRenderThreadSafety eRenderFullySafe

#define kParamSpriteSize "spriteSize"
#define kParamSpriteSizeLabel "Sprite Size", "Size in pixels of an individual sprite."

#define kParamSpriteRange "spriteRange"
#define kParamSpriteRangeLabel "Sprite Range", "Index of the first and last sprite in the animation. The sprite index starts at zero."

#define kParamFrameOffset "frameOffset"
#define kParamFrameOffsetLabel "Frame Offset", "Output frame number for the first sprite as a whole."

#define kParamFrameSeparation "frameSeparation"
#define kParamFrameSeparationLabel "Frame Separation", "The separation of the frames, based on the frame rate of the project."

#define kParamReadingDirection "readingDirection"
#define kParamReadingDirectionLabel "Reading Direction"
#define kParamReadingDirectionHint "The reading direction of the sprites."
#define kParamReadingDirectionOptionHorizontalForward "Horizontal Forward", "Read the sprites in a horizontal forward direction.", "hForward"
#define kParamReadingDirectionOptionHorizontalBackward "Horizontal Backward", "Read the sprites in a horizontal backward direction.", "hBackward"
#define kParamReadingDirectionOptionVerticalForward "Vertical Forward", "Read the sprites in a vertical forward direction.", "vForward"
#define kParamReadingDirectionOptionVerticalBackward "Vertical Backward", "Read the sprites in a vertical backward direction.", "vBackward"
#define kParamReadingDirectionOptionHorizontalForwardS "Horizontal Forward (S-shaped)", "Read the sprites in a horizontal forward S-shape direction.", "hForwardS"
#define kParamReadingDirectionOptionHorizontalBackwardS "Horizontal Backward (S-shaped)", "Read the sprites in a horizontal backward S-shape direction.", "hBackwardS"
#define kParamReadingDirectionOptionVerticalForwardS "Vertical Forward (S-shaped)", "Read the sprites in a vertical forward S-shape direction.", "vForwardS"
#define kParamReadingDirectionOptionVerticalBackwardS "Vertical Backward (S-shaped)", "Read the sprites in a vertical backward S-shape direction.", "vBackwardS"

#define kParamPlaybackMode "playbackMode"
#define kParamPlaybackModeLabel "Playback Mode"
#define kParamPlaybackModeHint "The playback mode when playing the sprites."
#define kParamPlaybackModeOptionNormal "Normal", "Play the sprites in Normal mode.", "normal"
#define kParamPlaybackModeOptionNormalReverse "Normal & Reverse", "Play the sprites in Normal & Reverse mode.", "normalReverse"
#define kParamPlaybackModeOptionNormalReverseMerge "Normal & Reverse (Merge)", "Play the sprites in Normal & Reverse mode, with repeated first/last frames being merged into one frame.", "normalReverseMerge"

#define kParamLoopOffset "loopOffset"
#define kParamLoopOffsetLabel "Loop Offset", "Output frame number for the first sprite in a single loop."

#define kParamRepeatRange "repeatRange"
#define kParamRepeatRangeLabel "Repeat Range", "Delimit a range within which the sprites will be repeated several times. When the Repeat Count is 0, no repeat is performed."

#define kParamRepeatCount "repeatCount"
#define kParamRepeatCountLabel "Repeat Count", "Delimit a range within which the sprites will be repeated several times. When the Repeat Count is 0, no repeat is performed."

#define kParamSpritesCut "spritesCut"
#define kParamSpritesCutLabel "Sprites Cut", "Cut the sprite sheet and read it separately."

enum ReadingDirectionEnum
{
    eReadingDirectionHorizontalForward,
    eReadingDirectionHorizontalBackward,
    eReadingDirectionVerticalForward,
    eReadingDirectionVerticalBackward,
    eReadingDirectionHorizontalForwardS,
    eReadingDirectionHorizontalBackwardS,
    eReadingDirectionVerticalForwardS,
    eReadingDirectionVerticalBackwardS
};

enum PlaybackModeEnum
{
    ePlaybackModeNormal,
    ePlaybackModeNormalReverse,
    ePlaybackModeNormalReverseMerge
};

#ifdef OFX_EXTENSIONS_NATRON
#define OFX_COMPONENTS_OK(c) ((c)== ePixelComponentAlpha || (c) == ePixelComponentXY || (c) == ePixelComponentRGB || (c) == ePixelComponentRGBA)
#else
#define OFX_COMPONENTS_OK(c) ((c)== ePixelComponentAlpha || (c) == ePixelComponentRGB || (c) == ePixelComponentRGBA)
#endif

class SpriteSheetProcessorBase
    : public ImageProcessor
{
protected:
    const Image *_srcImg;
    OfxRectI _cropRectPixel;

public:
    SpriteSheetProcessorBase(ImageEffect &instance)
        : ImageProcessor(instance)
        , _srcImg(NULL)
    {
        _cropRectPixel.x1 = _cropRectPixel.y1 = _cropRectPixel.x2 = _cropRectPixel.y2 = 0;
    }

    /** @brief set the src image */
    void setSrcImg(const Image *v)
    {
        _srcImg = v;
    }

    void setValues(const OfxRectI& cropRectPixel)
    {
        _cropRectPixel = cropRectPixel;
    }
};


template <class PIX, int nComponents, int maxValue>
class SpriteSheetProcessor
    : public SpriteSheetProcessorBase
{
public:
    SpriteSheetProcessor(ImageEffect &instance)
        : SpriteSheetProcessorBase(instance)
    {
    }

private:
    void multiThreadProcessImages(const OfxRectI& procWindow, const OfxPointD& rs) OVERRIDE FINAL
    {
        unused(rs);
        for (int y = procWindow.y1; y < procWindow.y2; ++y) {
            if ( _effect.abort() ) {
                break;
            }

            PIX *dstPix = (PIX *) _dstImg->getPixelAddress(procWindow.x1, y);

            for (int x = procWindow.x1; x < procWindow.x2; ++x, dstPix += nComponents) {
                const PIX *srcPix = _srcImg ? (const PIX*)_srcImg->getPixelAddress(x + _cropRectPixel.x1, y + _cropRectPixel.y1) : 0;
                if (srcPix) {
                    // inside of the rectangle
                    for (int k = 0; k < nComponents; ++k) {
                        dstPix[k] = srcPix[k];
                    }
                } else {
                    for (int k = 0; k < nComponents; ++k) {
                        dstPix[k] = 0;
                    }
                }
            }
        }
    } // multiThreadProcessImages
};

// the modulo oprator (always returns a positive number)
static inline
int
mod(int a, int b)
{
    return a >= 0 ? a % b : ( b - std::abs ( a%b ) ) % b;
}

////////////////////////////////////////////////////////////////////////////////
/** @brief The plugin that does our work */
class SpriteSheetPlugin
    : public ImageEffect
{
public:
    /** @brief ctor */
    SpriteSheetPlugin(OfxImageEffectHandle handle)
        : ImageEffect(handle)
        , _dstClip(NULL)
        , _srcClip(NULL)
        , _spriteSize(NULL)
        , _spriteRange(NULL)
        , _frameOffset(NULL)
        , _frameSeparation(NULL)
        , _readingDirection(NULL)
        , _playbackMode(NULL)
        , _loopOffset(NULL)
        , _repeatRange(NULL)
        , _repeatCount(NULL)
        , _spritesCut(NULL)
    {

        _dstClip = fetchClip(kOfxImageEffectOutputClipName);
        assert( _dstClip && (!_dstClip->isConnected() || _dstClip->getPixelComponents() == ePixelComponentAlpha ||
                             _dstClip->getPixelComponents() == ePixelComponentRGB ||
                             _dstClip->getPixelComponents() == ePixelComponentRGBA) );
        _srcClip = getContext() == eContextGenerator ? NULL : fetchClip(kOfxImageEffectSimpleSourceClipName);
        assert( (!_srcClip && getContext() == eContextGenerator) ||
                ( _srcClip && (!_srcClip->isConnected() || _srcClip->getPixelComponents() ==  ePixelComponentAlpha ||
                               _srcClip->getPixelComponents() == ePixelComponentRGB ||
                               _srcClip->getPixelComponents() == ePixelComponentRGBA) ) );

        _spriteSize = fetchInt2DParam(kParamSpriteSize);
        _spriteRange = fetchInt2DParam(kParamSpriteRange);
        _frameOffset = fetchIntParam(kParamFrameOffset);
        _frameSeparation = fetchIntParam(kParamFrameSeparation);
        _readingDirection = fetchChoiceParam(kParamReadingDirection);
        _playbackMode = fetchChoiceParam(kParamPlaybackMode);
        _loopOffset = fetchIntParam(kParamLoopOffset);
        _repeatRange = fetchInt2DParam(kParamRepeatRange);
        _repeatCount = fetchIntParam(kParamRepeatCount);
        _spritesCut = fetchInt2DParam(kParamSpritesCut);
    }

private:
    // override the roi call
    virtual void getRegionsOfInterest(const RegionsOfInterestArguments &args, RegionOfInterestSetter &rois) OVERRIDE FINAL;
    virtual bool getRegionOfDefinition(const RegionOfDefinitionArguments &args, OfxRectD &rod) OVERRIDE FINAL;

    /* Override the render */
    virtual void render(const RenderArguments &args) OVERRIDE FINAL;

    template <int nComponents>
    void renderInternal(const RenderArguments &args, BitDepthEnum dstBitDepth);

    /* set up and run a processor */
    void setupAndProcess(SpriteSheetProcessorBase &, const RenderArguments &args);

    //virtual void changedParam(const InstanceChangedArgs &args, const std::string &paramName) OVERRIDE FINAL;
    virtual void getClipPreferences(ClipPreferencesSetter &clipPreferences) OVERRIDE FINAL;

    void getCropRectangle(OfxTime time,
                          const OfxPointD& renderScale,
                          const OfxRectI& rodPixel, // RoD in pixel at renderscale = 1
                          const OfxPointI& spriteSize,
                          const OfxPointI& spriteRange,
                          int frameOffset,
                          int frameSeparation,
                          ReadingDirectionEnum readingDirection,
                          PlaybackModeEnum playbackMode,
                          int loopOffset,
                          const OfxPointI& repeatRange,
                          int repeatCount,
                          const OfxPointI& spritesCut,
        OfxRectI *cropRectPixel) const
    {
        bool verticalRead = (readingDirection == eReadingDirectionVerticalForward || readingDirection == eReadingDirectionVerticalBackward || readingDirection == eReadingDirectionVerticalForwardS || readingDirection == eReadingDirectionVerticalBackwardS);
        bool backwardRead = (readingDirection == eReadingDirectionHorizontalBackward || readingDirection == eReadingDirectionVerticalBackward || readingDirection == eReadingDirectionHorizontalBackwardS || readingDirection == eReadingDirectionVerticalBackwardS);
        bool sshapedRead = (readingDirection == eReadingDirectionHorizontalForwardS || readingDirection == eReadingDirectionHorizontalBackwardS || readingDirection == eReadingDirectionVerticalForwardS || readingDirection == eReadingDirectionVerticalBackwardS);
        // number of sprites in the range
        int n = std::abs(spriteRange.y - spriteRange.x) + 1;
        int m = std::abs(repeatRange.y - repeatRange.x) + 1;
        if (playbackMode == ePlaybackModeNormalReverseMerge) {
            n -= 1;
        }
        // sprite index
        int i = mod((int)std::floor(time) / frameSeparation + frameOffset, n + m * repeatCount);
        if ((playbackMode == ePlaybackModeNormalReverse || playbackMode == ePlaybackModeNormalReverseMerge) && mod(((int)std::floor(time) / frameSeparation + frameOffset) / (n + m * repeatCount), 2) == 1) {
            i = n + m * repeatCount - (playbackMode != ePlaybackModeNormalReverseMerge ? 1 : 0) - i;
        }
        i = mod(i + loopOffset, n + (playbackMode == ePlaybackModeNormalReverseMerge ? 1 : 0) + m * repeatCount);
        int j = (i - std::min(repeatRange.x, spriteRange.y)) / m;
        if (j < 0) {
            j = 0;
        }
        i = i - std::min(j, repeatCount) * m;
        if (spriteRange.x <= spriteRange.y) {
            i = spriteRange.x + i;
        } else {
            i = spriteRange.x - i;
        }
        // number of sprites per line
        int cols = verticalRead ? (rodPixel.y2 - rodPixel.y1) / spriteSize.y / spritesCut.y : (rodPixel.x2 - rodPixel.x1) / spriteSize.x / spritesCut.x;
        if (cols <= 0) {
            cols = 1;
        }
        int sum = (rodPixel.y2 - rodPixel.y1) / spriteSize.y / spritesCut.y * (rodPixel.x2 - rodPixel.x1) / spriteSize.x / spritesCut.x;
        if (sum <= 0) {
            sum = 1;
        }
        int r = i / cols % std::max(1, sum / cols);
        int c = i % cols;
        if (backwardRead) {
            c = cols - 1 - c;
        }
        if (sshapedRead) {
            c = (r + spriteRange.x / cols % std::max(1, sum / cols)) % 2 == 0 ? c : (cols - 1 - c);
        }
        int ii = i / sum;
        int colsCrop = verticalRead ? spritesCut.y : spritesCut.x;
        if (colsCrop <= 0) {
            colsCrop = 1;
        }
        int rr = ii / colsCrop;
        int cc = ii % colsCrop;
        if (backwardRead) {
            cc = colsCrop - 1 - cc;
        }
        r += rr * sum / cols;
        c += cc * cols;
        if (verticalRead) {
            int a = r;
            r = c;
            c = a;
        }
        cropRectPixel->x1 = (int)(renderScale.x * (rodPixel.x1 + c * spriteSize.x));
        cropRectPixel->y1 = (int)(renderScale.y * (rodPixel.y2 - (r + 1) * spriteSize.y));
        cropRectPixel->x2 = (int)(renderScale.x * (rodPixel.x1 + (c + 1) * spriteSize.x));
        cropRectPixel->y2 = (int)(renderScale.y * (rodPixel.y2 - r * spriteSize.y));
    } // SpriteSheetPlugin::getCropRectangle

    Clip* getSrcClip() const
    {
        return _srcClip;
    }

private:
    // do not need to delete these, the ImageEffect is managing them for us
    Clip *_dstClip;
    Clip *_srcClip;
    Int2DParam* _spriteSize;
    Int2DParam* _spriteRange;
    IntParam* _frameOffset;
    IntParam* _frameSeparation;
    ChoiceParam* _readingDirection;
    ChoiceParam* _playbackMode;
    IntParam* _loopOffset;
    Int2DParam* _repeatRange;
    IntParam* _repeatCount;
    Int2DParam* _spritesCut;
};


////////////////////////////////////////////////////////////////////////////////
/** @brief render for the filter */

////////////////////////////////////////////////////////////////////////////////
// basic plugin render function, just a skelington to instantiate templates from

/* set up and run a processor */
void
SpriteSheetPlugin::setupAndProcess(SpriteSheetProcessorBase &processor,
                            const RenderArguments &args)
{
    const double time = args.time;

    auto_ptr<Image> dst( _dstClip->fetchImage(args.time) );

    if ( !dst.get() ) {
        throwSuiteStatusException(kOfxStatFailed);
    }
# ifndef NDEBUG
    BitDepthEnum dstBitDepth    = dst->getPixelDepth();
    PixelComponentEnum dstComponents  = dst->getPixelComponents();
    if ( ( dstBitDepth != _dstClip->getPixelDepth() ) ||
         ( dstComponents != _dstClip->getPixelComponents() ) ) {
        setPersistentMessage(Message::eMessageError, "", "OFX Host gave image with wrong depth or components");
        throwSuiteStatusException(kOfxStatFailed);
    }
    checkBadRenderScaleOrField(dst, args);
# endif
    auto_ptr<const Image> src( ( _srcClip && _srcClip->isConnected() ) ?
                                    _srcClip->fetchImage(args.time) : 0 );
    if ( !src.get() || !( _srcClip && _srcClip->isConnected() ) ) {
        // nothing to do
        return;
    } else {
#     ifndef NDEBUG
        checkBadRenderScaleOrField(src, args);
        BitDepthEnum dstBitDepth       = dst->getPixelDepth();
        PixelComponentEnum dstComponents  = dst->getPixelComponents();
        BitDepthEnum srcBitDepth      = src->getPixelDepth();
        PixelComponentEnum srcComponents = src->getPixelComponents();
        if ( (srcBitDepth != dstBitDepth) || (srcComponents != dstComponents) ) {
            throwSuiteStatusException(kOfxStatFailed);
        }
#     endif
    }

    // set the images
    processor.setDstImg( dst.get() );
    processor.setSrcImg( src.get() );

    // set the render window
    processor.setRenderWindow(args.renderWindow, args.renderScale);


    // get the input format (Natron only) or the input RoD (others)
    OfxRectI srcRoDPixel;
    _srcClip->getFormat(srcRoDPixel);
    if ( OFX::Coords::rectIsEmpty(srcRoDPixel) ) {
        // no format is available, use the RoD instead
        OfxRectD srcRoD = _srcClip->getRegionOfDefinition(time);
        double par = _srcClip->getPixelAspectRatio();
        const OfxPointD rs1 = {1., 1.};
        Coords::toPixelNearest(srcRoD, rs1, par, &srcRoDPixel);
    }
    OfxPointI spriteSize;
    _spriteSize->getValueAtTime(time, spriteSize.x, spriteSize.y);
    OfxPointI spriteRange;
    _spriteRange->getValueAtTime(time, spriteRange.x, spriteRange.y);
    int frameOffset = _frameOffset->getValueAtTime(time);
    int frameSeparation = _frameSeparation->getValueAtTime(time);
    ReadingDirectionEnum readingDirection = (ReadingDirectionEnum)_readingDirection->getValueAtTime(time);
    PlaybackModeEnum playbackMode = (PlaybackModeEnum)_playbackMode->getValueAtTime(time);
    int loopOffset = _loopOffset->getValueAtTime(time);
    OfxPointI repeatRange;
    _repeatRange->getValueAtTime(time, repeatRange.x, repeatRange.y);
    int repeatCount = _repeatCount->getValueAtTime(time);

    OfxPointI spritesCut;
    _spritesCut->getValueAtTime(time, spritesCut.x, spritesCut.y);

    OfxRectI cropRectPixel;
    getCropRectangle(time, args.renderScale, srcRoDPixel, spriteSize, spriteRange, frameOffset, frameSeparation, readingDirection, playbackMode, loopOffset, repeatRange, repeatCount, spritesCut, &cropRectPixel);
    processor.setValues(cropRectPixel);

    // Call the base class process member, this will call the derived templated process code
    processor.process();
} // SpriteSheetPlugin::setupAndProcess

// override the roi call
// Required if the plugin requires a region from the inputs which is different from the rendered region of the output.
// (this is the case here)
void
SpriteSheetPlugin::getRegionsOfInterest(const RegionsOfInterestArguments &args,
                                 RegionOfInterestSetter &rois)
{
    const double time = args.time;

    // get the input format (Natron only) or the input RoD (others)
    OfxRectI srcRoDPixel;
    _srcClip->getFormat(srcRoDPixel);
    double par = _srcClip->getPixelAspectRatio();
    if ( OFX::Coords::rectIsEmpty(srcRoDPixel) ) {
        // no format is available, use the RoD instead
        OfxRectD srcRoD = _srcClip->getRegionOfDefinition(time);
        const OfxPointD rs1 = {1., 1.};
        Coords::toPixelNearest(srcRoD, rs1, par, &srcRoDPixel);
    }
    OfxPointI spriteSize;
    _spriteSize->getValueAtTime(time, spriteSize.x, spriteSize.y);
    OfxPointI spriteRange;
    _spriteRange->getValueAtTime(time, spriteRange.x, spriteRange.y);
    int frameOffset = _frameOffset->getValueAtTime(time); 
    int frameSeparation = _frameSeparation->getValueAtTime(time);
    ReadingDirectionEnum readingDirection = (ReadingDirectionEnum)_readingDirection->getValueAtTime(time);
    PlaybackModeEnum playbackMode = (PlaybackModeEnum)_playbackMode->getValueAtTime(time);
    int loopOffset = _loopOffset->getValueAtTime(time);
    OfxPointI repeatRange;
    _repeatRange->getValueAtTime(time, repeatRange.x, repeatRange.y);
    int repeatCount = _repeatCount->getValueAtTime(time);

    OfxPointI spritesCut;
    _spritesCut->getValueAtTime(time, spritesCut.x, spritesCut.y);

    OfxRectI cropRectPixel;
    getCropRectangle(time, args.renderScale, srcRoDPixel, spriteSize, spriteRange, frameOffset, frameSeparation, readingDirection, playbackMode, loopOffset, repeatRange, repeatCount, spritesCut, &cropRectPixel);

    OfxRectD cropRect;
    Coords::toCanonical(cropRectPixel, args.renderScale, par, &cropRect);

    OfxRectD roi = args.regionOfInterest;
    roi.x1 += cropRect.x1;
    roi.y1 += cropRect.y1;
    roi.x2 += cropRect.x1;
    roi.y2 += cropRect.y1;

    rois.setRegionOfInterest(*_srcClip, roi);
}

bool
SpriteSheetPlugin::getRegionOfDefinition(const RegionOfDefinitionArguments &args,
                                  OfxRectD &rod)
{
    const double time = args.time;

    double par = _srcClip->getPixelAspectRatio();
    const OfxPointD rs1 = {1., 1.};
    OfxRectI sprite = {0, 0, 0, 0};
    _spriteSize->getValueAtTime(time, sprite.x2, sprite.y2);

    Coords::toCanonical(sprite, rs1, par, &rod);

    return true;
}

// the internal render function
template <int nComponents>
void
SpriteSheetPlugin::renderInternal(const RenderArguments &args,
                           BitDepthEnum dstBitDepth)
{
    switch (dstBitDepth) {
    case eBitDepthUByte: {
        SpriteSheetProcessor<unsigned char, nComponents, 255> fred(*this);
        setupAndProcess(fred, args);
        break;
    }
    case eBitDepthUShort: {
        SpriteSheetProcessor<unsigned short, nComponents, 65535> fred(*this);
        setupAndProcess(fred, args);
        break;
    }
    case eBitDepthFloat: {
        SpriteSheetProcessor<float, nComponents, 1> fred(*this);
        setupAndProcess(fred, args);
        break;
    }
    default:
        throwSuiteStatusException(kOfxStatErrUnsupported);
    }
}

// the overridden render function
void
SpriteSheetPlugin::render(const RenderArguments &args)
{
    // instantiate the render code based on the pixel depth of the dst clip
    BitDepthEnum dstBitDepth    = _dstClip->getPixelDepth();
    PixelComponentEnum dstComponents  = _dstClip->getPixelComponents();

    assert( kSupportsMultipleClipPARs   || !_srcClip || !_srcClip->isConnected() || _srcClip->getPixelAspectRatio() == _dstClip->getPixelAspectRatio() );
    assert( kSupportsMultipleClipDepths || !_srcClip || !_srcClip->isConnected() || _srcClip->getPixelDepth()       == _dstClip->getPixelDepth() );
    assert(OFX_COMPONENTS_OK(dstComponents));
    if (dstComponents == ePixelComponentRGBA) {
        renderInternal<4>(args, dstBitDepth);
    } else if (dstComponents == ePixelComponentRGB) {
        renderInternal<3>(args, dstBitDepth);
#ifdef OFX_EXTENSIONS_NATRON
    } else if (dstComponents == ePixelComponentXY) {
        renderInternal<2>(args, dstBitDepth);
#endif
    } else {
        assert(dstComponents == ePixelComponentAlpha);
        renderInternal<1>(args, dstBitDepth);
    }
}

void
SpriteSheetPlugin::getClipPreferences(ClipPreferencesSetter &clipPreferences)
{
#ifdef OFX_EXTENSIONS_NATRON
    OfxRectI pixelFormat = {0, 0, 0, 0};
    _spriteSize->getValue(pixelFormat.x2, pixelFormat.y2);
    if ( !Coords::rectIsEmpty(pixelFormat) ) {
        clipPreferences.setOutputFormat(pixelFormat);
    }
#endif
    clipPreferences.setOutputFrameVarying(true); // because output depends on the frame number
}


mDeclarePluginFactory(SpriteSheetPluginFactory, {ofxsThreadSuiteCheck();}, {});

void
SpriteSheetPluginFactory::describe(ImageEffectDescriptor &desc)
{
    // basic labels
    desc.setLabel(kPluginName);
    desc.setPluginGrouping(kPluginGrouping);
    desc.setPluginDescription(kPluginDescription);

    desc.addSupportedContext(eContextGeneral);
    desc.addSupportedContext(eContextFilter);

    desc.addSupportedBitDepth(eBitDepthUByte);
    desc.addSupportedBitDepth(eBitDepthUShort);
    desc.addSupportedBitDepth(eBitDepthFloat);


    desc.setSingleInstance(false);
    desc.setHostFrameThreading(false);
    desc.setTemporalClipAccess(false);
    desc.setRenderTwiceAlways(true);
    desc.setSupportsMultipleClipPARs(kSupportsMultipleClipPARs);
    desc.setSupportsMultipleClipDepths(kSupportsMultipleClipDepths);
    desc.setRenderThreadSafety(kRenderThreadSafety);

    desc.setSupportsTiles(kSupportsTiles);

    // in order to support multiresolution, render() must take into account the pixelaspectratio and the renderscale
    // and scale the transform appropriately.
    // All other functions are usually in canonical coordinates.
    desc.setSupportsMultiResolution(kSupportsMultiResolution);
#ifdef OFX_EXTENSIONS_NUKE
    // ask the host to render all planes
    desc.setPassThroughForNotProcessedPlanes(ePassThroughLevelRenderAllRequestedPlanes);
#endif
#ifdef OFX_EXTENSIONS_NATRON
    desc.setChannelSelector(ePixelComponentNone);
#endif
}

ImageEffect*
SpriteSheetPluginFactory::createInstance(OfxImageEffectHandle handle,
                                  ContextEnum /*context*/)
{
    return new SpriteSheetPlugin(handle);
}

void
SpriteSheetPluginFactory::describeInContext(ImageEffectDescriptor &desc,
                                     ContextEnum /*context*/)
{
    // Source clip only in the filter context
    // create the mandated source clip
    // always declare the source clip first, because some hosts may consider
    // it as the default input clip (e.g. Nuke)
    ClipDescriptor *srcClip = desc.defineClip(kOfxImageEffectSimpleSourceClipName);

    srcClip->addSupportedComponent(ePixelComponentRGBA);
    srcClip->addSupportedComponent(ePixelComponentRGB);
#ifdef OFX_EXTENSIONS_NATRON
    srcClip->addSupportedComponent(ePixelComponentXY);
#endif
    srcClip->addSupportedComponent(ePixelComponentAlpha);
    srcClip->setTemporalClipAccess(false);
    srcClip->setSupportsTiles(kSupportsTiles);
    srcClip->setIsMask(false);

    // create the mandated output clip
    ClipDescriptor *dstClip = desc.defineClip(kOfxImageEffectOutputClipName);
    dstClip->addSupportedComponent(ePixelComponentRGBA);
    dstClip->addSupportedComponent(ePixelComponentRGB);
#ifdef OFX_EXTENSIONS_NATRON
    dstClip->addSupportedComponent(ePixelComponentXY);
#endif
    dstClip->addSupportedComponent(ePixelComponentAlpha);
    dstClip->setSupportsTiles(kSupportsTiles);

    // make some pages and to things in
    PageParamDescriptor *page = desc.definePageParam("Controls");

    {
        Int2DParamDescriptor* param = desc.defineInt2DParam(kParamSpriteSize);
        param->setLabelAndHint(kParamSpriteSizeLabel);
        param->setRange(1, 1, INT_MAX, INT_MAX);
        param->setDisplayRange(1, 1, 512, 512);
        param->setDefault(64, 64);
        param->setAnimates(false);
#ifdef OFX_EXTENSIONS_NATRON
        desc.addClipPreferencesSlaveParam(*param);
#endif
        if (page) {
            page->addChild(*param);
        }
    }
    {
        Int2DParamDescriptor* param = desc.defineInt2DParam(kParamSpriteRange);
        param->setLabelAndHint(kParamSpriteRangeLabel);
        param->setRange(0, 0, INT_MAX, INT_MAX);
        param->setDefault(0, 0);
        param->setDimensionLabels("first", "last");
        if (page) {
            page->addChild(*param);
        }
    }
    {
        IntParamDescriptor* param = desc.defineIntParam(kParamFrameOffset);
        param->setLabelAndHint(kParamFrameOffsetLabel);
        param->setRange(INT_MIN, INT_MAX);
        param->setDefault(0);
        if (page) {
            page->addChild(*param);
        }
    }
    {
        IntParamDescriptor* param = desc.defineIntParam(kParamFrameSeparation);
        param->setLabelAndHint(kParamFrameSeparationLabel);
        param->setRange(1, INT_MAX);
        param->setDefault(1);
        if (page) {
            page->addChild(*param);
        }
    }
    {
        ChoiceParamDescriptor* param = desc.defineChoiceParam(kParamReadingDirection);
        param->setLabel(kParamReadingDirectionLabel);
        param->setHint(kParamReadingDirectionHint);
        assert(param->getNOptions() == eReadingDirectionHorizontalForward);
        param->appendOption(kParamReadingDirectionOptionHorizontalForward);
        assert(param->getNOptions() == eReadingDirectionHorizontalBackward);
        param->appendOption(kParamReadingDirectionOptionHorizontalBackward);
        assert(param->getNOptions() == eReadingDirectionVerticalForward);
        param->appendOption(kParamReadingDirectionOptionVerticalForward);
        assert(param->getNOptions() == eReadingDirectionVerticalBackward);
        param->appendOption(kParamReadingDirectionOptionVerticalBackward);
        assert(param->getNOptions() == eReadingDirectionHorizontalForwardS);
        param->appendOption(kParamReadingDirectionOptionHorizontalForwardS);
        assert(param->getNOptions() == eReadingDirectionHorizontalBackwardS);
        param->appendOption(kParamReadingDirectionOptionHorizontalBackwardS);
        assert(param->getNOptions() == eReadingDirectionVerticalForwardS);
        param->appendOption(kParamReadingDirectionOptionVerticalForwardS);
        assert(param->getNOptions() == eReadingDirectionVerticalBackwardS);
        param->appendOption(kParamReadingDirectionOptionVerticalBackwardS);
        param->setAnimates(true);
        param->setDefault(eReadingDirectionHorizontalForward);
        if (page) {
            page->addChild(*param);
        }
    }
    {
        ChoiceParamDescriptor* param = desc.defineChoiceParam(kParamPlaybackMode);
        param->setLabel(kParamPlaybackModeLabel);
        param->setHint(kParamPlaybackModeHint);
        assert(param->getNOptions() == ePlaybackModeNormal);
        param->appendOption(kParamPlaybackModeOptionNormal);
        assert(param->getNOptions() == ePlaybackModeNormalReverse);
        param->appendOption(kParamPlaybackModeOptionNormalReverse);
        assert(param->getNOptions() == ePlaybackModeNormalReverseMerged);
        param->appendOption(kParamPlaybackModeOptionNormalReverseMerge);
        param->setAnimates(true);
        param->setDefault(ePlaybackModeNormal);
        if (page) {
            page->addChild(*param);
        }
    }
    {
        IntParamDescriptor* param = desc.defineIntParam(kParamLoopOffset);
        param->setLabelAndHint(kParamLoopOffsetLabel);
        param->setRange(INT_MIN, INT_MAX);
        param->setDefault(0);
        if (page) {
            page->addChild(*param);
        }
    }
    {
        Int2DParamDescriptor* param = desc.defineInt2DParam(kParamRepeatRange);
        param->setLabelAndHint(kParamRepeatRangeLabel);
        param->setRange(0, 0, INT_MAX, INT_MAX);
        param->setDefault(0, 0);
        param->setDimensionLabels("start", "end");
        if (page) {
            page->addChild(*param);
        }
    }
    {
        IntParamDescriptor* param = desc.defineIntParam(kParamRepeatCount);
        param->setLabelAndHint(kParamRepeatCountLabel);
        param->setRange(INT_MIN, INT_MAX);
        param->setDefault(0);
        if (page) {
            page->addChild(*param);
        }
    }
    {
        Int2DParamDescriptor* param = desc.defineInt2DParam(kParamSpritesCut);
        param->setLabelAndHint(kParamSpritesCutLabel);
        param->setRange(1, 1, INT_MAX, INT_MAX);
        param->setDefault(1, 1);
        param->setDimensionLabels("cutX", "cutY");
        if (page) {
            page->addChild(*param);
        }
    }
} // SpriteSheetPluginFactory::describeInContext

static SpriteSheetPluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
mRegisterPluginFactoryInstance(p)

OFXS_NAMESPACE_ANONYMOUS_EXIT
