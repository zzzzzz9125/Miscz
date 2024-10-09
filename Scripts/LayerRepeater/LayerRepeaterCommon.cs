#if !Sony
using ScriptPortal.Vegas;
#else
using Sony.Vegas;
#endif

using System;
using System.Drawing;
using System.Windows.Forms;
using System.Collections.Generic;
using System.Runtime.Remoting.Contexts;
using System.Runtime.CompilerServices;

namespace LayerRepeater
{
    public static class LayerRepeaterCommon
    {
        public const string VERSION = "v1.10";
        public static void CopyTrackKeyframe(this Project project, TrackMotionKeyframe source, TrackMotionKeyframe target, int type = 0, bool is3D = false)
        {
            project.CopyTrackKeyframeBase(source, target, type);
            if (is3D)
            {
                target.PositionZ = source.PositionZ;
                target.Depth = source.Depth;
                target.RotationX = source.RotationX;
                target.RotationY = source.RotationY;
                target.RotationOffsetZ = source.RotationOffsetZ;
                target.OrientationX = source.OrientationX;
                target.OrientationY = source.OrientationY;
            }
        }

        public static void CopyTrackKeyframe(this Project project, TrackShadowKeyframe source, TrackShadowKeyframe target, int type = 0)
        {
            project.CopyTrackKeyframeBase(source, target, type);
            target.Blur = source.Blur;
            target.Intensity = source.Intensity;
            target.Color = source.Color;
        }

        public static void CopyTrackKeyframe(this Project project, TrackGlowKeyframe source, TrackGlowKeyframe target, int type = 0)
        {
            project.CopyTrackKeyframeBase(source, target, type);
            target.Blur = source.Blur;
            target.Intensity = source.Intensity;
            target.Color = source.Color;
        }

        public static void CopyTrackKeyframeBase(this Project project, BaseTrackMotionKeyframe source, BaseTrackMotionKeyframe target, int type = 0)
        {
            target.Position = source.Position;
            target.Smoothness = source.Smoothness;
            target.Type = source.Type;
            target.PositionX = source.PositionX;
            target.PositionY = source.PositionY;
            target.Width = source.Width;
            switch (type)
            {
                // Child to Parent
                case 1: target.Width /= 1.0 * project.Video.Width / project.Video.Height; break;

                // Parent to Child
                case 2: target.Width *= 1.0 * project.Video.Width / project.Video.Height; break;

                default: break;
            }
            target.Height = source.Height;
            target.RotationZ = source.RotationZ;
            target.RotationOffsetX = source.RotationOffsetX;
            target.RotationOffsetY = source.RotationOffsetY;
            target.OrientationZ = source.OrientationZ;
        }

        public static float FadeCurveCalculate(double t, CurveType type, bool fadeOut = false)
        {
            t = (fadeOut ? type == CurveType.Fast : type == CurveType.Slow) ? Math.Pow(t, 2)
              : (fadeOut ? type == CurveType.Slow : type == CurveType.Fast) ? t * (2 - t)
              : type == CurveType.Smooth ? Math.Pow(t, 2) * (3 - t * 2)
              : type == CurveType.Sharp ? (Math.Pow(t - 0.5, 3) * 4 + 0.5) : t;
            return (float)t;
        }

        public static bool[] GetFxSplitBools(this VideoEvent ev, int fxSplitStatus)
        {
            if (fxSplitStatus == 2)
            {
                return new bool[0];
            }

            bool[] result = new bool[ev.Effects.Count];
            foreach (Effect ef in ev.Effects)
            {
                result[ef.Index] = false;

                if (!ef.Bypass)
                {
                    if (!ef.PlugIn.IsOFX)
                    {
                        result[ef.Index] = ef.Keyframes.Count > 1;
                    }
                    else
                    {
                        foreach (OFXParameter p in ef.OFXEffect.Parameters)
                        {
                            if (p.IsAnimated && p.Enabled)
                            {
                                int c = 1;
                                switch (p.ParameterType)
                                {
                                    case OFXParameterType.Boolean: c = ((OFXBooleanParameter)p).Keyframes.Count; break;
                                    case OFXParameterType.Choice: c = ((OFXChoiceParameter)p).Keyframes.Count; break;
                                    case OFXParameterType.Custom: c = ((OFXCustomParameter)p).Keyframes.Count; break;
                                    case OFXParameterType.Double2D: c = ((OFXDouble2DParameter)p).Keyframes.Count; break;
                                    case OFXParameterType.Double3D: c = ((OFXDouble3DParameter)p).Keyframes.Count; break;
                                    case OFXParameterType.Double: c = ((OFXDoubleParameter)p).Keyframes.Count; break;
                                    case OFXParameterType.Integer2D: c = ((OFXInteger2DParameter)p).Keyframes.Count; break;
                                    case OFXParameterType.Integer3D: c = ((OFXInteger3DParameter)p).Keyframes.Count; break;
                                    case OFXParameterType.Integer: c = ((OFXIntegerParameter)p).Keyframes.Count; break;
                                    case OFXParameterType.RGBA: c = ((OFXRGBAParameter)p).Keyframes.Count; break;
                                    case OFXParameterType.RGB: c = ((OFXRGBParameter)p).Keyframes.Count; break;
                                    case OFXParameterType.String: c = ((OFXStringParameter)p).Keyframes.Count; break;
                                }
                                if (c > 1)
                                {
                                    result[ef.Index] = true;
                                    break;
                                }
                            }
                        }
                    }

                    if (fxSplitStatus == 0 && result[ef.Index])
                    {
                        result[ef.Index] = DialogResult.Yes == MessageBox.Show(string.Format(L.FxSplitMessage, ef.Index + 1, ef.PlugIn.Name), L.FxSplitCaption, MessageBoxButtons.YesNo);
                    }
                }
            }
            return result;
        }


        public static bool GetRange(this Vegas myVegas, VideoEvent vEvent, LayerRepeaterArgs args, out Timecode start, out Timecode end)
        {
            start = new Timecode(0);
            end = vEvent.Length;

            bool rangeTypeSucceed = false;

            do
            {
                switch (args.RangeType)
                {
                    case 1:
                        if (vEvent.VideoMotion.Keyframes.Count > 1)
                        {
                            start = vEvent.VideoMotion.Keyframes[0].Position;
                            end = vEvent.VideoMotion.Keyframes[vEvent.VideoMotion.Keyframes.Count - 1].Position;
                            rangeTypeSucceed = true;
                        }
                        break;

                    case 2:
                        Timecode startFx = end, endFx = start;
                        foreach (Effect ef in vEvent.Effects)
                        {
                            if (!ef.Bypass)
                            {
                                if (!ef.PlugIn.IsOFX)
                                {
                                    foreach (Keyframe k in ef.Keyframes)
                                    {
                                        if (k.Position < startFx)
                                        {
                                            startFx = k.Position;
                                        }
                                        if (k.Position > endFx)
                                        {
                                            endFx = k.Position;
                                        }
                                    }
                                }
                                else
                                {
                                    List<OFXKeyframe> kfs = new List<OFXKeyframe>();
                                    foreach (OFXParameter p in ef.OFXEffect.Parameters)
                                    {
                                        if (p.IsAnimated && p.Enabled)
                                        {
                                            switch (p.ParameterType)
                                            {
                                                case OFXParameterType.Boolean: kfs.AddRange(((OFXBooleanParameter)p).Keyframes); break;
                                                case OFXParameterType.Choice: kfs.AddRange(((OFXChoiceParameter)p).Keyframes); break;
                                                case OFXParameterType.Custom: kfs.AddRange(((OFXCustomParameter)p).Keyframes); break;
                                                case OFXParameterType.Double2D: kfs.AddRange(((OFXDouble2DParameter)p).Keyframes); break;
                                                case OFXParameterType.Double3D: kfs.AddRange(((OFXDouble3DParameter)p).Keyframes); break;
                                                case OFXParameterType.Double: kfs.AddRange(((OFXDoubleParameter)p).Keyframes); break;
                                                case OFXParameterType.Integer2D: kfs.AddRange(((OFXInteger2DParameter)p).Keyframes); break;
                                                case OFXParameterType.Integer3D: kfs.AddRange(((OFXInteger3DParameter)p).Keyframes); break;
                                                case OFXParameterType.Integer: kfs.AddRange(((OFXIntegerParameter)p).Keyframes); break;
                                                case OFXParameterType.RGBA: kfs.AddRange(((OFXRGBAParameter)p).Keyframes); break;
                                                case OFXParameterType.RGB: kfs.AddRange(((OFXRGBParameter)p).Keyframes); break;
                                                case OFXParameterType.String: kfs.AddRange(((OFXStringParameter)p).Keyframes); break;
                                            }
                                        }
                                    }
                                    kfs.SortByTime();
                                    if (kfs.Count > 1)
                                    {
                                        startFx = kfs[0].Time;
                                        endFx = kfs[kfs.Count - 1].Time;
                                    }
                                }
                            }
                        }
                        if (startFx < endFx)
                        {
                            start = startFx;
                            end = endFx;
                            rangeTypeSucceed = true;
                        }
                        break;

                    case 3:
                        Timecode t = myVegas.Transport.PlayCursorPosition - vEvent.Start;
                        if (t >= new Timecode(0) && t <= vEvent.Length)
                        {
                            start = t;
                            end = t;
                            rangeTypeSucceed = true;
                        }
                        break;

                    case 4:
                        Timecode t1 = myVegas.Transport.LoopRegionStart - vEvent.Start, t2 = myVegas.Transport.LoopRegionStart + myVegas.Transport.LoopRegionLength - vEvent.Start;
                        if (myVegas.Transport.LoopRegionLength != new Timecode(0))
                        {
                            if (t1 > t2)
                            {
                                (t2, t1) = (t1, t2);
                            }

                            if (t1 >= new Timecode(0) && t2 <= vEvent.Length)
                            {
                                start = t1;
                                end = t2;
                                rangeTypeSucceed = true;
                            }
                        }
                        break;

                    default:
                        rangeTypeSucceed = true;
                        break;
                }
                if (!rangeTypeSucceed && args.RangeAdapt)
                {
                    args.RangeType = args.RangeType == 1 ? 0 : args.RangeType == 2 ? 1 : args.RangeType == 3 ? 0 : 3;
                }
            } while (!rangeTypeSucceed && args.RangeAdapt);

            if (!rangeTypeSucceed)
            {
                MessageBox.Show(args.RangeType == 1 ? L.RangeError1 : args.RangeType == 2 ? L.RangeError2 : args.RangeType == 3 ? L.RangeError3 : L.RangeError4);
            }

            return rangeTypeSucceed;
        }

        public static void RepeatLayer(this Vegas myVegas, VideoEvent vEvent, LayerRepeaterArgs args)
        {
            if (!myVegas.GetRange(vEvent, args, out Timecode start, out Timecode end))
            {
                return;
            }

            Project project = myVegas.Project;

            vEvent.Mute = false;

            VideoTrack vTrack = (VideoTrack)vEvent.Track;

            if (args.Mode == 1)
            {
                args.Count = (int)Math.Floor(args.TrueSpeed * (end - start).ToMilliseconds() / 1000 + 1);
            }

            if (args.Mode == 0 && args.Count == 0)
            {
                project.TransferTrackMotion(vTrack, vTrack, 2);
            }

            TrackEventGroup grp = vEvent.Group;
            if (grp == null)
            {
                grp = new TrackEventGroup(project);
                project.Groups.Add(grp);
                grp.Add(vEvent);
            }

            else
            {
                List<TrackEvent> evs = new List<TrackEvent>();
                foreach (TrackEvent ev in grp)
                {
                    if (ev.Track != vTrack && ev.Track.IsVideo())
                    {
                        if (((VideoTrack)ev.Track).CompositeNestingLevel > vTrack.CompositeNestingLevel)
                        {
                            evs.Add(ev);
                        }
                    }
                }
                foreach (TrackEvent ev in evs)
                {
                    if (ev.Track.Events.Count == 1)
                    {
                        project.Tracks.Remove(ev.Track);
                    }
                    else
                    {
                        ev.Track.Events.Remove(ev);
                    }
                }
            }

            List<VideoTrack> vTrkList = new List<VideoTrack>();
            for (int i = vTrack.Index + 1; i < project.Tracks.Count; i++)
            {
                if (!project.Tracks[i].IsVideo())
                {
                    break;
                }

                VideoTrack trk = (VideoTrack)project.Tracks[i];
                if (trk.CompositeNestingLevel > vTrack.CompositeNestingLevel)
                {
                    vTrkList.Add(trk);
                }
                else
                {
                    break;
                }
            }

            if (args.Count == 0)
            {
                return;
            }

            bool[] fxSplitBools = vEvent.GetFxSplitBools(args.FxSplitType);

            for (int i = args.Count; i > 0; i--)
            {
                VideoTrack newTrack = null;
                if (i > vTrkList.Count)
                {
                    newTrack = new VideoTrack(project, vTrack.Index + vTrkList.Count + 1, null);
                    project.Tracks.Add(newTrack);

                }
                else
                {
                    newTrack = vTrkList[i - 1];
                }

                if (i == 1 && i != args.Count)
                {
                    newTrack.CompositeNestingLevel = 0;
                    newTrack.Selected = true;
                    project.GroupSelectedTracks().CollapseTrackGroup();
                    newTrack.Selected = false;
                }
                newTrack.CompositeNestingLevel = 1;

                newTrack.SetCompositeMode(vTrack.CompositeMode, false);
                newTrack.Name = vTrack.Name;
                newTrack.Solo = vTrack.Solo;
                newTrack.Mute = vTrack.Mute;

                int ii = args.Reverse ? (args.Count - i + 1) : i;

                VideoEvent newEvent = (VideoEvent)vEvent.Copy(newTrack, vEvent.Start);
                VideoMotionKeyframes kfs = newEvent.VideoMotion.Keyframes;
                double posTime = args.Mode == 0 ? (end - start).ToMilliseconds() * (args.Count - ii) / (args.Count - 1)
                                                : ((args.Count - ii) * 1000 / args.TrueSpeed);
                Timecode pos =  new Timecode(posTime) + start;
                if (pos > end)
                {
                    pos = end;
                }
                if (ii != args.Count)
                {
                    newEvent.Start += pos;
                    VideoMotionKeyframe kf = new VideoMotionKeyframe(project, pos);
                    kfs.Add(kf);
                    kf.Type = VideoKeyframeType.Linear;

                    while (kfs[0].Position != pos)
                    {
                        kfs.RemoveAt(0);
                    }

                    kfs.Clear();
                    kfs[0].Position = new Timecode(0);
                }
                else
                {
                    for (int j = kfs.Count - 1; j >= 0; j--)
                    {
                        if (kfs[j].Position > start)
                        {
                            kfs.RemoveAt(j);
                        }
                        else
                        {
                            break;
                        }
                    }
                }

                foreach (Effect ef in newEvent.Effects)
                {
                    if (ef.Bypass || ef.Index >= fxSplitBools.Length || !fxSplitBools[ef.Index])
                    {
                        continue;
                    }

                    if (!ef.PlugIn.IsOFX)
                    {
                        Keyframe ekf = new Keyframe(pos);
                        bool success = false;

                        foreach (Keyframe k in ef.Keyframes)
                        {
                            if (k.Position == pos)
                            {
                                ekf = k;
                                success = true;
                                break;
                            }
                        }

                        if (!success)
                        {
                            ef.Keyframes.Add(ekf);
                        }

                        if (ef.Keyframes[0].Position < ekf.Position)
                        {
                            ef.Keyframes.RemoveAt(0);
                        }
                        ekf.Position = new Timecode(0);
                        for (int j = ef.Keyframes.Count - 1; j >= 1; j--)
                        {
                            ef.Keyframes.RemoveAt(j);
                        }
                    }
                    else
                    {
                        if (ef.PlugIn != vEvent.Effects[ef.Index].PlugIn)
                        {
                            continue;
                        }
                        foreach (OFXParameter p in ef.OFXEffect.Parameters)
                        {
                            if (!p.IsAnimated || !p.Enabled || ef.Index >= vEvent.Effects.Count)
                            {
                                continue;
                            }

                            OFXParameter p2 = vEvent.Effects[ef.Index].OFXEffect.FindParameterByName(p.Name);

                            p.IsAnimated = false;

                            switch (p.ParameterType)
                            {
                                case OFXParameterType.Boolean: ((OFXBooleanParameter)p).Value = ((OFXBooleanParameter)p2).GetValueAtTime(pos); break;
                                case OFXParameterType.Choice: ((OFXChoiceParameter)p).Value = ((OFXChoiceParameter)p2).GetValueAtTime(pos); break;
                                case OFXParameterType.Custom: ((OFXCustomParameter)p).Value = ((OFXCustomParameter)p2).GetValueAtTime(pos); break;
                                case OFXParameterType.Double2D: ((OFXDouble2DParameter)p).Value = ((OFXDouble2DParameter)p2).GetValueAtTime(pos); break;
                                case OFXParameterType.Double3D: ((OFXDouble3DParameter)p).Value = ((OFXDouble3DParameter)p2).GetValueAtTime(pos); break;
                                case OFXParameterType.Double: ((OFXDoubleParameter)p).Value = ((OFXDoubleParameter)p2).GetValueAtTime(pos); break;
                                case OFXParameterType.Integer2D: ((OFXInteger2DParameter)p).Value = ((OFXInteger2DParameter)p2).GetValueAtTime(pos); break;
                                case OFXParameterType.Integer3D: ((OFXInteger3DParameter)p).Value = ((OFXInteger3DParameter)p2).GetValueAtTime(pos); break;
                                case OFXParameterType.Integer: ((OFXIntegerParameter)p).Value = ((OFXIntegerParameter)p2).GetValueAtTime(pos); break;
                                case OFXParameterType.RGBA: ((OFXRGBAParameter)p).Value = ((OFXRGBAParameter)p2).GetValueAtTime(pos); break;
                                case OFXParameterType.RGB: ((OFXRGBParameter)p).Value = ((OFXRGBParameter)p2).GetValueAtTime(pos); break;
                                case OFXParameterType.String: ((OFXStringParameter)p).Value = ((OFXStringParameter)p2).GetValueAtTime(pos); break;
                            }
                        }
                    }
                }

                newEvent.End = vEvent.End;
                newEvent.Selected = false;
                newEvent.FadeIn.Length = new Timecode(0);
                newEvent.FadeOut.Length = new Timecode(0);
                if (newEvent.Start < vEvent.Start + vEvent.FadeIn.Length)
                {
                    double t = (newEvent.Start - vEvent.Start).ToMilliseconds() / vEvent.FadeIn.Length.ToMilliseconds();
                    newEvent.FadeIn.Gain *= FadeCurveCalculate(t, vEvent.FadeIn.Curve);
                }
                else if (newEvent.Start > vEvent.End - vEvent.FadeOut.Length)
                {
                    double t = (vEvent.End - newEvent.Start).ToMilliseconds() / vEvent.FadeOut.Length.ToMilliseconds();
                    newEvent.FadeIn.Gain *= FadeCurveCalculate(t, vEvent.FadeOut.Curve, true);
                }
                grp.Add(newEvent);
            }
            vEvent.Mute = args.Mute;

            for (int i = 0; i < args.Count; i++)
            {
                VideoTrack trk = (VideoTrack)project.Tracks[vTrack.Index + i + 1];
                trk.Selected = true;
                trk.CompositeNestingLevel += vTrack.CompositeNestingLevel;
            }

            if (vTrack.IsCompositingParent)
            {
                vTrack.SetParentCompositeMode(vTrack.CompositeMode, false);

                if (args.TransferToParentMotion)
                {
                    project.TransferTrackMotion(vTrack, vTrack, 1);
                }
            }

#if !Sony
            if (Common.VegasVersion > 18)
            {
                if (args.TransferToParentFx)
                {
                    foreach (Effect ef in vTrack.Effects)
                    {
                        ef.ApplyAfterComposite(true);
                    }
                }
            }
#endif
        }

        // type 0: Child to Child; type 1: Child to Parent; type 2: Parent to Child; type 3: Parent to Parent
        public static void TransferTrackMotion(this Project project, VideoTrack sourceTrack, VideoTrack targetTrack, int type = 0)
        {
            bool is3D = sourceTrack.CompositeMode == CompositeMode.SrcAlpha3D;
            TrackMotion sourceMotion = sourceTrack.TrackMotion, targetMotion = targetTrack.TrackMotion, tmpMotion;

            switch (type)
            {
                case 1: sourceMotion = sourceTrack.TrackMotion; targetMotion = targetTrack.ParentTrackMotion; break;
                case 2: sourceMotion = sourceTrack.ParentTrackMotion; targetMotion = targetTrack.TrackMotion; break;
                case 3: sourceMotion = sourceTrack.ParentTrackMotion; targetMotion = targetTrack.ParentTrackMotion; break;
                default: break;
            }

            bool motion = sourceMotion.HasMotionData && !targetMotion.HasMotionData;
            bool shadow = !is3D && sourceMotion.ShadowEnabled && sourceMotion.HasShadowData && !targetMotion.HasShadowData;
            bool glow = !is3D && sourceMotion.GlowEnabled && sourceMotion.HasGlowData && !targetMotion.HasGlowData;

            if (motion || shadow || glow)
            {
                VideoTrack tmpTrack1 = new VideoTrack(project, 0, null);
                project.Tracks.Add(tmpTrack1);
                VideoTrack tmpTrack0 = new VideoTrack(project, 0, null);
                project.Tracks.Add(tmpTrack0);
                tmpTrack1.CompositeNestingLevel = tmpTrack0.CompositeNestingLevel + 1;
                tmpTrack0.SetCompositeMode(CompositeMode.SrcAlpha3D, false);
                tmpTrack0.SetParentCompositeMode(CompositeMode.SrcAlpha3D, false);
                tmpMotion = type == 0 || type == 1 ? tmpTrack0.TrackMotion : tmpTrack0.ParentTrackMotion;

                if (motion)
                {
                    foreach (TrackMotionKeyframe k in sourceMotion.MotionKeyframes)
                    {
                        project.CopyTrackKeyframe(k, k.Index == 0 ? targetMotion.MotionKeyframes[0] : targetMotion.InsertMotionKeyframe(k.Position), type, is3D);
                    }
                    sourceMotion.MotionKeyframes.Clear();
                    project.CopyTrackKeyframe(tmpMotion.MotionKeyframes[0], sourceMotion.MotionKeyframes[0], 0, is3D);
                }

                if (shadow)
                {
                    foreach (TrackShadowKeyframe k in sourceMotion.ShadowKeyframes)
                    {
                        project.CopyTrackKeyframe(k, k.Index == 0 ? targetMotion.ShadowKeyframes[0] : targetMotion.InsertShadowKeyframe(k.Position), type);
                    }
                    sourceMotion.ShadowKeyframes.Clear();
                    project.CopyTrackKeyframe(tmpMotion.ShadowKeyframes[0], sourceMotion.ShadowKeyframes[0]);
                }

                if (glow)
                {
                    foreach (TrackGlowKeyframe k in sourceMotion.GlowKeyframes)
                    {
                        project.CopyTrackKeyframe(k, k.Index == 0 ? targetMotion.GlowKeyframes[0] : targetMotion.InsertGlowKeyframe(k.Position), type);
                    }
                    sourceMotion.GlowKeyframes.Clear();
                    project.CopyTrackKeyframe(tmpMotion.GlowKeyframes[0], sourceMotion.GlowKeyframes[0]);
                }

                project.Tracks.Remove(tmpTrack1);
                project.Tracks.Remove(tmpTrack0);
            }
        }

        public static bool PopUpWindow(this Vegas myVegas, out LayerRepeaterArgs args)
        {
            LayerRepeaterArgs args0 = new LayerRepeaterArgs(true);
            
            Color[] colors = myVegas.GetColors();
            Form form = new Form
            {
                ShowInTaskbar = false,
                AutoSize = true,
                BackColor = colors[0],
                ForeColor = colors[1],
                Font = new Font(L.Font, 9),
                Text = string.Format("{0} {1}", L.LayerRepeater, VERSION),
                FormBorderStyle = FormBorderStyle.FixedToolWindow,
                StartPosition = FormStartPosition.CenterScreen,
                AutoSizeMode = AutoSizeMode.GrowAndShrink
            };

            Panel p = new Panel
            {
                AutoSize = true,
                AutoSizeMode = AutoSizeMode.GrowAndShrink
            };
            form.Controls.Add(p);

            TableLayoutPanel l = new TableLayoutPanel
            {
                AutoSize = true,
                AutoSizeMode = AutoSizeMode.GrowAndShrink,
                GrowStyle = TableLayoutPanelGrowStyle.AddRows,
                ColumnCount = 2
            };
            l.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 90));
            l.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 120));
            p.Controls.Add(l);

            Label label = new Label
            {
                Margin = new Padding(6, 10, 0, 3),
                Text = L.Mode,
                AutoSize = true
            };
            l.Controls.Add(label);

            Button modeTypeButton = new Button
            {
                Margin = new Padding(6, 6, 10, 3),
                Tag = args0.Mode,
                Text = L.ModeType[args0.Mode],
                TextAlign = ContentAlignment.MiddleLeft,
                AutoSize = true,
                FlatStyle = FlatStyle.Flat,
                Dock = DockStyle.Fill
            };
            modeTypeButton.FlatAppearance.BorderSize = 0;
            l.Controls.Add(modeTypeButton);

            Label labelCount = new Label
            {
                Margin = new Padding(6, 10, 0, 6),
                Text = L.Count,
                AutoSize = true
            };
            l.Controls.Add(labelCount);

            TextBox countBox = new TextBox
            {
                AutoSize = true,
                Margin = new Padding(9, 6, 0, 6),
                Text = args0.Count.ToString()
            };
            l.Controls.Add(countBox);

            Button speedTypeButton = new Button
            {
                Margin = new Padding(3, 8, 0, 1),
                Tag = args0.SpeedType,
                Text = L.SpeedType[args0.SpeedType],
                TextAlign = ContentAlignment.MiddleLeft,
                AutoSize = false,
                FlatStyle = FlatStyle.Flat
            };
            speedTypeButton.FlatAppearance.BorderSize = 0;
            l.Controls.Add(speedTypeButton);

            TableLayoutPanel sp = new TableLayoutPanel
            {
                Dock = DockStyle.Fill,
                AutoSize = true,
                Anchor = (AnchorStyles.Top | AnchorStyles.Left),
                GrowStyle = TableLayoutPanelGrowStyle.AddRows,
                ColumnCount = 3
            };
            sp.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 50));
            sp.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 20));
            sp.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 30));
            l.Controls.Add(sp);

            TextBox speedBox = new TextBox
            {
                AutoSize = true,
                Margin = new Padding(6, 3, 0, 0),
                Text = args0.Speed.ToString()
            };

            sp.Controls.Add(speedBox);

            speedTypeButton.MouseDown += delegate (object o, MouseEventArgs e)
            {
                if (e.Button == MouseButtons.Right && (int)speedTypeButton.Tag == 1)
                {
                    speedBox.Text = myVegas.Project.Ruler.BeatsPerMinute.ToString();
                }
                else
                {
                    speedTypeButton.ClickToSwitch(L.SpeedType);
                    if (double.TryParse(speedBox.Text, out double tmp))
                    {
                        speedBox.Text = ((int)speedTypeButton.Tag == 0 ? (tmp / 60) : (tmp * 60)).ToString();
                    }
                }
            };

            string[] operatorStrs = new string[] { "*", "/" };
            Button operatorButton = new Button
            {
                Margin = new Padding(3, 6, 0, 4),
                Tag = args0.SpeedOperatorType,
                Text = operatorStrs[args0.SpeedOperatorType],
                TextAlign = ContentAlignment.MiddleLeft,
                AutoSize = false,
                FlatStyle = FlatStyle.Flat
            };
            operatorButton.FlatAppearance.BorderSize = 0;
            operatorButton.MouseDown += delegate (object o, MouseEventArgs e)
            {
                operatorButton.ClickToSwitch(operatorStrs);
            };
            sp.Controls.Add(operatorButton);

            TextBox multiplierBox = new TextBox
            {
                AutoSize = true,
                Margin = new Padding(6, 3, 8, 0),
                Text = args0.SpeedMultiplier.ToString()
            };
            sp.Controls.Add(multiplierBox);

            bool isCount = (int)modeTypeButton.Tag == 0;
            labelCount.Visible = isCount;
            countBox.Visible = isCount;
            speedTypeButton.Visible = !isCount;
            sp.Visible = !isCount;
            speedBox.Visible = !isCount;
            operatorButton.Visible = !isCount;
            multiplierBox.Visible = !isCount;

            modeTypeButton.MouseDown += delegate (object o, MouseEventArgs e) 
            {
                ClickToSwitch(modeTypeButton, L.ModeType);
                isCount = (int)modeTypeButton.Tag == 0;
                form.SuspendLayout();
                labelCount.Visible = isCount;
                countBox.Visible = isCount;
                speedTypeButton.Visible = !isCount;
                sp.Visible = !isCount;
                speedBox.Visible = !isCount;
                operatorButton.Visible = !isCount;
                multiplierBox.Visible = !isCount;
                form.ResumeLayout();
            };

            label = new Label
            {
                Margin = new Padding(6, 9, 0, 3),
                Text = L.Range,
                AutoSize = true
            };
            l.Controls.Add(label);

            ComboBox rangeTypeBox = new ComboBox
            {
                AutoSize = true,
                Margin = new Padding(9, 6, 11, 6),
                DataSource = L.RangeType,
                DropDownStyle = ComboBoxStyle.DropDownList,
                Dock = DockStyle.Fill
            };
            l.Controls.Add(rangeTypeBox);

            int rangeTypeIndex = args0.RangeType;
            form.Load += delegate (object o, EventArgs ea)
            {
                rangeTypeBox.SelectedIndex = rangeTypeIndex;
            };

            CheckBox reverseBox = new CheckBox
            {
                Text = L.Reverse,
                Margin = new Padding(6, 3, 0, 3),
                AutoSize = true,
                Checked = args0.Reverse
            };

            l.Controls.Add(reverseBox);

            CheckBox muteBox = new CheckBox
            {
                Text = L.Mute,
                Margin = new Padding(6, 3, 0, 3),
                AutoSize = true,
                Checked = args0.Mute
            };

            l.Controls.Add(muteBox);

            GroupBox transferToParentBox = new GroupBox
            {
                AutoSize = true,
                AutoSizeMode = AutoSizeMode.GrowAndShrink,
                Dock = DockStyle.Fill,
                Text = L.TransferToParent,
                ForeColor = colors[1]
            };

            l.Controls.Add(transferToParentBox);
            l.SetColumnSpan(transferToParentBox, 2);

            TableLayoutPanel lp = new TableLayoutPanel
            {
                Dock = DockStyle.Fill,
                AutoSize = true,
                Anchor = (AnchorStyles.Top | AnchorStyles.Left),
                GrowStyle = TableLayoutPanelGrowStyle.AddRows,
                ColumnCount = 2
            };
            lp.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 90));
            lp.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 120));
            transferToParentBox.Controls.Add(lp);

            CheckBox parentMotionBox = new CheckBox
            {
                Text = L.TrackMotion,
                Margin = new Padding(0, 3, 0, 3),
                AutoSize = true,
                Checked = args0.TransferToParentMotion
            };

            lp.Controls.Add(parentMotionBox);

            CheckBox parentFxBox = new CheckBox
            {
                Text = L.TrackFx,
                Margin = new Padding(0, 3, 0, 3),
                AutoSize = true,
                Checked = args0.TransferToParentFx,
                Visible = Common.VegasVersion > 18
            };

            lp.Controls.Add(parentFxBox);

            FlowLayoutPanel panel = new FlowLayoutPanel
            {
                AutoSize = true,
                AutoSizeMode = AutoSizeMode.GrowAndShrink,
                Anchor = AnchorStyles.None,
                Font = new Font(L.Font, 8)
            };
            l.Controls.Add(panel);
            l.SetColumnSpan(panel, 2);

            Button ok = new Button
            {
                Text = L.OK,
                DialogResult = DialogResult.OK
            };
            panel.Controls.Add(ok);
            form.AcceptButton = ok;

            Button cancel = new Button
            {
                Text = L.Cancel,
                DialogResult = DialogResult.Cancel
            };
            panel.Controls.Add(cancel);
            form.CancelButton = cancel;

            Button settings = new Button
            {
                Text = L.Settings
            };
            settings.Click += new EventHandler(myVegas.Settings_Click);
            panel.Controls.Add(settings);

            Button clear = new Button
            {
                Text = L.Clear,
                DialogResult = DialogResult.OK
            };
            clear.Click += delegate (object o, EventArgs e)
            {
                modeTypeButton.Tag = 0;
                countBox.Text = "0";
            };
            panel.Controls.Add(clear);

            DialogResult result = form.ShowDialog();
            int count = int.TryParse(countBox.Text, out int temp) && temp > 1 ? temp : 0;
            double.TryParse(speedBox.Text, out double speed);
            double.TryParse(multiplierBox.Text, out double multiplier);
            args = new LayerRepeaterArgs((int)modeTypeButton.Tag, count, rangeTypeBox.SelectedIndex, reverseBox.Checked, muteBox.Checked, parentMotionBox.Checked, parentFxBox.Checked);
            if (args.Mode == 1)
            {
                args.SetSpeedParameters(speed, (int)speedTypeButton.Tag, (int)operatorButton.Tag, multiplier);
            }
            args.GetSettingsFromIni();
            args.SaveToIni();
            return result == DialogResult.OK;
        }

        public static void Settings_Click(this Vegas myVegas, object sender, EventArgs e)
        {
            Color[] colors = myVegas.GetColors();
            Form form = new Form
            {
                ShowInTaskbar = false,
                AutoSize = true,
                BackColor = colors[0],
                ForeColor = colors[1],
                Font = new Font(L.Font, 9),
                Text = L.Settings,
                FormBorderStyle = FormBorderStyle.FixedToolWindow,
                StartPosition = FormStartPosition.CenterScreen,
                AutoSizeMode = AutoSizeMode.GrowAndShrink
            };

            Panel p = new Panel
            {
                AutoSize = true,
                AutoSizeMode = AutoSizeMode.GrowAndShrink
            };
            form.Controls.Add(p);

            TableLayoutPanel l = new TableLayoutPanel
            {
                AutoSize = true,
                AutoSizeMode = AutoSizeMode.GrowAndShrink,
                GrowStyle = TableLayoutPanelGrowStyle.AddRows,
                ColumnCount = 2
            };
            l.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 100));
            l.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 120));
            p.Controls.Add(l);

            Label label = new Label
            {
                Margin = new Padding(6, 10, 0, 6),
                Text = L.Language,
                AutoSize = true
            };
            l.Controls.Add(label);

            ComboBox languageBox = new ComboBox
            {
                DataSource = new string[] { "English", "中文" },
                DropDownStyle = ComboBoxStyle.DropDownList,
                Dock = DockStyle.Fill
            };
            l.Controls.Add(languageBox);

            int languageIndex = L.CurrentLanguage == "zh" ? 1 : 0;

            label = new Label
            {
                Margin = new Padding(6, 10, 0, 6),
                Text = L.DarkMode,
                AutoSize = true
            };
            l.Controls.Add(label);

            ComboBox darkModeBox = new ComboBox
            {
                DataSource = L.DarkModeType,
                DropDownStyle = ComboBoxStyle.DropDownList,
                Dock = DockStyle.Fill
            };
            l.Controls.Add(darkModeBox);
            int darkModeType = Common.IniMiscz.ReadInt("DarkModeType", "MisczTools", 0);

            label = new Label
            {
                Margin = new Padding(6, 10, 0, 6),
                Text = L.FxSplitEnable,
                AutoSize = true
            };
            l.Controls.Add(label);

            ComboBox fxSplitBox = new ComboBox
            {
                DataSource = L.FxSplitEnableType,
                DropDownStyle = ComboBoxStyle.DropDownList,
                Dock = DockStyle.Fill
            };
            l.Controls.Add(fxSplitBox);
            int fxSplitType = Common.IniMiscz.ReadInt("FxSplitType", "LayerRepeater", 0);

            form.Load += delegate (object o, EventArgs ea)
            {
                languageBox.SelectedIndex = languageIndex;
                darkModeBox.SelectedIndex = darkModeType;
                fxSplitBox.SelectedIndex = fxSplitType;
            };

            bool rangeAdapt = Common.IniMiscz.ReadBool("RangeAdapt", "LayerRepeater", true);
            CheckBox rangeAdaptBox = new CheckBox
            {
                Text = L.RangeAdapt,
                Margin = new Padding(6, 3, 0, 3),
                AutoSize = true,
                Checked = rangeAdapt
            };

            l.Controls.Add(rangeAdaptBox);
            l.SetColumnSpan(rangeAdaptBox, 2);

            FlowLayoutPanel panel = new FlowLayoutPanel
            {
                AutoSize = true,
                AutoSizeMode = AutoSizeMode.GrowAndShrink,
                Anchor = AnchorStyles.None,
                Font = new Font(L.Font, 8)
            };
            l.Controls.Add(panel);
            l.SetColumnSpan(panel, 2);

            Button ok = new Button
            {
                Text = L.OK,
                DialogResult = DialogResult.OK
            };
            panel.Controls.Add(ok);
            form.AcceptButton = ok;
            if (form.ShowDialog() == DialogResult.OK)
            {
                if (languageBox.SelectedIndex != languageIndex)
                {
                    L.CurrentLanguage = languageBox.SelectedIndex == 1 ? "zh" : "en";
                    Common.IniMiscz.Write("Language", L.CurrentLanguage, "MisczTools");
                    L.Localize();
                }

                if (darkModeBox.SelectedIndex != darkModeType)
                {
                    Common.IniMiscz.Write("DarkModeType", darkModeBox.SelectedIndex.ToString(), "MisczTools");
                }

                if (languageBox.SelectedIndex != languageIndex || darkModeBox.SelectedIndex != darkModeType)
                {
                    MessageBox.Show(L.UIChange);
                }

                if (fxSplitBox.SelectedIndex != fxSplitType)
                {
                    Common.IniMiscz.Write("FxSplitType", fxSplitBox.SelectedIndex.ToString(), "LayerRepeater");
                }

                if (rangeAdaptBox.Checked != rangeAdapt)
                {
                    Common.IniMiscz.Write("RangeAdapt", rangeAdaptBox.Checked ? "1" : "0", "LayerRepeater");
                }
            }
        }

        public static void ClickToSwitch(this Button b, string[] strs)
        {
            
            if (strs.Length > 0)
            {
                int i = (int)b.Tag + 1;
                i = i < 0 ? 0 : i < strs.Length ? i : (i - i / strs.Length * strs.Length);
                b.Tag = i;
                b.Text = strs[i];
            }
        }
    }
}