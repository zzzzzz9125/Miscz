#define Sony

using System;
using System.Drawing;
using System.Windows.Forms;
using System.Collections.Generic;

#if MAGIX
using ScriptPortal.Vegas;
#else
using Sony.Vegas;
#endif

namespace LayerRepeater
{
    public class LayerRepeaterClass
    {
        public Vegas myVegas;
        public const string VERSION = "v1.01";
        public static int VegasVersion = System.Diagnostics.FileVersionInfo.GetVersionInfo(Application.ExecutablePath).FileMajorPart;
        public static bool TransferToParentFXEnabled = VegasVersion > 18;

        public void Main(Vegas vegas)
        {
            myVegas = vegas;
            myVegas.ResumePlaybackOnScriptExit = true;
            myVegas.UnloadScriptDomainOnScriptExit = true;
            Project project = myVegas.Project;
            int count = 0;
            bool mute = true;
            L.Localize();

            List<VideoEvent> vEvents = project.GetSelectedVideoEvents(false);
            if (vEvents.Count < 1 || (vEvents.Count == 1 && vEvents[0].VideoMotion.Keyframes.Count < 2 && !Common.CtrlMode))
            {
                MessageBox.Show(L.TooFewKeyframes);
                return;
            }

            if (!Common.CtrlMode)
            {
                DialogResult result = PopUpWindow(out count, out mute);
                if (result != DialogResult.OK)
                {
                    return;
                }
            }

            foreach (VideoEvent vEvent in vEvents)
            {
                vEvent.Mute = false;

                // to avoid problems later when creating track groups
                foreach (Track myTrack in project.Tracks)
                {
                    myTrack.Selected = false;
                }

                VideoTrack vTrack = (VideoTrack)vEvent.Track;
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

                if (count == 0)
                {
                    continue;
                }
                Timecode start = vEvent.VideoMotion.Keyframes[0].Position, end = vEvent.VideoMotion.Keyframes[vEvent.VideoMotion.Keyframes.Count-1].Position;

                for (int i = count; i > 0; i--)
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
                    
                    if (i == 1 && i != count)
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

                    VideoEvent newEvent = (VideoEvent) vEvent.Copy(newTrack, vEvent.Start);
                    VideoMotionKeyframes kfs = newEvent.VideoMotion.Keyframes;
                    Timecode pos = new Timecode((end - start).ToMilliseconds() * (count - i) / (count - 1)) + start;

                    if (i != count)
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
                vEvent.Mute = mute;
                
                for (int i = 0; i < count; i++)
                {
                    VideoTrack trk = (VideoTrack)project.Tracks[vTrack.Index + i + 1];
                    trk.Selected = true;
                    trk.CompositeNestingLevel += vTrack.CompositeNestingLevel;
                }

                if (vTrack.IsCompositingParent)
                {
                    vTrack.SetParentCompositeMode(vTrack.CompositeMode, false);
                }

#if MAGIX
                if (TransferToParentFXEnabled)
                {
                    string parentFXStr = Common.IniMiscz.Read("TransferToParentFX", "LayerRepeater");
                    if (string.IsNullOrEmpty(parentFXStr) || parentFXStr == "1")
                    {
                        foreach (Effect ef in vTrack.Effects)
                        {
                            ef.ApplyAfterComposite(true);
                        }
                    }
                }
#endif
            }
        }

        static float FadeCurveCalculate(double t, CurveType type, bool fadeOut = false)
        {
            t = (fadeOut ? type == CurveType.Fast : type == CurveType.Slow) ? Math.Pow(t, 2)
              : (fadeOut ? type == CurveType.Slow : type == CurveType.Fast) ? t * (2 - t)
              : type == CurveType.Smooth ? Math.Pow(t, 2) * (3 - t * 2)
              : type == CurveType.Sharp ? (Math.Pow(t - 0.5, 3) * 4 + 0.5) : t;
            return (float)t;
        }

        DialogResult PopUpWindow(out int count, out bool mute)
        {
            count = int.TryParse(Common.IniMiscz.Read("Count", "LayerRepeater"), out int tmp) ? tmp : 50;
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
            l.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 100));
            l.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 100));
            p.Controls.Add(l);

            Label label = new Label
            {
                Margin = new Padding(6, 10, 0, 6),
                Text = L.Count,
                AutoSize = true
            };
            l.Controls.Add(label);

            TextBox countBox = new TextBox
            {
                AutoSize = true,
                Margin = new Padding(9, 6, 11, 6),
                Text = count.ToString()
            };
            l.Controls.Add(countBox);

            string muteStr = Common.IniMiscz.Read("Mute", "LayerRepeater");
            CheckBox muteBox = new CheckBox
            {
                Text = L.Mute,
                Margin = new Padding(0, 3, 0, 3),
                AutoSize = true,
                Checked = string.IsNullOrEmpty(muteStr) || muteStr == "1"
            };

            l.Controls.Add(muteBox);
            l.SetColumnSpan(muteBox, 2);

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
            settings.Click += new EventHandler(Settings_Click);
            panel.Controls.Add(settings);

            Button clear = new Button
            {
                Text = L.Clear,
                DialogResult = DialogResult.OK
            };
            clear.Click += delegate (object o, EventArgs e)
            {
                countBox.Text = "0";
            };
            panel.Controls.Add(clear);

            DialogResult result = form.ShowDialog();
            if (int.TryParse(countBox.Text, out count) && count > 1)
            {
                Common.IniMiscz.Write("Count", count.ToString(), "LayerRepeater");
            }
            else
            {
                count = 0;
            }
            Common.IniMiscz.Write("Mute", muteBox.Checked ? "1" : "0", "LayerRepeater");
            mute = muteBox.Checked;
            return result;
        }

        void Settings_Click(object sender, EventArgs e)
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
            l.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 100));
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
            int.TryParse(Common.IniMiscz.Read("DarkModeType", "MisczTools"), out int darkIndex);

            form.Load += delegate (object o, EventArgs ea)
            {
                languageBox.SelectedIndex = languageIndex;
                darkModeBox.SelectedIndex = darkIndex;
            };

            string parentFXStr = Common.IniMiscz.Read("TransferToParentFX", "LayerRepeater");
            bool parentFXCheck = string.IsNullOrEmpty(parentFXStr) || parentFXStr == "1";
            CheckBox parentFX = new CheckBox
            {
                Text = L.TransferToParentFX,
                Margin = new Padding(0, 3, 0, 3),
                AutoSize = true,
                Checked = parentFXCheck
            };

            if (TransferToParentFXEnabled)
            {
                l.Controls.Add(parentFX);
                l.SetColumnSpan(parentFX, 2);
            }

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

                if (darkModeBox.SelectedIndex != darkIndex)
                {
                    Common.IniMiscz.Write("DarkModeType", darkModeBox.SelectedIndex.ToString(), "MisczTools");
                }

                if (languageBox.SelectedIndex != languageIndex || darkModeBox.SelectedIndex != darkIndex)
                {
                    MessageBox.Show(L.UIChange);
                }

                if (TransferToParentFXEnabled && (parentFX.Checked != parentFXCheck))
                {
                    Common.IniMiscz.Write("TransferToParentFX", parentFX.Checked ? "1" : "0", "LayerRepeater");
                }
            }
        }
    }
}

public class EntryPoint
{
    public void FromVegas(Vegas vegas)
    {
        LayerRepeater.LayerRepeaterClass test = new LayerRepeater.LayerRepeaterClass();
        test.Main(vegas);
    }
}