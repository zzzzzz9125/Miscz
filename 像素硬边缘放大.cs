using System;
using System.Collections.Generic;
using System.Collections;
using System.IO;
using System.Text;
using System.Windows.Forms;
using System.Globalization;
using System.Drawing;
using System.Runtime;
using System.Xml;
using ScriptPortal.Vegas;

namespace Test_Script1
{

    public class Class1
    {
        public const double SCALETYPEFACTOR = 1.8;
        public Vegas myVegas;
        public float scrWidth, scrHeight, dFullWidth, dFullHeight;
        public double firstWidth, firstHeight;
        public void Main(Vegas vegas)
        {
            myVegas = vegas;
            myVegas.ResumePlaybackOnScriptExit = true;
            PlugInNode plugin1 = myVegas.VideoFX.GetChildByUniqueID("{Svfx:net.sf.openfx.MzTransformPlugin}");
            PlugInNode plugin2 = myVegas.VideoFX.GetChildByUniqueID("{Svfx:net.sf.openfx.MzPosition}");
            scrWidth = myVegas.Project.Video.Width;
            scrHeight = myVegas.Project.Video.Height;
            bool resetMode = ((Control.ModifierKeys & Keys.Control) != 0) ? true : false;
            foreach (Track myTrack in myVegas.Project.Tracks)
            {
                if (myTrack.IsVideo())
                {
                    foreach (TrackEvent evnt in myTrack.Events)
                    {
                        if (evnt.Selected)
                        {

                            VideoEvent vEvent = (VideoEvent)evnt;
                            VideoMotionKeyframes vKeyframes = vEvent.VideoMotion.Keyframes;
                            int countKeyframes = vKeyframes.Count;

                            MediaStream mediaStream = GetActiveMediaStream(evnt);
                            VideoStream videoStream = (VideoStream)mediaStream;
                            dFullWidth = videoStream.Width;
                            dFullHeight = videoStream.Height;

                            // Judge the status of XFlip and YFlip
                            double rotationSave = vKeyframes[0].Rotation;
                            vKeyframes[0].Rotation = 1; // Turn at a certain angle to avoid misjudgment in some cases
                            bool isXFlip = (vKeyframes[0].TopLeft.X - vKeyframes[0].TopRight.X) * Math.Cos(vKeyframes[0].Rotation) > 0;
                            bool isYFlip = (vKeyframes[0].TopRight.Y - vKeyframes[0].BottomRight.Y) * Math.Cos(vKeyframes[0].Rotation) > 0;
                            vKeyframes[0].Rotation = rotationSave;

                            // Calculate the index where TransformOFX is added
                            int theLastOne = 0, countBefore = 0;
                            int effectCount = vEvent.Effects.Count;
                            for (int i = effectCount - 1; i >= 0; i--)
                            {
                                if (vEvent.Effects[i].PlugIn.UniqueID == plugin1.UniqueID)
                                {
                                    if (resetMode)
                                    {
                                        vEvent.Effects.RemoveAt(i);
                                        continue;
                                    }
                                    else
                                    {
                                        if (theLastOne < i + 1)
                                        {
                                            theLastOne = i + 1;
                                        }
                                    }
                                }

                                if (vEvent.Effects[i].ApplyBeforePanCrop == true)
                                {
                                    countBefore += 1;
                                }
                            }

                            Effect effect1 = new Effect(plugin1);
                            vEvent.Effects.Insert(Math.Max(theLastOne, countBefore), effect1);

                            OFXEffect ofx1 = effect1.OFXEffect;
                            OFXBooleanParameter transformUniform = (OFXBooleanParameter)ofx1["uniform"];
                            transformUniform.Value = vEvent.MaintainAspectRatio ? true : true; // Experimental and imperfect, delete "? true : true" if you want to test
                            OFXChoiceParameter transformFilter = (OFXChoiceParameter)ofx1["filter"];
                            transformFilter.Value = transformFilter.Choices[0];
                            OFXDouble2DParameter transformScale = (OFXDouble2DParameter)ofx1["scale"];
                            double ScaleValueX = Math.Max(scrWidth / dFullWidth, scrHeight / dFullHeight);
                            double ScaleValueY = Math.Min(scrWidth / dFullWidth, scrHeight / dFullHeight);
                            OFXDouble2D Scale = new OFXDouble2D { X = transformUniform.Value ? ScaleValueY : ScaleValueX, Y = ScaleValueY};
                            OFXDouble2DParameter transformCenter = (OFXDouble2DParameter)ofx1["center"];
                            OFXDouble2D Pos = new OFXDouble2D { X = scrWidth / 2 + (dFullWidth % 2)/2.0, Y = scrHeight / 2 - (dFullHeight % 2)/2.0};

                            if (resetMode)
                            {
                                transformScale.Value = Scale;
                                transformCenter.Value = Pos;
                            }

                            else if (theLastOne > 0)
                            {
                                Scale.X = 1;
                                Scale.Y = 1;
                                Pos.X = scrWidth / 2;
                                Pos.Y = scrHeight / 2;
                                transformScale.Value = Scale;
                                transformCenter.Value = Pos;
                            }

                            else
                            {
                                // Set the values of the initial state
                                if(isXFlip || isYFlip)
                                {
                                    OFXBooleanParameter transformFlop = (OFXBooleanParameter)ofx1["flop"];
                                    transformFlop.Value = isXFlip;
                                    OFXBooleanParameter transformFlip = (OFXBooleanParameter)ofx1["flip"];
                                    transformFlip.Value = isYFlip;
                                }
                                OFXDoubleParameter transformRotate = (OFXDoubleParameter)ofx1["rotate"];
                                OFXDouble2DParameter transformTranslate = (OFXDouble2DParameter)ofx1["translate"];
                                firstWidth = PointDistance(vKeyframes[0].TopLeft, vKeyframes[0].TopRight);
                                firstHeight = PointDistance(vKeyframes[0].TopLeft, vKeyframes[0].BottomLeft);
                                ScaleValueX = Math.Max(scrWidth / firstWidth, scrHeight / firstHeight);
                                ScaleValueY = Math.Min(scrWidth / firstWidth, scrHeight / firstHeight);
                                Scale = new OFXDouble2D { X = transformUniform.Value ? ScaleValueY : ScaleValueX, Y = ScaleValueY};
                                transformScale.Value = Scale;
                                transformRotate.Value = vKeyframes[0].Rotation / Math.PI * 180 * (isXFlip ? -1 : 1) * (isYFlip ? -1 : 1);
                                transformTranslate.Value = PointTranslate(vKeyframes[0]);
                                transformCenter.Value = new OFXDouble2D { X = Pos.X, Y = Pos.Y};
                                OFXInterpolationType type0 = TypeChange(vKeyframes[0].Type);

                                if (countKeyframes > 1)
                                {
                                    // Verify if we need to animate these keyframes
                                    int nScale = 0, nScaleN = 0, nRotate = 0, nRotateN = 0, nTranslate = 0, nTranslateN = 0,nCenter = 0, nCenterN = 0, nTranslateRotate = 0, nTranslateRotateN = 0;
                                    for (int jj = countKeyframes - 1; jj >= 1; jj--)
                                    {
                                        VideoMotionKeyframe thisKeyframe = vKeyframes[jj] as VideoMotionKeyframe;
                                        nScale = Ratio(thisKeyframe) == 1 ? 0 : 1;
                                        nScaleN += nScale;
                                        nRotate = Math.Round(thisKeyframe.Rotation - vKeyframes[0].Rotation, 4) == 0 ? 0 : 1;
                                        nRotateN += nRotate;
                                        nTranslate = PointEqual(PointTranslate(thisKeyframe), PointTranslate(vKeyframes[0])) ? 0 : 1;
                                        nTranslateN += nTranslate;
                                        nTranslateRotate = PointEqual(PointTranslate(thisKeyframe, true), PointTranslate(vKeyframes[0], true)) ? 0 : 1;
                                        nTranslateRotateN += nTranslateRotate;
                                        nCenter = PointEqual(thisKeyframe.Center, vKeyframes[0].Center) ? 0 : 1;
                                        nCenterN += nCenter;
                                    }

                                    transformScale.IsAnimated = Convert.ToBoolean(nScaleN);
                                    if (transformScale.IsAnimated)
                                    {
                                        transformScale.Keyframes[0].Time = vKeyframes[0].Position;
                                        transformScale.Keyframes[0].Interpolation = type0;
                                    }

                                    transformRotate.IsAnimated = Convert.ToBoolean(nRotateN);
                                    if (transformRotate.IsAnimated)
                                    {
                                        transformRotate.Keyframes[0].Time = vKeyframes[0].Position;
                                        transformRotate.Keyframes[0].Interpolation = type0;
                                        transformTranslate.Value = PointTranslate(vKeyframes[0], true);
                                        transformCenter.Value = new OFXDouble2D { X = Pos.X + vKeyframes[0].Center.X - dFullWidth / 2, Y = Pos.Y - vKeyframes[0].Center.Y + dFullHeight / 2};
                                    }

                                    transformTranslate.IsAnimated = Convert.ToBoolean(nRotateN) ? Convert.ToBoolean(nTranslateRotateN) : Convert.ToBoolean(nTranslateN);
                                    if (transformTranslate.IsAnimated) 
                                    {
                                        transformTranslate.Keyframes[0].Time = vKeyframes[0].Position;
                                        transformTranslate.Keyframes[0].Interpolation = type0;
                                    }

                                    transformCenter.IsAnimated = Convert.ToBoolean(nCenterN) && Convert.ToBoolean(nRotateN);
                                    if (transformCenter.IsAnimated) 
                                    {
                                        transformCenter.Keyframes[0].Time = vKeyframes[0].Position;
                                        transformCenter.Keyframes[0].Interpolation = type0;
                                    }

                                    // Add keyframes
                                    for (int jj = countKeyframes - 1; jj >= 1; jj--)
                                    {
                                        VideoMotionKeyframe thisKeyframe = vKeyframes[jj] as VideoMotionKeyframe;
                                        Timecode time = thisKeyframe.Position;
                                        OFXInterpolationType type = TypeChange(thisKeyframe.Type);

                                        if (transformScale.IsAnimated)
                                        {
                                            transformScale.SetValueAtTime(time, new OFXDouble2D { X = Scale.X * Ratio(thisKeyframe), Y = Scale.Y * Ratio(thisKeyframe)});
                                            transformScale.Keyframes[1].Interpolation = type;
                                        }

                                        if (transformRotate.IsAnimated)
                                        {
                                            transformRotate.SetValueAtTime(time, thisKeyframe.Rotation / Math.PI * 180 * (isXFlip ? -1 : 1) * (isYFlip ? -1 : 1));
                                            transformRotate.Keyframes[1].Interpolation = type;
                                        }

                                        if (transformTranslate.IsAnimated)
                                        {
                                            transformTranslate.SetValueAtTime(time, PointTranslate(thisKeyframe, transformRotate.IsAnimated));
                                            transformTranslate.Keyframes[1].Interpolation = type;
                                        }

                                        if (transformCenter.IsAnimated)
                                        {
                                            transformCenter.SetValueAtTime(time, new OFXDouble2D { X = Pos.X + (thisKeyframe.Center.X - dFullWidth / 2) * (transformRotate.IsAnimated ? 1 : 0), Y = Pos.Y - (thisKeyframe.Center.Y - dFullHeight / 2) * (transformRotate.IsAnimated ? 1 : 0)});
                                            transformCenter.Keyframes[1].Interpolation = type;
                                        }
                                    }

                                    // Change Scale keyframe types
                                    if (transformScale.IsAnimated && SCALETYPEFACTOR > 1) 
                                    {
                                        for (int jj = countKeyframes - 1; jj >= 1; jj--)
                                        transformScale.Keyframes[jj - 1].Interpolation = TypeChange(vKeyframes[jj - 1].Type, transformScale.Keyframes[jj].Value.X / transformScale.Keyframes[jj - 1].Value.X);
                                    }
                                }
                            }

                            // Set Pan/Crop keyframes
                            vKeyframes.Clear();
                            vKeyframes[0].Rotation = 0;
                            vKeyframes[0].Center = new VideoMotionVertex(0f, 0f);
                            vKeyframes[0].Type = VideoKeyframeType.Linear;
                            VideoMotionBounds bounds = new VideoMotionBounds(new VideoMotionVertex(0f, 0f), new VideoMotionVertex((float)scrWidth, 0f), new VideoMotionVertex((float)scrWidth, (float)scrHeight), new VideoMotionVertex(0f, (float)scrHeight));
                            vKeyframes[0].Bounds = bounds;
                            vKeyframes[0].MoveBy(new VideoMotionVertex((int)dFullWidth / 2 - scrWidth / 2, (int)dFullHeight / 2 - scrHeight / 2));

                            // If the material's height (or width) is larger than the project's, add PositionOFX
                            if (dFullWidth > scrWidth || dFullHeight > scrHeight)
                            {
                                effectCount = vEvent.Effects.Count;
                                for (int i = effectCount - 1; i >= 0; i--)
                                {
                                    if (vEvent.Effects[i].PlugIn.UniqueID == plugin2.UniqueID && vEvent.Effects[i].ApplyBeforePanCrop == true)
                                    {
                                        break;
                                    }
                                    if (i == 0)
                                    {
                                        Effect effect2 = new Effect(plugin2);
                                        vEvent.Effects.Insert(countBefore, effect2);
                                        effect2.ApplyBeforePanCrop = true;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        public MediaStream GetActiveMediaStream(TrackEvent trackEvent)
        {
            try
            {
                if (!(trackEvent.ActiveTake.IsValid()))
                {
                    throw new ArgumentException("empty or invalid take");
                }

                Media media = myVegas.Project.MediaPool.Find(trackEvent.ActiveTake.MediaPath);
                if (null == media)
                {
                    MessageBox.Show("missing media");
                    throw new ArgumentException("missing media");
                }

                MediaStream mediaStream = media.Streams.GetItemByMediaType(MediaType.Video, trackEvent.ActiveTake.StreamIndex);
                return mediaStream;
            }
            catch (Exception e)
            {
                MessageBox.Show("{0}", e.Message);
                return null;
            }
        }

        public double PointDistance(VideoMotionVertex point1, VideoMotionVertex point2)
        {
            double distance = Math.Sqrt(Math.Pow(point1.X - point2.X, 2) + Math.Pow(point1.Y - point2.Y, 2));
            return distance;
        }

        public double Ratio(VideoMotionKeyframe keyframe, bool forScale = true)
        {
            double ratio = forScale ? Math.Max(firstWidth/PointDistance(keyframe.TopLeft, keyframe.TopRight), firstHeight/PointDistance(keyframe.TopLeft, keyframe.BottomLeft)) : Math.Min(scrWidth/PointDistance(keyframe.TopLeft, keyframe.TopRight), scrHeight/PointDistance(keyframe.TopLeft, keyframe.BottomLeft));
            return ratio;
        }

        public OFXDouble2D PointTranslate(VideoMotionKeyframe keyframe, bool rotateMode = false)
        {
            double rotation = keyframe.Rotation, ratio = Ratio(keyframe, false), pointX, pointY;
            if (rotateMode)
            {
                pointX = ((keyframe.TopLeft.X + keyframe.BottomRight.X) / 2 - keyframe.Center.X) * ratio * (-1);
                pointY = ((keyframe.TopLeft.Y + keyframe.BottomRight.Y) / 2 - keyframe.Center.Y) * ratio;
            }
            else
            {
                pointX = (keyframe.TopLeft.X + keyframe.BottomRight.X - dFullWidth) / 2 * ratio * (-1);
                pointY = (keyframe.TopLeft.Y + keyframe.BottomRight.Y - dFullHeight) / 2 * ratio;
            }
            OFXDouble2D point = new OFXDouble2D { X = pointX, Y = pointY};
            point = PointRotate(point, rotation);
            return point;
        }

        public OFXDouble2D PointRotate(OFXDouble2D point, double rotation)
        {
            double pointX = point.X * Math.Cos(rotation) - point.Y * Math.Sin(rotation);
            double pointY = point.X * Math.Sin(rotation) + point.Y * Math.Cos(rotation);
            OFXDouble2D point0 = new OFXDouble2D { X = pointX, Y = pointY};
            return point0;
        }

        public bool PointEqual(VideoMotionVertex point1, VideoMotionVertex point2)
        {
            bool pointEqual = Math.Round(point1.X - point2.X, 4) == 0 && Math.Round(point1.Y - point2.Y, 4) == 0;
            return pointEqual;
        }

        public bool PointEqual(OFXDouble2D point1, OFXDouble2D point2)
        {
            bool pointEqual = Math.Round(point1.X - point2.X, 4) == 0 && Math.Round(point1.Y - point2.Y, 4) == 0;
            return pointEqual;
        }

        public OFXInterpolationType TypeChange(VideoKeyframeType type, double scaleFactor = 1)
        {
            switch (type)
            {
                case VideoKeyframeType.Hold:
                    return OFXInterpolationType.Hold;
                case VideoKeyframeType.Slow:
                    return OFXInterpolationType.Slow;
                case VideoKeyframeType.Fast:
                    return OFXInterpolationType.Fast;
                case VideoKeyframeType.Smooth:
                    return OFXInterpolationType.Smooth;
                case VideoKeyframeType.Sharp:
                    return OFXInterpolationType.Sharp;
                default:
                    if (scaleFactor != 1)
                    {
                        if (scaleFactor >= SCALETYPEFACTOR)
                        {
                            return OFXInterpolationType.Slow;
                        }
                        else if(scaleFactor <= 1/SCALETYPEFACTOR)
                        {
                            return OFXInterpolationType.Fast;
                        }
                    }
                    return OFXInterpolationType.Linear;
            }
        }
    }
}


public class EntryPoint
{
    public void FromVegas(Vegas vegas)
    //public void FromVegas(Vegas vegas, String scriptFile, XmlDocument scriptSettings, ScriptArgs args)
    {
        Test_Script1.Class1 test1 = new Test_Script1.Class1();
        test1.Main(vegas);
    }
}
