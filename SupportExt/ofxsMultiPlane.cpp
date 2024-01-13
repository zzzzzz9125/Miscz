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
 * Helper functions to implement plug-ins that support kFnOfxImageEffectPlaneSuite v2
 * In order to use these functions the following condition must be met:
 *#if defined(OFX_EXTENSIONS_NUKE) && defined(OFX_EXTENSIONS_NATRON)

   if (fetchSuite(kFnOfxImageEffectPlaneSuite, 2) &&  // for clipGetImagePlane
   getImageEffectHostDescription()->supportsDynamicChoices && // for dynamic layer choices
   getImageEffectHostDescription()->isMultiPlanar) // for clipGetImagePlane
   ... this is ok...
 *#endif
 */
#include "ofxsMultiPlane.h"

#include <algorithm>
#include <set>

using namespace OFX;

using std::vector;
using std::string;
using std::map;
using std::set;

static bool gHostSupportsMultiPlaneV1 = false;
static bool gHostSupportsMultiPlaneV2 = false;
static bool gHostSupportsDynamicChoices = false;
static bool gHostIsNatron3OrGreater = false;

static const char* rgbaComps[4] = {"R", "G", "B", "A"};
static const char* rgbComps[3] = {"R", "G", "B"};
static const char* alphaComps[1] = {"A"};
static const char* motionComps[2] = {"U", "V"};
static const char* disparityComps[2] = {"X", "Y"};
static const char* xyComps[2] = {"X", "Y"};

namespace OFX {
namespace MultiPlane {


ImagePlaneDesc::ImagePlaneDesc()
: _planeID("none")
, _planeLabel("none")
, _channels()
, _channelsLabel("none")
{
}

ImagePlaneDesc::ImagePlaneDesc(const std::string& planeID,
                               const std::string& planeLabel,
                               const std::string& channelsLabel,
                               const std::vector<std::string>& channels)
: _planeID(planeID)
, _planeLabel(planeLabel)
, _channels(channels)
, _channelsLabel(channelsLabel)
{
    if (planeLabel.empty()) {
        // Plane label is the ID if empty
        _planeLabel = _planeID;
    }
    if ( channelsLabel.empty() ) {
        // Channels label is the concatenation of all channels
        for (std::size_t i = 0; i < channels.size(); ++i) {
            _channelsLabel.append(channels[i]);
        }
    }
}

ImagePlaneDesc::ImagePlaneDesc(const std::string& planeName,
                               const std::string& planeLabel,
                               const std::string& channelsLabel,
                               const char** channels,
                               int count)
: _planeID(planeName)
, _planeLabel(planeLabel)
, _channels()
, _channelsLabel(channelsLabel)
{
    _channels.resize(count);
    for (int i = 0; i < count; ++i) {
        _channels[i] = channels[i];
    }

    if (planeLabel.empty()) {
        // Plane label is the ID if empty
        _planeLabel = _planeID;
    }
    if ( channelsLabel.empty() ) {
        // Channels label is the concatenation of all channels
        for (std::size_t i = 0; i < _channels.size(); ++i) {
            _channelsLabel.append(channels[i]);
        }
    }
}

ImagePlaneDesc::ImagePlaneDesc(const ImagePlaneDesc& other)
{
    *this = other;
}

ImagePlaneDesc&
ImagePlaneDesc::operator=(const ImagePlaneDesc& other)
{
    _planeID = other._planeID;
    _planeLabel = other._planeLabel;
    _channels = other._channels;
    _channelsLabel = other._channelsLabel;
    return *this;
}

ImagePlaneDesc::~ImagePlaneDesc()
{
}

bool
ImagePlaneDesc::isColorPlane(const std::string& planeID)
{
    return planeID == kOfxMultiplaneColorPlaneID;
}

bool
ImagePlaneDesc::isColorPlane() const
{
    return ImagePlaneDesc::isColorPlane(_planeID);
}



bool
ImagePlaneDesc::operator==(const ImagePlaneDesc& other) const
{
    if ( _channels.size() != other._channels.size() ) {
        return false;
    }
    return _planeID == other._planeID;
}

bool
ImagePlaneDesc::operator<(const ImagePlaneDesc& other) const
{
    return _planeID < other._planeID;
}

int
ImagePlaneDesc::getNumComponents() const
{
    return (int)_channels.size();
}

const std::string&
ImagePlaneDesc::getPlaneID() const
{
    return _planeID;
}

const std::string&
ImagePlaneDesc::getPlaneLabel() const
{
    return _planeLabel;
}

const std::string&
ImagePlaneDesc::getChannelsLabel() const
{
    return _channelsLabel;
}

const std::vector<std::string>&
ImagePlaneDesc::getChannels() const
{
    return _channels;
}

const ImagePlaneDesc&
ImagePlaneDesc::getNoneComponents()
{
    static const ImagePlaneDesc comp;
    return comp;
}

const ImagePlaneDesc&
ImagePlaneDesc::getRGBAComponents()
{
    static const ImagePlaneDesc comp(kOfxMultiplaneColorPlaneID, kOfxMultiplaneColorPlaneLabel, "", rgbaComps, 4);

    return comp;
}

const ImagePlaneDesc&
ImagePlaneDesc::getRGBComponents()
{
    static const ImagePlaneDesc comp(kOfxMultiplaneColorPlaneID, kOfxMultiplaneColorPlaneLabel, "", rgbComps, 3);

    return comp;
}


const ImagePlaneDesc&
ImagePlaneDesc::getXYComponents()
{
    static const ImagePlaneDesc comp(kOfxMultiplaneColorPlaneID, kOfxMultiplaneColorPlaneLabel, "XY", xyComps, 2);

    return comp;
}

const ImagePlaneDesc&
ImagePlaneDesc::getAlphaComponents()
{
    static const ImagePlaneDesc comp(kOfxMultiplaneColorPlaneID, kOfxMultiplaneColorPlaneLabel, "Alpha", alphaComps, 1);

    return comp;
}

const ImagePlaneDesc&
ImagePlaneDesc::getBackwardMotionComponents()
{
    static const ImagePlaneDesc comp(kOfxMultiplaneBackwardMotionVectorsPlaneID, kOfxMultiplaneBackwardMotionVectorsPlaneLabel, kOfxMultiplaneMotionComponentsLabel, motionComps, 2);

    return comp;
}

const ImagePlaneDesc&
ImagePlaneDesc::getForwardMotionComponents()
{
    static const ImagePlaneDesc comp(kOfxMultiplaneForwardMotionVectorsPlaneID, kOfxMultiplaneForwardMotionVectorsPlaneLabel, kOfxMultiplaneMotionComponentsLabel, motionComps, 2);

    return comp;
}

const ImagePlaneDesc&
ImagePlaneDesc::getDisparityLeftComponents()
{
    static const ImagePlaneDesc comp(kOfxMultiplaneDisparityLeftPlaneID, kOfxMultiplaneDisparityLeftPlaneLabel, kOfxMultiplaneDisparityComponentsLabel, disparityComps, 2);

    return comp;
}

const ImagePlaneDesc&
ImagePlaneDesc::getDisparityRightComponents()
{
    static const ImagePlaneDesc comp(kOfxMultiplaneDisparityRightPlaneID, kOfxMultiplaneDisparityRightPlaneLabel, kOfxMultiplaneDisparityComponentsLabel, disparityComps, 2);

    return comp;
}


void
ImagePlaneDesc::getChannelOption(int channelIndex, std::string* optionID, std::string* optionLabel) const
{
    if (channelIndex < 0 || channelIndex >= (int)_channels.size()) {
        assert(false);
        return;
    }

    *optionLabel += _planeLabel;
    *optionID += _planeID;
    if ( !optionLabel->empty() ) {
        *optionLabel += '.';
    }
    if (!optionID->empty()) {
        *optionID += '.';
    }

    // For the option label, append the name of the channel
    *optionLabel += _channels[channelIndex];
    *optionID += _channels[channelIndex];
}

void
ImagePlaneDesc::getPlaneOption(std::string* optionID, std::string* optionLabel) const
{
    // The option ID is always the name of the layer, this ensures for the Color plane that even if the components type changes, the choice stays
    // the same in the parameter.
    *optionLabel = _planeLabel + "." + _channelsLabel;
    *optionID = _planeID;
}

const ImagePlaneDesc&
ImagePlaneDesc::mapNCompsToColorPlane(int nComps)
{
    switch (nComps) {
        case 1:
            return ImagePlaneDesc::getAlphaComponents();
        case 2:
            return ImagePlaneDesc::getXYComponents();
        case 3:
            return ImagePlaneDesc::getRGBComponents();
        case 4:
            return ImagePlaneDesc::getRGBAComponents();
        default:
            return ImagePlaneDesc::getNoneComponents();
    }
}

static ImagePlaneDesc
ofxCustomCompToNatronComp(const std::string& comp)
{
    std::string planeID, planeLabel, channelsLabel;
    std::vector<std::string> channels;
    if (!extractCustomPlane(comp, &planeID, &planeLabel, &channelsLabel, &channels)) {
        return ImagePlaneDesc::getNoneComponents();
    }

    return ImagePlaneDesc(planeID, planeLabel, channelsLabel, channels);
}

ImagePlaneDesc
ImagePlaneDesc::mapOFXPlaneStringToPlane(const std::string& ofxPlane)
{
    assert(ofxPlane != kFnOfxImagePlaneColour);
    if (ofxPlane == kFnOfxImagePlaneBackwardMotionVector) {
        return ImagePlaneDesc::getBackwardMotionComponents();
    } else if (ofxPlane == kFnOfxImagePlaneForwardMotionVector) {
        return ImagePlaneDesc::getForwardMotionComponents();
    } else if (ofxPlane == kFnOfxImagePlaneStereoDisparityLeft) {
        return ImagePlaneDesc::getDisparityLeftComponents();
    } else if (ofxPlane == kFnOfxImagePlaneStereoDisparityRight) {
        return ImagePlaneDesc::getDisparityRightComponents();
    } else {
        return ofxCustomCompToNatronComp(ofxPlane);
    }
}

void
ImagePlaneDesc::mapOFXComponentsTypeStringToPlanes(const std::string& ofxComponents, ImagePlaneDesc* plane, ImagePlaneDesc* pairedPlane)
{
    if (ofxComponents ==  kOfxImageComponentRGBA) {
        *plane = ImagePlaneDesc::getRGBAComponents();
    } else if (ofxComponents == kOfxImageComponentAlpha) {
        *plane = ImagePlaneDesc::getAlphaComponents();
    } else if (ofxComponents == kOfxImageComponentRGB) {
        *plane = ImagePlaneDesc::getRGBComponents();
    }else if (ofxComponents == kNatronOfxImageComponentXY) {
        *plane = ImagePlaneDesc::getXYComponents();
    } else if (ofxComponents == kOfxImageComponentNone) {
        *plane = ImagePlaneDesc::getNoneComponents();
    } else if (ofxComponents == kFnOfxImageComponentMotionVectors) {
        *plane = ImagePlaneDesc::getBackwardMotionComponents();
        *pairedPlane = ImagePlaneDesc::getForwardMotionComponents();
    } else if (ofxComponents == kFnOfxImageComponentStereoDisparity) {
        *plane = ImagePlaneDesc::getDisparityLeftComponents();
        *pairedPlane = ImagePlaneDesc::getDisparityRightComponents();
    } else {
        *plane = ofxCustomCompToNatronComp(ofxComponents);
    }

} // mapOFXComponentsTypeStringToPlanes


static std::string
natronCustomCompToOfxComp(const ImagePlaneDesc &comp)
{
    std::stringstream ss;
    const std::vector<std::string>& channels = comp.getChannels();
    const std::string& planeID = comp.getPlaneID();
    const std::string& planeLabel = comp.getPlaneLabel();
    const std::string& channelsLabel = comp.getChannelsLabel();
    ss << kNatronOfxImageComponentsPlaneName << planeID;
    if (!planeLabel.empty()) {
        ss << kNatronOfxImageComponentsPlaneLabel << planeLabel;
    }
    if (!channelsLabel.empty()) {
        ss << kNatronOfxImageComponentsPlaneChannelsLabel << channelsLabel;
    }
    for (std::size_t i = 0; i < channels.size(); ++i) {
        ss << kNatronOfxImageComponentsPlaneChannel << channels[i];
    }

    return ss.str();
} // natronCustomCompToOfxComp


std::string
ImagePlaneDesc::mapPlaneToOFXPlaneString(const ImagePlaneDesc& plane)
{
    if (plane.isColorPlane()) {
        return kFnOfxImagePlaneColour;
    } else if ( plane == ImagePlaneDesc::getBackwardMotionComponents() ) {
        return kFnOfxImagePlaneBackwardMotionVector;
    } else if ( plane == ImagePlaneDesc::getForwardMotionComponents()) {
        return kFnOfxImagePlaneForwardMotionVector;
    } else if ( plane == ImagePlaneDesc::getDisparityLeftComponents()) {
        return kFnOfxImagePlaneStereoDisparityLeft;
    } else if ( plane == ImagePlaneDesc::getDisparityRightComponents() ) {
        return kFnOfxImagePlaneStereoDisparityRight;
    } else {
        return natronCustomCompToOfxComp(plane);
    }

}

std::string
ImagePlaneDesc::mapPlaneToOFXComponentsTypeString(const ImagePlaneDesc& plane)
{
    if ( plane == ImagePlaneDesc::getNoneComponents() ) {
        return kOfxImageComponentNone;
    } else if ( plane == ImagePlaneDesc::getAlphaComponents() ) {
        return kOfxImageComponentAlpha;
    } else if ( plane == ImagePlaneDesc::getRGBComponents() ) {
        return kOfxImageComponentRGB;
    } else if ( plane == ImagePlaneDesc::getRGBAComponents() ) {
        return kOfxImageComponentRGBA;
    } else if ( plane == ImagePlaneDesc::getXYComponents() ) {
        return kNatronOfxImageComponentXY;
    } else if ( plane == ImagePlaneDesc::getBackwardMotionComponents() ||
               plane == ImagePlaneDesc::getForwardMotionComponents()) {
        return kFnOfxImageComponentMotionVectors;
    } else if ( plane == ImagePlaneDesc::getDisparityLeftComponents() ||
               plane == ImagePlaneDesc::getDisparityRightComponents()) {
        return kFnOfxImageComponentStereoDisparity;
    } else {
        return natronCustomCompToOfxComp(plane);
    }
}

} // namespace MultiPlane
} // namespace OFX

namespace  {

void
getHardCodedPlanes(bool onlyColorPlane, std::vector<const MultiPlane::ImagePlaneDesc*>* planesToAdd)
{
    const MultiPlane::ImagePlaneDesc& rgbaPlane = MultiPlane::ImagePlaneDesc::getRGBAComponents();
    const MultiPlane::ImagePlaneDesc& disparityLeftPlane = MultiPlane::ImagePlaneDesc::getDisparityLeftComponents();
    const MultiPlane::ImagePlaneDesc& disparityRightPlane = MultiPlane::ImagePlaneDesc::getDisparityRightComponents();
    const MultiPlane::ImagePlaneDesc& motionBwPlane = MultiPlane::ImagePlaneDesc::getBackwardMotionComponents();
    const MultiPlane::ImagePlaneDesc& motionFwPlane = MultiPlane::ImagePlaneDesc::getForwardMotionComponents();

    planesToAdd->push_back(&rgbaPlane);
    if (!onlyColorPlane) {
        planesToAdd->push_back(&disparityLeftPlane);
        planesToAdd->push_back(&disparityRightPlane);
        planesToAdd->push_back(&motionBwPlane);
        planesToAdd->push_back(&motionFwPlane);
    }

}

struct ChoiceOption
{
    string name, label, hint;

};

struct ChoiceOption_Compare
{
    bool operator() (const ChoiceOption& lhs, const ChoiceOption& rhs) const
    {
        return lhs.name < rhs.name;
    }
};

void
getHardCodedPlaneOptions(const vector<string>& clips,
                         bool addConstants,
                         bool onlyColorPlane,
                         vector<ChoiceOption>* options)
{


    std::vector<const MultiPlane::ImagePlaneDesc*> planesToAdd;
    getHardCodedPlanes(onlyColorPlane, &planesToAdd);

    for (std::size_t c = 0; c < clips.size(); ++c) {
        const string& clipName = clips[c];

        for (std::size_t p = 0; p < planesToAdd.size(); ++p) {
            const std::string& planeLabel = planesToAdd[p]->getPlaneLabel();

            const std::vector<std::string>& planeChannels = planesToAdd[p]->getChannels();

            for (std::size_t i = 0; i < planeChannels.size(); ++i) {
                ChoiceOption option;

                // Prefix the clip name if there are multiple clip channels to read from
                if (clips.size() > 1) {
                    option.name.append(clipName);
                    option.name.push_back('.');
                    option.label.append(clipName);
                    option.label.push_back('.');
                }

                // Prefix the plane name
                option.name.append(planesToAdd[p]->getPlaneID());
                option.name.push_back('.');
                option.label.append(planeLabel);
                option.label.push_back('.');


                option.name.append(planeChannels[i]);
                option.label.append(planeChannels[i]);

                // Make up some tooltip
                option.hint.append(planeChannels[i]);
                option.hint.append(" channel from input ");
                option.hint.append(clipName);
                options->push_back(option);

            }
        }

        if ( addConstants && (c == 0) ) {
            {
                string opt, hint;
                opt.append(kMultiPlaneChannelParamOption0);
                hint.append(kMultiPlaneChannelParamOption0Hint);

                ChoiceOption choice = {opt, opt, hint};
                options->push_back(choice);
            }
            {
                string opt, hint;
                opt.append(kMultiPlaneChannelParamOption1);
                hint.append(kMultiPlaneChannelParamOption1Hint);

                ChoiceOption choice = {opt, opt, hint};
                options->push_back(choice);
            }
        }
    }

} // getHardCodedPlanes

template <typename T>
void
addInputChannelOptionsRGBAInternal(T* param,
                                   const vector<string>& clips,
                                   bool addConstants,
                                   bool onlyColorPlane,
                                   vector<ChoiceOption>* optionsParam)
{
    vector<ChoiceOption> options;
    getHardCodedPlaneOptions(clips, addConstants, onlyColorPlane, &options);
    if (optionsParam) {
        *optionsParam = options;
    }

    if (param) {
        for (std::size_t i = 0; i < options.size(); ++i) {
            param->appendOption(options[i].label, options[i].hint, options[i].name);
        }
    }
} // addInputChannelOptionsRGBAInternal


} // anonymous namespace

namespace OFX {
namespace MultiPlane {

namespace Factory {
void
addInputChannelOptionsRGBA(ChoiceParamDescriptor* param,
                           const vector<string>& clips,
                           bool addConstants,
                           bool onlyColorPlane)
{
    addInputChannelOptionsRGBAInternal<ChoiceParamDescriptor>(param, clips, addConstants, onlyColorPlane, 0);
}

void
addInputChannelOptionsRGBA(const vector<string>& clips,
                           bool addConstants,
                           bool onlyColorPlane)
{
    addInputChannelOptionsRGBAInternal<ChoiceParam>(0, clips, addConstants, onlyColorPlane, 0);
}
}         // factory


/**
 * @brief For each choice param, the list of clips it "depends on" (that is the clip available planes that will be visible in the choice)
 **/
struct ChoiceParamClips
{
    // The choice parameter containing the planes or channels.
    ChoiceParam* param;

    // True if the menu should contain any entry for each channel of each plane
    bool splitPlanesIntoChannels;

    // True if we should add a "None" option
    bool addNoneOption;

    // True if we should add 0 and 1 options
    bool addConstantOptions;

    bool isOutput;

    bool hideIfClipDisconnected;

    vector<Clip*> clips;
    vector<string> clipNames;

    ChoiceParamClips()
    : param(NULL)
    , splitPlanesIntoChannels(false)
    , addNoneOption(false)
    , addConstantOptions(false)
    , isOutput(false)
    , hideIfClipDisconnected(false)
    , clips()
    , clipNames()

    {
    }
};



struct MultiPlaneEffectPrivate
{
    // Pointer to the public interface
    MultiPlaneEffect* _publicInterface;

    // A map of each dynamic choice parameters containing planes/channels
    map<string, ChoiceParamClips> params;

    // If true, all planes have to be processed
    BooleanParam* allPlanesCheckbox;

    Clip* dstClip;

    // Stores for each clip its available planes
    // This is to avoid a recursion when calling getPlanesPresent
    // on the output clip.
    std::map<Clip*, std::list<ImagePlaneDesc> > perClipPlanesAvailable;

    MultiPlaneEffectPrivate(MultiPlaneEffect* publicInterface)
    : _publicInterface(publicInterface)
    , params()
    , allPlanesCheckbox(NULL)
    , dstClip(publicInterface->fetchClip(kOfxImageEffectOutputClipName))
    , perClipPlanesAvailable()
    {
    }

    /**
     * @brief The instanceChanged handler for the "All Planes" checkbox if the parameter was defined with
     **/
    void handleAllPlanesCheckboxParamChanged();

    /**
     * @brief To be called in createInstance and clipChanged to refresh visibility of input channel/plane selectors.
     **/
    void refreshSelectorsVisibility();


    /**
     * @brief Rebuild all choice parameters depending on the clips planes present.
     * This function is supposed to be called in the clipChanged action on the output clip.
     **/
    void buildChannelMenus();
};

MultiPlaneEffect::MultiPlaneEffect(OfxImageEffectHandle handle)
    : ImageEffect(handle)
    , _imp( new MultiPlaneEffectPrivate(this) )
{
}

MultiPlaneEffect::~MultiPlaneEffect()
{
}

void
MultiPlaneEffect::fetchDynamicMultiplaneChoiceParameter(const string& paramName,
                                                        const FetchChoiceParamOptions& args)
{
    ChoiceParamClips& paramData = _imp->params[paramName];

    paramData.param = fetchChoiceParam(paramName);
    paramData.splitPlanesIntoChannels = args.splitPlanesIntoChannelOptions;
    paramData.addNoneOption = args.addNoneOption;
    paramData.addConstantOptions = args.addConstantOptions;
    paramData.clips = args.dependsClips;

    for (std::size_t i = 0; i < args.dependsClips.size(); ++i) {
        // A choice menu cannot depend on the planes present on an output clip, since we actually may need the value
        // of the choice to return the planes present in output!
        assert(args.dependsClips[i]->name() != kOfxImageEffectOutputClipName);
        paramData.clipNames.push_back( args.dependsClips[i]->name() );
    }

    paramData.isOutput = args.isOutputPlaneChoice;
    paramData.hideIfClipDisconnected = args.hideIfClipDisconnected;

    if (args.isOutputPlaneChoice && !_imp->allPlanesCheckbox && paramExists(kMultiPlaneProcessAllPlanesParam)) {
        _imp->allPlanesCheckbox = fetchBooleanParam(kMultiPlaneProcessAllPlanesParam);
    }

    if (_imp->allPlanesCheckbox) {
        bool allPlanesSelected = _imp->allPlanesCheckbox->getValue();
        paramData.param->setIsSecretAndDisabled(allPlanesSelected);
    }

}



void
MultiPlaneEffectPrivate::buildChannelMenus()
{
    perClipPlanesAvailable.clear();

    // If no dynamic choices support, only add built-in planes.
    if (!gHostSupportsDynamicChoices) {
        vector<const MultiPlane::ImagePlaneDesc*> planesToAdd;
        getHardCodedPlanes(!gHostSupportsMultiPlaneV1, &planesToAdd);

        for (map<string, ChoiceParamClips>::iterator it = params.begin(); it != params.end(); ++it) {
            for (std::size_t c = 0; c < it->second.clips.size(); ++c) {

                // For the output plane selector, map the clip planes against the output clip even though the user provided a
                // source clip as pass-through clip
                Clip* clip = it->second.isOutput ? dstClip : it->second.clips[c];
                map<Clip*,  std::list<ImagePlaneDesc> >::iterator foundClip = perClipPlanesAvailable.find(clip);
                if (foundClip != perClipPlanesAvailable.end()) {
                    continue;
                } else {
                    std::list<ImagePlaneDesc>& clipPlanes = perClipPlanesAvailable[clip];
                    for (vector<const MultiPlane::ImagePlaneDesc*>::const_iterator it2 = planesToAdd.begin(); it2 != planesToAdd.end(); ++it2) {
                        clipPlanes.push_back(**it2);
                    }
                }
            }
        }
        return;
    }
    
    // This code requires dynamic choice parameters support.


    // For each parameter to refresh
    std::vector<std::pair<ChoiceParam*,std::vector<ChoiceOption> > > perParamOptions;
    for (map<string, ChoiceParamClips>::iterator it = params.begin(); it != params.end(); ++it) {

        vector<ChoiceOption> options;
        set<ChoiceOption, ChoiceOption_Compare> optionsSorted;

        if (it->second.splitPlanesIntoChannels) {
            // Add built-in hard-coded options A.R, A.G, ... 0, 1, B.R, B.G ...
            getHardCodedPlaneOptions(it->second.clipNames, it->second.addConstantOptions, true /*onlyColorPlane*/, &options);
            optionsSorted.insert(options.begin(), options.end());
        } else {
            // For plane selectors, we might want a "None" option to select an input plane.
            if (it->second.addNoneOption) {
                ChoiceOption opt = {kMultiPlanePlaneParamOptionNone, kMultiPlanePlaneParamOptionNoneLabel, ""};
                options.push_back(opt);
                optionsSorted.insert(opt);
            }
        }

        // We don't use a map here to keep the clips in the order of what the user passed them in fetchDynamicMultiplaneChoiceParameter
        std::list<std::pair<Clip*, std::list<ImagePlaneDesc>* > > perClipPlanes;
        for (std::size_t c = 0; c < it->second.clips.size(); ++c) {

            Clip* clip = it->second.clips[c];

            // Did we fetch the clip available planes already ? This speeds it up in the case where we have multiple choice parameters
            // accessing the same clip.
            std::list<ImagePlaneDesc>* availableClipPlanes = 0;

            // For the output plane selector, map the clip planes against the output clip even though the user provided a
            // source clip as pass-through clip so the extraneous planes returned by getExtraneousPlanesCreated are not added for
            // the available planes on the source clip
            Clip* clipToMap = it->second.isOutput ? dstClip : clip;

            map<Clip*,  std::list<ImagePlaneDesc> >::iterator foundClip = perClipPlanesAvailable.find(clipToMap);
            if (foundClip != perClipPlanesAvailable.end()) {
                availableClipPlanes = &foundClip->second;
            } else {

                availableClipPlanes = &(perClipPlanesAvailable)[clipToMap];

                // Fetch planes presents from the clip and map them to ImagePlaneDesc
                // Note that the clip cannot bethe output clip: the host may call recursively the getClipComponents() action during the call to getPlanesPresent()
                // to find out the components present in output of this effect.
                //
                // Instead the plug-in should read planes from the pass-through clip (the same that is set in the implementation of getClipComponents)
                // to populate the output menu.
                vector<string> clipPlaneStrings;
                clip->getPlanesPresent(&clipPlaneStrings);

                // If this is the output menu, add user created planes from a user interface
                if (it->second.isOutput) {
                    vector<string> extraPlanes;
                    _publicInterface->getExtraneousPlanesCreated(&extraPlanes);
                    clipPlaneStrings.insert(clipPlaneStrings.end(), extraPlanes.begin(), extraPlanes.end());
                }

                for (std::size_t i = 0; i < clipPlaneStrings.size(); ++i) {
                    ImagePlaneDesc plane;
                    if (clipPlaneStrings[i] == kOfxMultiplaneColorPlaneID) {
                        plane = ImagePlaneDesc::mapNCompsToColorPlane(clip->getPixelComponentCount());
                    } else {
                        plane = ImagePlaneDesc::mapOFXPlaneStringToPlane(clipPlaneStrings[i]);
                    }
                    availableClipPlanes->push_back(plane);
                }

            }

            perClipPlanes.push_back(std::make_pair(clip, availableClipPlanes));
        } // for each clip

        for (std::list<std::pair<Clip*, std::list<ImagePlaneDesc>* > >::const_iterator it2 = perClipPlanes.begin(); it2 != perClipPlanes.end(); ++it2) {

            const std::list<ImagePlaneDesc>* planes = it2->second;

            for (std::list<ImagePlaneDesc>::const_iterator it3 = planes->begin(); it3 != planes->end(); ++it3) {
                if (it->second.splitPlanesIntoChannels) {
                    // User wants per-channel options
                    int nChannels = it3->getNumComponents();
                    for (int k = 0; k < nChannels; ++k) {

                        ChoiceOption opt;
                        it3->getChannelOption(k, &opt.name, &opt.label);

                        // Prefix the clip name if there are multiple clip channels to read from
                        if (it->second.clips.size() > 1) {
                            opt.name = it2->first->name() + '.' + opt.name;
                            opt.label = it2->first->name() + '.' + opt.label;
                        }

                        if (optionsSorted.find(opt) == optionsSorted.end()) {
                            options.push_back(opt);
                            optionsSorted.insert(opt);
                        }

                    }
                } else {
                    // User wants planes in options
                    ChoiceOption opt;
                    it3->getPlaneOption(&opt.name, &opt.label);

                    // Prefix the clip name if there are multiple clip channels to read from
                    if (it->second.clips.size() > 1) {
                        opt.name = it2->first->name() + '.' + opt.name;
                        opt.label = it2->first->name() + '.' + opt.label;
                    }
                    if (optionsSorted.find(opt) == optionsSorted.end()) {
                        options.push_back(opt);
                        optionsSorted.insert(opt);
                    }
                }
            } // for each plane

        } // for each clip planes available

        // Set the new choice menu
        perParamOptions.push_back(std::make_pair(it->second.param,options));


    } // for all choice parameters


    // Reset all choice options in the same pass, once the perClipPlanesAvailable is full, because the resetOptions call may recursively call
    // getClipComponents and thus getPlaneNeeded which relies on it.
    for (std::vector<std::pair<ChoiceParam*,std::vector<ChoiceOption> > >::const_iterator it = perParamOptions.begin(); it != perParamOptions.end(); ++it) {
        vector<string> labels(it->second.size()), hints(it->second.size()), enums(it->second.size());
        for (std::size_t i = 0; i < it->second.size(); ++i) {
            labels[i] = it->second[i].label;
            hints[i] = it->second[i].hint;
            enums[i] = it->second[i].name;
        }
        it->first->resetOptions(labels, hints, enums);
    }
} // buildChannelMenus

void
MultiPlaneEffectPrivate::handleAllPlanesCheckboxParamChanged()
{
    bool allPlanesSelected = allPlanesCheckbox->getValue();
    for (map<string, ChoiceParamClips>::const_iterator it = params.begin(); it != params.end(); ++it) {
        if (!it->second.splitPlanesIntoChannels) {
            it->second.param->setIsSecretAndDisabled(allPlanesSelected);
        }
    }
}

void
MultiPlaneEffectPrivate::refreshSelectorsVisibility()
{
    for (map<string, ChoiceParamClips>::iterator it = params.begin(); it != params.end(); ++it) {
        if ( it->second.isOutput || !it->second.hideIfClipDisconnected) {
            continue;
        }
        bool hasClipVisible = false;
        for (std::size_t i = 0; i < it->second.clips.size(); ++i) {
            if (it->second.clips[i]->isConnected()) {
                hasClipVisible = true;
                break;
            }
        }
        it->second.param->setIsSecretAndDisabled(!hasClipVisible);
    }
}

void
MultiPlaneEffect::onAllParametersFetched()
{
    _imp->refreshSelectorsVisibility();
}

void
MultiPlaneEffect::refreshPlaneChoiceMenus()
{
    _imp->buildChannelMenus();
}

void
MultiPlaneEffect::changedParam(const InstanceChangedArgs & /*args*/, const std::string &paramName)
{
    if (_imp->allPlanesCheckbox && paramName == _imp->allPlanesCheckbox->getName()) {
        _imp->handleAllPlanesCheckboxParamChanged();
    }
}

void
MultiPlaneEffect::changedClip(const InstanceChangedArgs & /*args*/, const std::string &clipName)
{
    _imp->refreshSelectorsVisibility();

    if (gHostIsNatron3OrGreater && clipName == kOfxImageEffectOutputClipName) {
        _imp->buildChannelMenus();
    }
}

void
MultiPlaneEffect::getClipPreferences(ClipPreferencesSetter &clipPreferences)
{
    // Refresh the channel menus on Natron < 3 or if it has never been refreshed, otherwise this is done in clipChanged in Natron >= 3
    if (!gHostIsNatron3OrGreater) {
        _imp->buildChannelMenus();
    }


    // By default set the clip preferences according to what is selected with the output plane selector.
    for (map<string, ChoiceParamClips>::iterator it = _imp->params.begin(); it != _imp->params.end(); ++it) {
        if (!it->second.isOutput) {
            continue;
        }
        MultiPlane::ImagePlaneDesc dstPlane;
        {
            OFX::Clip* clip = 0;
            int channelIndex = -1;
            MultiPlane::MultiPlaneEffect::GetPlaneNeededRetCodeEnum stat = getPlaneNeeded(it->second.param->getName(), &clip, &dstPlane, &channelIndex);
            if (stat != MultiPlane::MultiPlaneEffect::eGetPlaneNeededRetCodeReturnedPlane) {
                dstPlane = MultiPlane::ImagePlaneDesc::getNoneComponents();
            }
        }

        // Get clip preferences expects a hard coded components string but here we may have any kind of plane.
        // To respect OpenFX, we map our plane number of components to the components of the corresponding color plane
        // e.g: if our plane is Toto.XYZ and has 3 channels, it becomes RGB
        MultiPlane::ImagePlaneDesc colorPlaneMapped = MultiPlane::ImagePlaneDesc::mapNCompsToColorPlane(dstPlane.getNumComponents());
        PixelComponentEnum dstPixelComps = mapStrToPixelComponentEnum(MultiPlane::ImagePlaneDesc::mapPlaneToOFXComponentsTypeString(colorPlaneMapped));

        clipPreferences.setClipComponents(*it->second.clips[0], dstPixelComps);

    }
} // getClipPreferences

OfxStatus
MultiPlaneEffect::getClipComponents(const ClipComponentsArguments& args, ClipComponentsSetter& clipComponents)
{

    assert(gHostSupportsMultiPlaneV2 || gHostSupportsMultiPlaneV1);


    // Record clips that have already had their planes set because multipla parameters can reference the same clip
    std::map<Clip*, std::set<std::string> > clipMap;
    bool passThroughClipSet = false;
    for (map<string, ChoiceParamClips>::iterator it = _imp->params.begin(); it != _imp->params.end(); ++it) {
        MultiPlane::ImagePlaneDesc plane;
        OFX::Clip* clip = 0;
        int channelIndex = -1;
        MultiPlane::MultiPlaneEffect::GetPlaneNeededRetCodeEnum stat = getPlaneNeeded(it->second.param->getName(), &clip, &plane, &channelIndex);
        if (stat == MultiPlane::MultiPlaneEffect::eGetPlaneNeededRetCodeFailed) {
            return kOfxStatFailed;
        }
        if (stat == MultiPlane::MultiPlaneEffect::eGetPlaneNeededRetCodeReturnedConstant0 ||
            stat == MultiPlane::MultiPlaneEffect::eGetPlaneNeededRetCodeReturnedConstant1 ||
            stat == MultiPlane::MultiPlaneEffect::eGetPlaneNeededRetCodeReturnedAllPlanes) {
            continue;
        }
        std::set<std::string>& availablePlanes = clipMap[clip];

        std::string ofxComponentsStr = MultiPlane::ImagePlaneDesc::mapPlaneToOFXPlaneString(plane);
        std::pair<std::set<std::string>::iterator, bool> ret = availablePlanes.insert(ofxComponentsStr);
        if (ret.second) {
            clipComponents.addClipPlane(*clip, ofxComponentsStr);
        }

        // Set the pass-through clip to the first encountered source clip
        if (!passThroughClipSet && clip->name() != kOfxImageEffectOutputClipName) {
            passThroughClipSet = true;
            clipComponents.setPassThroughClip(clip, args.time, args.view);
        }
    }
    return kOfxStatOK;
} // getClipComponents

static bool findBuiltInSelectedChannel(const std::string& selectedOptionID,
                                       bool compareWithID,
                                       const ChoiceParamClips& param,
                                       MultiPlaneEffect::GetPlaneNeededRetCodeEnum* retCode,
                                       OFX::Clip** clip,
                                       ImagePlaneDesc* plane,
                                       int* channelIndexInPlane)
{
    if (selectedOptionID == kMultiPlaneChannelParamOption0) {
        *retCode = MultiPlaneEffect::eGetPlaneNeededRetCodeReturnedConstant0;
        return true;
    }

    if (selectedOptionID == kMultiPlaneChannelParamOption1) {
        *retCode = MultiPlaneEffect::eGetPlaneNeededRetCodeReturnedConstant1;
        return true;
    }

    if (param.addNoneOption && selectedOptionID == kMultiPlanePlaneParamOptionNone) {
        *plane = ImagePlaneDesc::getNoneComponents();
        *retCode = MultiPlaneEffect::eGetPlaneNeededRetCodeReturnedPlane;
        return true;
    }

    // The option must have a clip name prepended if there are multiple clips, find the clip
    std::string optionWithoutClipPrefix;

    if (param.clips.size() == 1) {
        *clip = param.clips[0];
        optionWithoutClipPrefix = selectedOptionID;
    } else {
        for (std::size_t c = 0; c < param.clipNames.size(); ++c) {
            const std::string& clipName = param.clipNames[c];
            if (selectedOptionID.substr(0, clipName.size()) == clipName) {
                *clip = param.clips[c];
                optionWithoutClipPrefix = selectedOptionID.substr(clipName.size() + 1); // + 1 to skip the dot
                break;
            }
        }
    }

    if (!*clip) {
        // We did not find the corresponding clip.
        *retCode = MultiPlaneEffect::eGetPlaneNeededRetCodeFailed;
        return false;
    }


    // Find a hard-coded option

    std::vector<const MultiPlane::ImagePlaneDesc*> planesToAdd;
    getHardCodedPlanes(false /*onlyColorPlane*/, &planesToAdd);
    for (std::size_t p = 0; p < planesToAdd.size(); ++p) {

        const vector<string>& planeChannels = planesToAdd[p]->getChannels();
        for (std::size_t c = 0; c < planeChannels.size(); ++c) {
            std::string channelOptionID;

            if (compareWithID) {
                channelOptionID = planesToAdd[p]->getPlaneID() + '.' + planeChannels[c];
            } else {
                channelOptionID = planesToAdd[p]->getPlaneLabel() + '.' + planeChannels[c];
            }

            if (channelOptionID == optionWithoutClipPrefix) {

                int chanIndex = (int)c;
                // If the hard-coded plane is the color plane, the channel may not exist actually in the available components,
                // e.g: Alpha may be present in the choice but the components may be RGB
                // In this case, return 1 instead for Alpha and 0 for any other channel.
                if (planesToAdd[p]->isColorPlane()) {
                    int clipComponentsCount = (*clip)->getPixelComponentCount();

                    // For the color plane, Color.A is channel index 0 when the plane is Color.Alpha
                    if (clipComponentsCount == 1 && chanIndex == 3) {
                        chanIndex = 0;
                    }
                    if ((int)chanIndex >= clipComponentsCount) {
                        if (chanIndex == 3) {
                            *retCode = MultiPlaneEffect::eGetPlaneNeededRetCodeReturnedConstant1;
                        } else {
                            *retCode = MultiPlaneEffect::eGetPlaneNeededRetCodeReturnedConstant0;
                        }
                        return true;
                    }
                }
                *plane = *planesToAdd[p];
                *channelIndexInPlane = chanIndex;
                *retCode = MultiPlaneEffect::eGetPlaneNeededRetCodeReturnedChannelInPlane;
                return true;
            }
        }

    } // for each built-in plane



    return false;
} // findBuiltInSelectedChannel

MultiPlaneEffect::GetPlaneNeededRetCodeEnum
MultiPlaneEffect::getPlaneNeeded(const std::string& paramName,
                                 OFX::Clip** clip,
                                 ImagePlaneDesc* plane,
                                 int* channelIndexInPlane)
{
    map<string, ChoiceParamClips>::iterator found = _imp->params.find(paramName);
    assert( found != _imp->params.end() );

    OFX::Clip* retClip = 0;
    ImagePlaneDesc retPlane;
    int retChannelIndexInPlane = -1;

    if ( found == _imp->params.end() ) {
        if (clip) {
            *clip = retClip;
        }
        if (plane) {
            *plane = retPlane;
        }
        if (channelIndexInPlane) {
            *channelIndexInPlane = retChannelIndexInPlane;
        }

        return eGetPlaneNeededRetCodeFailed;
    }


    if (found->second.isOutput && _imp->allPlanesCheckbox) {
        bool processAll = _imp->allPlanesCheckbox->getValue();
        if (processAll) {
            if (clip) {
                *clip = retClip;
            }
            if (plane) {
                *plane = retPlane;
            }
            if (channelIndexInPlane) {
                *channelIndexInPlane = retChannelIndexInPlane;
            }

            return eGetPlaneNeededRetCodeReturnedAllPlanes;
        }
    }

    // Get the selected option
    string selectedOptionID;

    // By default compare option IDs, except if the host does not support it.
    bool compareWithID = true;
    {
        int choice_i;
        found->second.param->getValue(choice_i);

        if ( (0 <= choice_i) && ( choice_i < found->second.param->getNOptions() ) ) {
#ifdef OFX_EXTENSIONS_NATRON
            found->second.param->getEnum(choice_i, selectedOptionID);
            if (selectedOptionID.empty()) {
#endif
                found->second.param->getOption(choice_i, selectedOptionID);
                compareWithID = false;
#ifdef OFX_EXTENSIONS_NATRON
            }
#endif
        } else {
            if (clip) {
                *clip = retClip;
            }
            if (plane) {
                *plane = retPlane;
            }
            if (channelIndexInPlane) {
                *channelIndexInPlane = retChannelIndexInPlane;
            }

            return eGetPlaneNeededRetCodeFailed;
        }
        if ( selectedOptionID.empty() ) {
            if (clip) {
                *clip = retClip;
            }
            if (plane) {
                *plane = retPlane;
            }
            if (channelIndexInPlane) {
                *channelIndexInPlane = retChannelIndexInPlane;
            }

            return eGetPlaneNeededRetCodeFailed;
        }
    }


    // If the choice is split by channels, check for hard coded options
    if (found->second.splitPlanesIntoChannels) {
        MultiPlaneEffect::GetPlaneNeededRetCodeEnum retCode;
        if (findBuiltInSelectedChannel(selectedOptionID, compareWithID, found->second, &retCode, &retClip, &retPlane, &retChannelIndexInPlane)) {
            if (clip) {
                *clip = retClip;
            }
            if (plane) {
                *plane = retPlane;
            }
            if (channelIndexInPlane) {
                *channelIndexInPlane = retChannelIndexInPlane;
            }

            return retCode;
        }
    } else {
        if (found->second.addNoneOption && selectedOptionID == kMultiPlanePlaneParamOptionNone) {
            retPlane = ImagePlaneDesc::getNoneComponents();
            if (clip) {
                *clip = retClip;
            }
            if (plane) {
                *plane = retPlane;
            }
            if (channelIndexInPlane) {
                *channelIndexInPlane = retChannelIndexInPlane;
            }

            return  MultiPlaneEffect::eGetPlaneNeededRetCodeReturnedPlane;
        }
    } // found->second.splitPlanesIntoChannels


    // This is not a hard-coded option, check for dynamic planes
    // The option must have a clip name prepended if there are multiple clips, find the clip
    std::string optionWithoutClipPrefix;
    if (found->second.clips.size() == 1) {
        retClip = found->second.clips[0];
        optionWithoutClipPrefix = selectedOptionID;
    } else {
        for (std::size_t c = 0; c < found->second.clipNames.size(); ++c) {
            const std::string& clipName = found->second.clipNames[c];
            if (selectedOptionID.substr(0, clipName.size()) == clipName) {
                retClip = found->second.clips[c];
                optionWithoutClipPrefix = selectedOptionID.substr(clipName.size() + 1); // + 1 to skip the dot
                break;
            }
        }
    }

    if (!retClip) {
        // We did not find the corresponding clip.
        if (clip) {
            *clip = retClip;
        }
        if (plane) {
            *plane = retPlane;
        }
        if (channelIndexInPlane) {
            *channelIndexInPlane = retChannelIndexInPlane;
        }

        return MultiPlaneEffect::eGetPlaneNeededRetCodeFailed;
    }

    // For the output plane selector, map the clip planes against the output clip even though the user provided a
    // source clip as pass-through clip so the extraneous planes returned by getExtraneousPlanesCreated are not added for
    // the available planes on the source clip
    retClip = found->second.isOutput ? _imp->dstClip : retClip;

    std::map<Clip*, std::list<ImagePlaneDesc> >::iterator foundPlanesPresentForClip = _imp->perClipPlanesAvailable.find(retClip);
    if (foundPlanesPresentForClip == _imp->perClipPlanesAvailable.end()) {
        // No components available for this clip...
        if (clip) {
            *clip = retClip;
        }
        if (plane) {
            *plane = retPlane;
        }
        if (channelIndexInPlane) {
            *channelIndexInPlane = retChannelIndexInPlane;
        }

        return MultiPlaneEffect::eGetPlaneNeededRetCodeFailed;
    }

    for (std::list<ImagePlaneDesc>::const_iterator it = foundPlanesPresentForClip->second.begin(); it != foundPlanesPresentForClip->second.end(); ++it) {
        if (found->second.splitPlanesIntoChannels) {
            // User wants per-channel options
            int nChannels = it->getNumComponents();
            for (int k = 0; k < nChannels; ++k) {
                std::string optionID, optionLabel;
                it->getChannelOption(k, &optionID, &optionLabel);

                bool foundPlane;
                if (compareWithID) {
                    foundPlane = optionWithoutClipPrefix == optionID;
                } else {
                    foundPlane = optionWithoutClipPrefix == optionLabel;
                }
                if (foundPlane) {
                    retPlane = *it;
                    retChannelIndexInPlane = k;
                    if (clip) {
                        *clip = retClip;
                    }
                    if (plane) {
                        *plane = retPlane;
                    }
                    if (channelIndexInPlane) {
                        *channelIndexInPlane = retChannelIndexInPlane;
                    }

                    return eGetPlaneNeededRetCodeReturnedChannelInPlane;
                }
            }
        } else {
            // User wants planes in options
            std::string optionID, optionLabel;
            it->getPlaneOption(&optionID, &optionLabel);
            bool foundPlane;
            if (compareWithID) {
                foundPlane = optionWithoutClipPrefix == optionID;
            } else {
                foundPlane = optionWithoutClipPrefix == optionLabel;
            }
            if (foundPlane) {
                retPlane = *it;
                if (clip) {
                    *clip = retClip;
                }
                if (plane) {
                    *plane = retPlane;
                }
                if (channelIndexInPlane) {
                    *channelIndexInPlane = retChannelIndexInPlane;
                }

               return eGetPlaneNeededRetCodeReturnedPlane;
            }
        }
    } // for each plane available on this clip

    if (clip) {
        *clip = retClip;
    }
    if (plane) {
        *plane = retPlane;
    }
    if (channelIndexInPlane) {
        *channelIndexInPlane = retChannelIndexInPlane;
    }

    return eGetPlaneNeededRetCodeFailed;
} // MultiPlaneEffect::getPlaneNeededForParam


static void refreshHostFlags()
{
    gHostSupportsDynamicChoices = false;
    gHostIsNatron3OrGreater = false;
    gHostSupportsMultiPlaneV1 = false;
    gHostSupportsMultiPlaneV2 = false;

#ifdef OFX_EXTENSIONS_NATRON
    if (getImageEffectHostDescription()->supportsDynamicChoices) {
        gHostSupportsDynamicChoices = true;
    }
    if (getImageEffectHostDescription()->isNatron && getImageEffectHostDescription()->versionMajor >= 3) {
        gHostIsNatron3OrGreater = true;
    }

#endif
#ifdef OFX_EXTENSIONS_NUKE
    if (getImageEffectHostDescription()->isMultiPlanar && fetchSuite(kFnOfxImageEffectPlaneSuite, 1)) {
        gHostSupportsMultiPlaneV1 = true;
    }
    if (getImageEffectHostDescription()->isMultiPlanar && gHostSupportsDynamicChoices && fetchSuite(kFnOfxImageEffectPlaneSuite, 2)) {
        gHostSupportsMultiPlaneV2 = true;
    }
#endif
}

namespace Factory {
ChoiceParamDescriptor*
describeInContextAddPlaneChoice(ImageEffectDescriptor &desc,
                                PageParamDescriptor* page,
                                const std::string& name,
                                const std::string& label,
                                const std::string& hint)
{

    refreshHostFlags();
    if (!gHostSupportsMultiPlaneV2 && !gHostSupportsMultiPlaneV1) {
        throw std::runtime_error("Hosts does not meet requirements");
    }
    ChoiceParamDescriptor *ret;
    {
        ChoiceParamDescriptor *param = desc.defineChoiceParam(name);
        param->setLabel(label);
        param->setHint(hint);
        if (!gHostSupportsMultiPlaneV2) {
            // Add hard-coded planes
            const MultiPlane::ImagePlaneDesc& rgbaPlane = MultiPlane::ImagePlaneDesc::getRGBAComponents();
            const MultiPlane::ImagePlaneDesc& disparityLeftPlane = MultiPlane::ImagePlaneDesc::getDisparityLeftComponents();
            const MultiPlane::ImagePlaneDesc& disparityRightPlane = MultiPlane::ImagePlaneDesc::getDisparityRightComponents();
            const MultiPlane::ImagePlaneDesc& motionBwPlane = MultiPlane::ImagePlaneDesc::getBackwardMotionComponents();
            const MultiPlane::ImagePlaneDesc& motionFwPlane = MultiPlane::ImagePlaneDesc::getForwardMotionComponents();

            std::vector<const MultiPlane::ImagePlaneDesc*> planesToAdd;
            planesToAdd.push_back(&rgbaPlane);
            planesToAdd.push_back(&disparityLeftPlane);
            planesToAdd.push_back(&disparityRightPlane);
            planesToAdd.push_back(&motionBwPlane);
            planesToAdd.push_back(&motionFwPlane);

            for (std::size_t i = 0; i < planesToAdd.size(); ++i) {
                std::string optionID, optionLabel;
                planesToAdd[i]->getPlaneOption(&optionID, &optionLabel);
                param->appendOption(optionLabel, "", optionID);
            }

        }
        param->setDefault(0);
        param->setAnimates(false);
        desc.addClipPreferencesSlaveParam(*param);             // < the menu is built in getClipPreferences
        if (page) {
            page->addChild(*param);
        }
        ret = param;
    }

    return ret;
}

OFX::BooleanParamDescriptor*
describeInContextAddAllPlanesOutputCheckbox(OFX::ImageEffectDescriptor &desc, OFX::PageParamDescriptor* page)
{
    refreshHostFlags();
    if (!gHostSupportsMultiPlaneV2 && !gHostSupportsMultiPlaneV1) {
        throw std::runtime_error("Hosts does not meet requirements");
    }
    BooleanParamDescriptor* param = desc.defineBooleanParam(kMultiPlaneProcessAllPlanesParam);
    param->setLabel(kMultiPlaneProcessAllPlanesParamLabel);
    param->setHint(kMultiPlaneProcessAllPlanesParamHint);
    desc.addClipPreferencesSlaveParam(*param);
    param->setAnimates(false);
    if (page) {
        page->addChild(*param);
    }
    return param;
}

ChoiceParamDescriptor*
describeInContextAddPlaneChannelChoice(ImageEffectDescriptor &desc,
                                       PageParamDescriptor* page,
                                       const vector<string>& clips,
                                       const string& name,
                                       const string& label,
                                       const string& hint,
                                       bool addConstants)
    
{

    refreshHostFlags();
    if (!gHostSupportsMultiPlaneV2 && !gHostSupportsMultiPlaneV1) {
        throw std::runtime_error("Hosts does not meet requirements");
    }
    
    ChoiceParamDescriptor *ret;
    {
        ChoiceParamDescriptor *param = desc.defineChoiceParam(name);
        param->setLabel(label);
        desc.addClipPreferencesSlaveParam(*param);
        param->setHint(hint);
        param->setAnimates(false);
        addInputChannelOptionsRGBA(param, clips, addConstants /*addContants*/, gHostSupportsMultiPlaneV2 /*onlyColorPlane*/);

        if (page) {
            page->addChild(*param);
        }
        ret = param;
    }

    return ret;
}
}         // Factory
}     // namespace MultiPlane
} // namespace OFX
