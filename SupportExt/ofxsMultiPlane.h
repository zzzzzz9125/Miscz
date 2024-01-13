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

    if (OFX::fetchSuite(kFnOfxImageEffectPlaneSuite, 2) &&  // for clipGetImagePlane
        OFX::getImageEffectHostDescription()->supportsDynamicChoices && // for dynamic layer choices
        OFX::getImageEffectHostDescription()->isMultiPlanar) // for clipGetImagePlane
         ... this is ok...
 *#endif
 */

#ifndef openfx_supportext_ofxsMultiPlane_h
#define openfx_supportext_ofxsMultiPlane_h

#include <cmath>
#include <map>
#include <string>
#include <list>
#include <vector>

#include "ofxsImageEffect.h"
#include "ofxsMacros.h"
#ifdef OFX_EXTENSIONS_NATRON
#include "ofxNatron.h"
#endif


#define kOfxMultiplaneColorPlaneID kFnOfxImagePlaneColour
#define kOfxMultiplaneColorPlaneLabel "Color"

#define kOfxMultiplaneBackwardMotionVectorsPlaneID kFnOfxImagePlaneBackwardMotionVector
#define kOfxMultiplaneBackwardMotionVectorsPlaneLabel "Backward"

#define kOfxMultiplaneForwardMotionVectorsPlaneID kFnOfxImagePlaneForwardMotionVector
#define kOfxMultiplaneForwardMotionVectorsPlaneLabel "Forward"

#define kOfxMultiplaneDisparityLeftPlaneID kFnOfxImagePlaneStereoDisparityLeft
#define kOfxMultiplaneDisparityLeftPlaneLabel "DisparityLeft"

#define kOfxMultiplaneDisparityRightPlaneID kFnOfxImagePlaneStereoDisparityRight
#define kOfxMultiplaneDisparityRightPlaneLabel "DisparityRight"

#define kOfxMultiplaneDisparityComponentsLabel "Disparity"
#define kOfxMultiplaneMotionComponentsLabel "Motion"


#define kMultiPlaneChannelParamOption0 "0"
#define kMultiPlaneChannelParamOption0Hint "0 constant channel"
#define kMultiPlaneChannelParamOption1 "1"
#define kMultiPlaneChannelParamOption1Hint "1 constant channel"

#define kMultiPlanePlaneParamOptionNone "none"
#define kMultiPlanePlaneParamOptionNoneLabel "None"

#define kMultiPlaneProcessAllPlanesParam "processAllPlanes"
#define kMultiPlaneProcessAllPlanesParamLabel "All Planes"
#define kMultiPlaneProcessAllPlanesParamHint "When checked all planes in input will be processed and output to the same plane as in input. It is useful for example to apply a Transform effect on all planes."



/*
 A Multi-planar effect is a plane that can process arbitrary (other than color) image planes.
 The planes described further can be seen as a layer in the OpenEXR specification.
 
 A plane consists of an arbitrary number of channels, ranging from 1 to 4 (included) and is 
 labeled with a unique name and a label
 For example...
    The Color.RGBA plane, corresponds to the Color plane with 4 channels: RGBA
    
 This extension also allow to alias the channel names in a more meaningful way, 
 For example...
 
    The DisparityLeft.Disparity plane corresponds to the DisparityLeft plane with
    channels of type "Disparity", being 2 channels: XY.
 
 The generic way of describing a layer is done as such:
 

 kNatronOfxImageComponentsPlaneName +
 planeName +
 <optional> kNatronOfxImageComponentsPlaneLabel + planeLabel +
 <optional> kNatronOfxImageComponentsPlaneChannelsLabel + channelsLabel +
 kNatronOfxImageComponentsPlaneChannel + channel1Name +
 kNatronOfxImageComponentsPlaneChannel + channel2Name +
 kNatronOfxImageComponentsPlaneChannel + channel3Name

 Examples:

 kNatronOfxImageComponentsPlaneName + "fr.unique.id.position" + kNatronOfxImageComponentsPlaneLabel + "Position" + kNatronOfxImageComponentsPlaneChannel + "X" + kNatronOfxImageComponentsPlaneChannel + "Y" + kNatronOfxImageComponentsPlaneChannel + "Z"


 kNatronOfxImageComponentsPlaneName + "DisparityLeft" + kNatronOfxImageComponentsPlaneChannelsLabel + "Disparity" + kNatronOfxImageComponentsPlaneChannel + "X" + kNatronOfxImageComponentsPlaneChannel + "Y"

 
 A multi-planar effect is expected to set the kFnOfxImageEffectPropMultiPlanar property to 1 on the image effect descritpor.
 
 A multi-planar effect is expected to support images of any number of channels: 

 - The planes fetched from clipGetImagePlane are returned "as is" and will not be remapped by the host to another components type

 - The exception is for the kFnOfxImagePlaneColour plane: the components of the plane will be remapped to those specified in the getClipPreferences action

 A multi-planar effect is expected to specify the planes it produces in output and the planes it needs in input via the getClipComponents action.
 
 OpenFX only defines kOfxImageComponentRGBA, kOfxImageComponentRGB, kOfxImageComponentAlpha for default components type.
 However, an effect may still be able to process 2-channel image and be non multi-planar aware.
 To enable this, we also define kNatronOfxImageComponentXY as a components type that can be used in the getClipPreferences action and can be indicated
 on a 2-channel image.
 
 Support for Nuke:
 ----------------
 
 Nuke only supports the MultiPlane suite V1 and does not uses the getClipComponents action and does not support abitrary planes as defined above.
 
 Nuke only supports the following hard-coded planes:
 
 kFnOfxImagePlaneColour --> This plane can only have the following components types: kOfxImageComponentRGBA, kOfxImageComponentRGB, kOfxImageComponentAlpha

 kFnOfxImagePlaneBackwardMotionVector --> This plane has the following components type: kFnOfxImageComponentMotionVectors
 kFnOfxImagePlaneForwardMotionVector --> This plane has the following components type: kFnOfxImageComponentMotionVectors

 kFnOfxImagePlaneStereoDisparityLeft --> This plane has the following components type: kFnOfxImageComponentStereoDisparity
 kFnOfxImagePlaneStereoDisparityRight --> This plane has the following components type: kFnOfxImageComponentStereoDisparity

 Flagging kFnOfxImageEffectPropMultiPlanar=1 in Nuke only means that besides the regular clipGetImage call to get the color plane,
 you can fetch the motion vectors (BW/FW) and disparity planes (Left, Right) with the clipGetImagePlane call.
 
 To indicate which plane you want on a clip, you do so from the getClipPreferences action by indicating the components TYPE,
 that is kFnOfxImageComponentMotionVectors, kFnOfxImageComponentStereoDisparity or kOfxImageComponentRGBA, kOfxImageComponentRGB, kOfxImageComponentAlpha

 If a clip is set to the kFnOfxImageComponentMotionVectors components type, the host expects the plug-in to call clipGetImagePlane on both the
 kFnOfxImagePlaneBackwardMotionVector plane and the kFnOfxImagePlaneForwardMotionVector plane.
 These planes are "paired": a plug-in that renders kFnOfxImageComponentMotionVectors is expected to render both planes at once.
 
 Similarly, if the clip components are set to kFnOfxImageComponentStereoDisparity, the host expects that the plug-in calls clipGetImagePlane on both
 the kFnOfxImagePlaneStereoDisparityLeft and kFnOfxImagePlaneStereoDisparityRight planes.
 
 
 Planes vs. components:
 ----------------------
 
 Planes and components are different things. A plane is a unique image label of a certain components type. 
 The components define how many channels are present in an image plane.
 
 With the Natron extension, planes encoded directly their components type along them. This is not the case with the
 Nuke multi-plane suite V1 where components types and planes are hard-coded.
 
 To sum-up:
 
 - Planes defined in the Natron extension form indicate directly their components type, in the form

 kNatronOfxImageComponentsPlaneName + planeName +
 <optional> kNatronOfxImageComponentsPlaneLabel + planeLabel +
 <optional> kNatronOfxImageComponentsPlaneChannelsLabel + channelsLabel +
 kNatronOfxImageComponentsPlaneChannel + channel1Name +
 kNatronOfxImageComponentsPlaneChannel + channel2Name +
 kNatronOfxImageComponentsPlaneChannel + channel3Name

- Planes defined in the Nuke multi-plane suite have hard-coded components type (see Support for Nuke above)

- The getClipComponents action must return a list of string corresponding to planes and not components type.

 MultiPlaneEffect:
 ----------------
 
 To support abitrary planes, this suite needs to deal with dynamic choice parameter menus. This is known not to be supported in Nuke.

 The MultiPlaneEffect class below is mainly a helper class inheriting ImageEffect to conveniently create and manage choice parameters
 that allow the user to select planes or a specific channel in a plane.
 
 The host notifies that the planes have changed for a plug-in by calling the instanceChanged action on the output clip.
 
 This is where the buildChannelMenus() function should be called to refresh choice parameters menus.
 
 Note that this class also supports multi-planar effects in the sense of the the Nuke multi-plane suite (i.e: only motion vectors and disparity planes)
 
 Note that the derived plug-in still needs to set the kFnOfxImageEffectPropMultiPlanar property to 1, otherwise this class will not do much.

 */
namespace OFX {
namespace MultiPlane {


/**
 * @brief An ImagePlaneDesc represents an image plane and its components type.
 * The plane is uniquely identified by its planeID, it is used internally to compare planes.
 * The plane label is used for any UI related display: this is what the user sees.
 * If empty, the plane label is the same as the planeID.
 * The channels label is an optional string indicating in a more convenient way the types
 * of components expressed by the channels e.g: Instead of having "XY" for motion vectors,
 * they could be labeled with "Motion".
 * If empty, the channels label is set to the concatenation of all channels.
 * The channels are the unique identifier for each channel composing the plane.
 * The plane can only be composed from 1 to 4 (included) channels.
    **/
class ImagePlaneDesc
{
    public:


    ImagePlaneDesc();

    ImagePlaneDesc(const std::string& planeID,
                   const std::string& planeLabel,
                   const std::string& channelsLabel,
                   const std::vector<std::string>& channels);

    ImagePlaneDesc(const std::string& planeID,
                   const std::string& planeLabel,
                   const std::string& channelsLabel,
                   const char** channels,
                   int count);


    ImagePlaneDesc(const ImagePlaneDesc& other);

    ImagePlaneDesc& operator=(const ImagePlaneDesc& other);

    ~ImagePlaneDesc();

    // Is it Alpha, RGB or RGBA
    bool isColorPlane() const;

    static bool isColorPlane(const std::string& layerID);

    /**
     * @brief Returns the number of channels in this plane.
     **/
    int getNumComponents() const;

    /**
     * @brief Returns the plane unique identifier. This should be used to compare ImagePlaneDesc together.
     * This is not supposed to be used for display purpose, use getPlaneLabel() instead.
     **/
    const std::string& getPlaneID() const;

    /**
     * @brief Returns the plane label.
     * This is what is used to display to the user.
     **/
    const std::string& getPlaneLabel() const;

    /**
     * @brief Returns the channels composing this plane.
     **/
    const std::vector<std::string>& getChannels() const;

    /**
     * @brief Returns a label used to better represent the type of components used by this plane.
     * e.g: "Motion" can be used to better label "XY" component types.
     **/
    const std::string& getChannelsLabel() const;


    bool operator==(const ImagePlaneDesc& other) const;

    bool operator!=(const ImagePlaneDesc& other) const
    {
        return !(*this == other);
    }

    // For std::map
    bool operator<(const ImagePlaneDesc& other) const;

    operator bool() const
    {
        return getNumComponents() > 0;
    }

    bool operator!() const
    {
        return getNumComponents() == 0;
    }


    void getPlaneOption(std::string* optionID, std::string* optionLabel) const;
    void getChannelOption(int channelIndex, std::string* optionID, std::string* optionLabel) const;

    /**
     * @brief Maps the given nComps to the color plane
     **/
    static const ImagePlaneDesc& mapNCompsToColorPlane(int nComps);

    /**
     * @brief Maps the given OpenFX plane to a ImagePlaneDesc.
     * @param ofxPlane Can be

     *  kFnOfxImagePlaneBackwardMotionVector
     *  kFnOfxImagePlaneForwardMotionVector
     *  kFnOfxImagePlaneStereoDisparityLeft
     *  kFnOfxImagePlaneStereoDisparityRight
     *  Or any plane encoded in the format specified by the Natron multi-plane extension.
     * This function CANNOT be used to map the color plane, instead use mapNCompsToColorPlane.
     *
     * This function returns an empty plane desc upon failure.
     **/
    static ImagePlaneDesc mapOFXPlaneStringToPlane(const std::string& ofxPlane);

    /**
     * @brief Maps OpenFX components string to a plane, optionnally also to a paired plane in the case of disparity/motion vectors.
     * @param ofxComponents Must be a string between
     * kOfxImageComponentRGBA, kOfxImageComponentRGB, kOfxImageComponentAlpha, kNatronOfxImageComponentXY, kOfxImageComponentNone
     * or kFnOfxImageComponentStereoDisparity or kFnOfxImageComponentMotionVectors
     * Or any plane encoded in the format specified by the Natron multi-plane extension.
     **/
    static void mapOFXComponentsTypeStringToPlanes(const std::string& ofxComponents, ImagePlaneDesc* plane, ImagePlaneDesc* pairedPlane);

    /**
     * @brief Does the inverse of mapOFXPlaneStringToPlane, except that it can also be used for
     * the color plane.
     **/
    static std::string mapPlaneToOFXPlaneString(const ImagePlaneDesc& plane);

    /**
     * @brief Returns an OpenFX encoded string representing the components type of the plane.
     * @returns One of the following strings:
     * kOfxImageComponentRGBA, kOfxImageComponentRGB, kOfxImageComponentAlpha, kNatronOfxImageComponentXY, kOfxImageComponentNone
     * or kFnOfxImageComponentStereoDisparity or kFnOfxImageComponentMotionVectors
     * Or any plane encoded in the format specified by the Natron multi-plane extension.
     **/
    static std::string mapPlaneToOFXComponentsTypeString(const ImagePlaneDesc& plane);

    /**
     * @brief Find a layer equivalent to this layer in the other layers container.
     * ITERATOR must be either a std::vector<ImagePlaneDesc>::iterator or std::list<ImagePlaneDesc>::iterator
     **/
    template <typename ITERATOR>
    static ITERATOR findEquivalentLayer(const ImagePlaneDesc& layer, ITERATOR begin, ITERATOR end)
    {
        bool isColor = layer.isColorPlane();

        ITERATOR foundExistingColorMatch = end;
        ITERATOR foundExistingComponents = end;

        for (ITERATOR it = begin; it != end; ++it) {
            if (it->isColorPlane() && isColor) {
                foundExistingColorMatch = it;
            } else {
                if (*it == layer) {
                    foundExistingComponents = it;
                    break;
                }
            }
        } // for each output components

        if (foundExistingComponents != end) {
            return foundExistingComponents;
        } else if (foundExistingColorMatch != end) {
            return foundExistingColorMatch;
        } else {
            return end;
        }
    } // findEquivalentLayer

    /*
     * These are default presets image components
     */
    static const ImagePlaneDesc& getNoneComponents();
    static const ImagePlaneDesc& getRGBAComponents();
    static const ImagePlaneDesc& getRGBComponents();
    static const ImagePlaneDesc& getAlphaComponents();
    static const ImagePlaneDesc& getBackwardMotionComponents();
    static const ImagePlaneDesc& getForwardMotionComponents();
    static const ImagePlaneDesc& getDisparityLeftComponents();
    static const ImagePlaneDesc& getDisparityRightComponents();
    static const ImagePlaneDesc& getXYComponents();


private:
    std::string _planeID, _planeLabel;
    std::vector<std::string> _channels;
    std::string _channelsLabel;
};


struct MultiPlaneEffectPrivate;
class MultiPlaneEffect
    : public OFX::ImageEffect
{
    auto_ptr<MultiPlaneEffectPrivate> _imp;

public:


    MultiPlaneEffect(OfxImageEffectHandle handle);

    virtual ~MultiPlaneEffect();

    struct FetchChoiceParamOptions
    {
        bool splitPlanesIntoChannelOptions;
        bool addNoneOption;
        bool addConstantOptions;
        bool isOutputPlaneChoice;
        bool hideIfClipDisconnected;
        std::vector<OFX::Clip*> dependsClips;

        static FetchChoiceParamOptions createFetchChoiceParamOptionsForInputChannel()
        {
            FetchChoiceParamOptions ret;
            ret.splitPlanesIntoChannelOptions = true;
            ret.addNoneOption = false;
            ret.addConstantOptions = true;
            ret.isOutputPlaneChoice = false;
            ret.hideIfClipDisconnected = false;
            return ret;
        }

        static FetchChoiceParamOptions createFetchChoiceParamOptionsForOutputPlane()
        {
            FetchChoiceParamOptions ret;
            ret.splitPlanesIntoChannelOptions = false;
            ret.addNoneOption = false;
            ret.addConstantOptions = false;
            ret.isOutputPlaneChoice = true;
            ret.hideIfClipDisconnected = false;
            return ret;
        }
    };

    /**
     * @brief Fetch a dynamic choice parameter that was declared to the factory with
     * describeInContextAddOutputLayerChoice() or describeInContextAddChannelChoice().
     * @param splitPlanesIntoChannelOptions If true, each option will be a channel of a plane
     * @param dependsClips The planes available from the given clips will be used to populate the choice options.
     * @param isOutputPlaneChoice If this
     **/
    void fetchDynamicMultiplaneChoiceParameter(const std::string& paramName,
                                               const FetchChoiceParamOptions& args);

    /**
     * @brief Should be called by the implementation to refresh the visibility of parameters once they have all been fetched.
     **/
    void onAllParametersFetched();

    enum GetPlaneNeededRetCodeEnum
    {
        eGetPlaneNeededRetCodeFailed,
        eGetPlaneNeededRetCodeReturnedPlane,
        eGetPlaneNeededRetCodeReturnedChannelInPlane,
        eGetPlaneNeededRetCodeReturnedConstant0,
        eGetPlaneNeededRetCodeReturnedConstant1,
        eGetPlaneNeededRetCodeReturnedAllPlanes
    };
    
    /**
     * @brief Returns the plane and channel index selected by the user in the given dynamic choice parameter "paramName".
     * @param plane Contains in output the plane selected by the user
     * If ofxPlane is empty but the function returned true that is because the choice is either kMultiPlaneParamOutputOption0 or kMultiPlaneParamOutputOption1
     * ofxComponents will have been set correctly to one of these values.
     *
     * @param channelIndexInPlane Contains in output the selected channel index in the plane set to ofxPlane
     *
     * @returns eGetPlaneNeededRetCodeFailed if this function failed.
     * If the result is eGetPlaneNeededRetCodeReturnedConstant0 or eGetPlaneNeededRetCodeReturnedConstant1 is returned, the plug-in should use
     * a corresponding constant (0 or 1) instead of a channel from the clip.
     * If the result is eGetPlaneNeededRetCodeReturnedPlane, the plane in output will be set to the user selected plane.
     * If the result is eGetPlaneNeededRetCodeReturnedChannelInPlane, the plane and the channelIndexInPlane in output will be set to the user
     * selected channel.
     * If the result is eGetPlaneNeededRetCodeReturnedAllPlanes, the plug-in is expected to render what is requested and should act as 
     * plane agnostic.
     **/
    GetPlaneNeededRetCodeEnum getPlaneNeeded(const std::string& paramName,
                                             OFX::Clip** clip,
                                             ImagePlaneDesc* plane,
                                             int* channelIndexInPlane);


    /**
     * @brief Must be called by derived class before anything else: it refreshes the channel menus.
     * By default it also set the dst clip pixel components according to the output plane selector if there is any
     **/
    virtual void getClipPreferences(ClipPreferencesSetter &clipPreferences) OVERRIDE;

    /**
     * @brief Set the requested planes according to the selectors that were registered for each clip.
     * By default the pass-through clip is set to the first source clip encountered in the registered plane selectors
     **/
    virtual OfxStatus getClipComponents(const ClipComponentsArguments& args, ClipComponentsSetter& clipComponents) OVERRIDE;

    /**
     * @brief Force a refresh of the channel selectors. This should in general not be called as this is done for you in changedClip() in Natron > 3
     * or getClipPreferences for any other host.
     **/
    void refreshPlaneChoiceMenus();

    /**
     * @brief Overriden to handle parameter changes. Derived class must call this class implementation.
     **/
    virtual void changedParam(const InstanceChangedArgs &args, const std::string &paramName) OVERRIDE;

    /**
     * @brief Overriden to handle clip changes. Derived class must call this class implementation.
     **/
    virtual void changedClip(const InstanceChangedArgs &args, const std::string &clipName) OVERRIDE;

    
};

namespace Factory {

/**
 * @brief Add a dynamic choice parameter to select a plane.
 * This should only be called for effects that flag kFnOfxImageEffectPropMultiPlanar=1 and
 * if the host supports multi-plane suite v2 and dynamic choice parameters.
 **/
OFX::ChoiceParamDescriptor* describeInContextAddPlaneChoice(OFX::ImageEffectDescriptor &desc,
                                                            OFX::PageParamDescriptor* page,
                                                            const std::string& name,
                                                            const std::string& label,
                                                            const std::string& hint);


/**
 * @brief Add a boolean parameter indicating if true that the plug-in is plane agnostic and will fetch in input the plane requested to render
 * in output. Note that in this case any other plane choice parameter will be made secret.
 **/
OFX::BooleanParamDescriptor* describeInContextAddAllPlanesOutputCheckbox(OFX::ImageEffectDescriptor &desc, OFX::PageParamDescriptor* page);

/**
 * @brief Add a dynamic choice parameter to select a channel among planes available in one or multiple source clips.
 **/
OFX::ChoiceParamDescriptor* describeInContextAddPlaneChannelChoice(OFX::ImageEffectDescriptor &desc,
                                                                   OFX::PageParamDescriptor* page,
                                                                   const std::vector<std::string>& clips,
                                                                   const std::string& name,
                                                                   const std::string& label,
                                                                   const std::string& hint,
                                                                   bool addConstants = true);

/**
 * @brief Add the standard R,G,B,A choices for the given clips.
 * @param addConstants If true, it will also add the "0" and "1" choice to the list
 **/
void addInputChannelOptionsRGBA(OFX::ChoiceParamDescriptor* param,
                                const std::vector<std::string>& clips,
                                bool addConstants,
                                bool onlyColorPlane);


/**
 * @brief Same as above, but for a choice param instance
 **/
void addInputChannelOptionsRGBA(const std::vector<std::string>& clips,
                                bool addConstants,
                                bool onlyColorPlane);
}         // Factory
}     // namespace MultiPlane
} // namespace OFX


#endif /* defined(openfx_supportext_ofxsMultiPlane_h) */
