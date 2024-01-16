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
        public PlugInNode plugin1;
        public PlugInNode plugin2;
        public Effect effect1;
        public Effect effect2;
        public bool resetMode;
        public float scrWidth;
        public float scrHeight;
        public float dFullWidth;
        public float dFullHeight;
        public void Main(Vegas vegas)
        {
            myVegas = vegas;
            plugin1 = myVegas.VideoFX.GetChildByUniqueID("{Svfx:net.sf.openfx.MzTransformPlugin}");
            /* if (plugin1 == null)
            {
                plugin1 = myVegas.VideoFX.GetChildByUniqueID("{Svfx:net.sf.openfx.TransformPlugin}");
                plugin2 = myVegas.VideoFX.GetChildByUniqueID("{Svfx:net.sf.openfx.Mirror}");
            } */
            scrWidth = myVegas.Project.Video.Width;
            scrHeight = myVegas.Project.Video.Height;
            resetMode = ((Control.ModifierKeys & Keys.Control) != 0) ? true : false;
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
                            double rotationSave = vKeyframes[0].Rotation;
                            vKeyframes[0].Rotation = 1;
                            bool isXFlip = (vKeyframes[0].TopLeft.X - vKeyframes[0].TopRight.X) * Math.Cos(vKeyframes[0].Rotation) > 0, isYFlip = (vKeyframes[0].TopRight.Y - vKeyframes[0].BottomRight.Y) * Math.Cos(vKeyframes[0].Rotation) > 0;
                            vKeyframes[0].Rotation = rotationSave;
                            int theLastOne = 0;

                            int effectCount = vEvent.Effects.Count;
                            for (int i = effectCount - 1; i >= 0; i--)
                            {
                                if (vEvent.Effects[i].PlugIn.UniqueID == plugin1.UniqueID)
                                {
                                    if (resetMode)
                                    {
                                        vEvent.Effects.RemoveAt(i);
                                    }
                                    else
                                    {
                                        if (theLastOne < i + 1)
                                        {
                                            theLastOne = i + 1;
                                        }
                                    }
                                }
                            }

                            effect1 = new Effect(plugin1);
                            vEvent.Effects.Insert(theLastOne, effect1);
                            
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

                            if (theLastOne > 0)
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
                                transformScale.Value = new OFXDouble2D { X = Scale.X * Ratio(vKeyframes[0]), Y = Scale.Y * Ratio(vKeyframes[0])};
                                transformRotate.Value = vKeyframes[0].Rotation / Math.PI * 180 * (isXFlip ? -1 : 1) * (isYFlip ? -1 : 1);
                                transformTranslate.Value = PointTranslate(vKeyframes[0], Ratio(vKeyframes[0], false));
                                transformCenter.Value = new OFXDouble2D { X = Pos.X, Y = Pos.Y};
                                OFXInterpolationType type0 = TypeChange(vKeyframes[0].Type);

                                if (countKeyframes > 1)
                                {
                                    // Verify if we need to animate these keyframes
                                    int nScale = 0, nScaleN = 0, nRotate = 0, nRotateN = 0, nTranslate = 0, nTranslateN = 0,nCenter = 0, nCenterN = 0, nTranslateRotate = 0, nTranslateRotateN = 0;
                                    for (int jj = countKeyframes - 1; jj >= 1; jj--)
                                    {
                                        VideoMotionKeyframe thisKeyframe = vKeyframes[jj] as VideoMotionKeyframe;
                                        nScale = Math.Round(PointDistance(thisKeyframe.TopLeft, thisKeyframe.TopRight) - PointDistance(vKeyframes[0].TopLeft, vKeyframes[0].TopRight), 4) == 0 ? 0 : 1;
                                        nScaleN += nScale;
                                        nRotate = Math.Round(thisKeyframe.Rotation - vKeyframes[0].Rotation, 4) == 0 ? 0 : 1;
                                        nRotateN += nRotate;
                                        nTranslate = PointEqual(PointTranslate(thisKeyframe), PointTranslate(vKeyframes[0])) ? 0 : 1;
                                        nTranslateN += nTranslate;
                                        nTranslateRotate = PointEqual(PointTranslate(thisKeyframe, rotateMode : true), PointTranslate(vKeyframes[0], rotateMode : true)) ? 0 : 1;
                                        nTranslateRotateN += nTranslateRotate;
                                        nCenter = PointEqual(thisKeyframe.Center, vKeyframes[0].Center) ? 0 : 1;
                                        nCenterN += nCenter;
                                    }
                                    transformScale.IsAnimated = Convert.ToBoolean(nScaleN);
                                    if (transformScale.IsAnimated) {transformScale.Keyframes[0].Interpolation = type0;}
                                    transformRotate.IsAnimated = Convert.ToBoolean(nRotateN);
                                    if (transformRotate.IsAnimated) {transformRotate.Keyframes[0].Interpolation = type0;transformTranslate.Value = PointTranslate(vKeyframes[0], Ratio(vKeyframes[0], false), true);transformCenter.Value = new OFXDouble2D { X = Pos.X + vKeyframes[0].Center.X - dFullWidth / 2, Y = Pos.Y - vKeyframes[0].Center.Y + dFullHeight / 2};}
                                    transformTranslate.IsAnimated = Convert.ToBoolean(nRotateN) ? Convert.ToBoolean(nTranslateRotateN) : Convert.ToBoolean(nTranslateN);
                                    if (transformTranslate.IsAnimated) {transformTranslate.Keyframes[0].Interpolation = type0;}
                                    transformCenter.IsAnimated = Convert.ToBoolean(nCenterN) && Convert.ToBoolean(nRotateN);
                                    if (transformCenter.IsAnimated) {transformCenter.Keyframes[0].Interpolation = type0;}

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
                                            transformTranslate.SetValueAtTime(time, PointTranslate(thisKeyframe, Ratio(thisKeyframe, false), transformRotate.IsAnimated));
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


                            MatchAspect(evnt);

                            foreach (VideoMotionKeyframe MyKF in vKeyframes)
                            {
                                float dWidth = Math.Abs(MyKF.TopRight.X - MyKF.TopLeft.X);
                                float dHeight = Math.Abs(MyKF.BottomLeft.Y - MyKF.TopLeft.Y);

                                float pwid = 0.0F;

                                if (dFullHeight > scrHeight)
                                {
                                    pwid = dFullHeight / dHeight * 100;
                                }
                                else
                                {
                                    pwid = dHeight / scrHeight * 100;
                                }

                                ScaleKeyframe(MyKF, pwid, 0);
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

        public void ScaleKeyframe(VideoMotionKeyframe keyframe, float szChange, float rotAngle)
        {
            float cWidth = (1 / (szChange / 100));
            float cHeight = (1 / (szChange / 100));

            if (szChange > 100)
            {
                cWidth = (szChange / 100);
                cHeight = (szChange / 100);
            }

            VideoMotionVertex bounds2 = new VideoMotionVertex(cWidth, cHeight);

            keyframe.ScaleBy(bounds2);
            keyframe.RotateBy((rotAngle * (Math.PI / 180)));
        }

        public void MatchAspect(TrackEvent trackEvent)
        {
            float dWidthProject = myVegas.Project.Video.Width;
            float dHeightProject = myVegas.Project.Video.Height;
            double dPixelAspect = myVegas.Project.Video.PixelAspectRatio;
            double dAspect = dPixelAspect * dWidthProject / dHeightProject;

            MediaStream mediaStream = GetActiveMediaStream(trackEvent);
            if (!(mediaStream == null))
            {
                VideoStream videoStream = mediaStream as VideoStream;

                double dMediaPixelAspect = videoStream.PixelAspectRatio;
                VideoEvent videoEvent = trackEvent as VideoEvent;
                VideoMotionKeyframes keyframes = videoEvent.VideoMotion.Keyframes;
                keyframes.Clear();
                MatchOutputAspect(keyframes[0], dMediaPixelAspect, dAspect);
            }
            myVegas.UpdateUI();
        }


        static void Swap(VideoMotionVertex a,VideoMotionVertex b)
        {
            VideoMotionVertex temp = a;
            a = b;
            b = temp;
        }

        public void MatchOutputAspect(VideoMotionKeyframe keyframe, double dMediaPixelAspect, double dAspectOut)
        {
            foreach (Track myTrack in myVegas.Project.Tracks)
            {
                if (myTrack.IsVideo())
                {
                    foreach (VideoEvent vEvent in myTrack.Events)
                    {
                        if (vEvent.Selected)
                        {
                            VideoMotionKeyframe keyframeSave = keyframe;

                            try
                            {
                                VideoStream vs = (VideoStream)vEvent.ActiveTake.Media.Streams.GetItemByMediaType(MediaType.Video,vEvent.ActiveTake.StreamIndex);
                                int mHeight = vs.Height;
                                int mWidth = vs.Width;

                                keyframe.Rotation = 0.0;

                                VideoMotionBounds bounds1 = new VideoMotionBounds(keyframe.TopLeft, keyframe.TopRight, keyframe.BottomRight, keyframe.BottomLeft);
                                bounds1.TopLeft = new VideoMotionVertex(0f, 0f);
                                bounds1.TopRight = new VideoMotionVertex((float)mWidth, 0f);
                                bounds1.BottomLeft = new VideoMotionVertex(0f, (float)mHeight);
                                bounds1.BottomRight = new VideoMotionVertex((float)mWidth, (float)mHeight);
                                keyframe.Bounds = bounds1;

                                float dWidth = Math.Abs(keyframe.TopRight.X - keyframe.TopLeft.X);
                                float dHeight = Math.Abs(keyframe.BottomLeft.Y - keyframe.TopLeft.Y);
                                double dCurrentAspect = dMediaPixelAspect * dWidth / dHeight;
                                float centerY = keyframe.Center.Y;
                                float centerX = keyframe.Center.X;
                                double dFactor1, dFactor2;

                                VideoMotionBounds bounds2 = new VideoMotionBounds(keyframe.TopLeft, keyframe.TopRight, keyframe.BottomRight, keyframe.BottomLeft);

                                if (dCurrentAspect < dAspectOut)
                                {
                                    // alter y coords            
                                    dFactor1 = dCurrentAspect / dAspectOut;
                                    dFactor2 = 1;
                                }
                                else
                                {
                                    // alter x coords
                                    dFactor1 = 1;
                                    dFactor2 = dAspectOut / dCurrentAspect;
                                }

                                bounds2.TopLeft = new VideoMotionVertex((bounds2.TopLeft.X - centerX) * (float)dFactor2 + mWidth / 2, (bounds2.TopLeft.Y - centerY) * (float)dFactor1 + mHeight / 2);
                                bounds2.TopRight = new VideoMotionVertex((bounds2.TopRight.X - centerX) * (float)dFactor2 + mWidth / 2, (bounds2.TopRight.Y - centerY) * (float)dFactor1 + mHeight / 2);
                                bounds2.BottomLeft = new VideoMotionVertex((bounds2.BottomLeft.X - centerX) * (float)dFactor2 + mWidth / 2, (bounds2.BottomLeft.Y - centerY) * (float)dFactor1 + mHeight / 2);
                                bounds2.BottomRight = new VideoMotionVertex((bounds2.BottomRight.X - centerX) * (float)dFactor2 + mWidth / 2, (bounds2.BottomRight.Y - centerY) * (float)dFactor1 + mHeight / 2);

                                // set it to new bounds2
                                keyframe.Bounds = bounds2;
                                keyframe.Type = VideoKeyframeType.Linear;
                            }
                            catch
                            {
                                // restore original settings on error
                                keyframe = keyframeSave;
                            }
                        }
                    }
                }
            }
        }

        public double PointDistance(VideoMotionVertex point1, VideoMotionVertex point2)
        {
            double distance = Math.Sqrt(Math.Pow(point1.X - point2.X, 2) + Math.Pow(point1.Y - point2.Y, 2));
            return distance;
        }

        public double Ratio(VideoMotionKeyframe keyframe, bool forScale = true)
        {
            double ratio = forScale ? Math.Max(dFullWidth/PointDistance(keyframe.TopLeft, keyframe.TopRight), dFullHeight/PointDistance(keyframe.TopLeft, keyframe.BottomLeft)) : Math.Min(scrWidth/PointDistance(keyframe.TopLeft, keyframe.TopRight), scrHeight/PointDistance(keyframe.TopLeft, keyframe.BottomLeft));
            return ratio;
        }

        public OFXDouble2D PointTranslate(VideoMotionKeyframe keyframe, double ratio = 1, bool rotateMode = false)
        {
            double rotation = keyframe.Rotation, pointX, pointY;
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
            bool pointEqual = Math.Round(point1.X - point2.X) == 0 && Math.Round(point1.Y - point2.Y) == 0;
            return pointEqual;
        }

        public bool PointEqual(OFXDouble2D point1, OFXDouble2D point2)
        {
            bool pointEqual = Math.Round(point1.X - point2.X) == 0 && Math.Round(point1.Y - point2.Y) == 0;
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