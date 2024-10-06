using System;
using System.IO;
using System.Text;
using System.Drawing;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Windows.Forms;
using System.Collections.Generic;

using MyINI;

using ScriptPortal.Vegas;  // "ScriptPortal.Vegas" for Magix Vegas Pro 14 or above, "Sony.Vegas" for Sony Vegas Pro 13 or below

namespace LayerRepeater
{
    public class LayerRepeaterClass
    {
        public Vegas myVegas;
        public const int COUNT_DEFAULT = 50;
        public const string VERSION = "v1.0";
        public static int VegasVersion = System.Diagnostics.FileVersionInfo.GetVersionInfo(Application.ExecutablePath).FileMajorPart;
        public static string IniFolder = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData), "VEGAS Pro", VegasVersion + ".0");
        public static IniFile IniMiscz = new IniFile(Path.Combine(IniFolder, "MisczTools.ini"));

        public void Main(Vegas vegas)
        {
            myVegas = vegas;
            myVegas.ResumePlaybackOnScriptExit = true;
            Project project = myVegas.Project;
            bool ctrlMode = (Control.ModifierKeys & Keys.Control) != 0;
            int count = 0;
            L.Localize();

            List<VideoEvent> vEvents = GetSelectedVideoEvents(project, false);
            if (vEvents.Count < 1 || (vEvents.Count == 1 && vEvents[0].VideoMotion.Keyframes.Count < 2 && !ctrlMode))
            {
                MessageBox.Show(L.TooFewKeyframes);
                return;
            }

            if (!ctrlMode)
            {
                DialogResult result = PopUpWindow(out count);
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
                vEvent.Mute = true;
                
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

                string parentFXStr = IniMiscz.Read("TransferToParentFX", "LayerRepeater");
                if (string.IsNullOrEmpty(parentFXStr) ? true : parentFXStr == "1")
                {
                    foreach (Effect ef in vTrack.Effects)
                    {
                        ef.ApplyAfterComposite = true;
                    }
                }
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

        static DialogResult PopUpWindow(out int count)
        {
            int tmp = 0;
            count = int.TryParse(IniMiscz.Read("Count", "LayerRepeater"), out tmp) ? tmp : COUNT_DEFAULT;
            Form form = new Form
            {
                ShowInTaskbar = false,
                AutoSize = true,
                BackColor = Color.FromArgb(45, 45, 45),
                ForeColor = Color.FromArgb(200, 200, 200),
                Font = new Font(L.Font, 9),
                Text = string.Format("{0} {1}", L.LayerRepeater, VERSION),
                FormBorderStyle = FormBorderStyle.FixedToolWindow,
                StartPosition = FormStartPosition.CenterScreen,
                AutoSizeMode = AutoSizeMode.GrowAndShrink
            };
            if (VegasVersion < 15)
            {
                form.BackColor = Color.FromArgb(153,153,153);
                form.ForeColor = Color.FromArgb(0,0,0);
            }

            Panel p = new Panel();
            p.AutoSize = true;
            p.AutoSizeMode = AutoSizeMode.GrowAndShrink;
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
                IniMiscz.Write("Count", count.ToString(), "LayerRepeater");
            }
            else
            {
                count = 0;
            }
            return result;
        }

        static void Settings_Click(object sender, EventArgs e)
        {
            Form form = new Form
            {
                ShowInTaskbar = false,
                AutoSize = true,
                BackColor = Color.FromArgb(45, 45, 45),
                ForeColor = Color.FromArgb(200, 200, 200),
                Font = new Font(L.Font, 9),
                Text = L.Settings,
                FormBorderStyle = FormBorderStyle.FixedToolWindow,
                StartPosition = FormStartPosition.CenterScreen,
                AutoSizeMode = AutoSizeMode.GrowAndShrink
            };
            if (VegasVersion < 15)
            {
                form.BackColor = Color.FromArgb(153,153,153);
                form.ForeColor = Color.FromArgb(0,0,0);
            }

            Panel p = new Panel();
            p.AutoSize = true;
            p.AutoSizeMode = AutoSizeMode.GrowAndShrink;
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

            form.Load += delegate (object o, EventArgs ea)
            {
                languageBox.SelectedIndex = languageIndex;
            };

            string parentFXStr = IniMiscz.Read("TransferToParentFX", "LayerRepeater");
            bool parentFXCheck = string.IsNullOrEmpty(parentFXStr) ? true : parentFXStr == "1";
            CheckBox parentFX = new CheckBox
            {
                Text = L.TransferToParentFX,
                Margin = new Padding(0, 3, 0, 3),
                AutoSize = true,
                Checked = parentFXCheck
            };
            l.Controls.Add(parentFX);
            l.SetColumnSpan(parentFX, 2);

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
                    IniMiscz.Write("Language", L.CurrentLanguage, "MisczTools");
                    L.Localize();
                    MessageBox.Show(L.LanguageChange);
                }

                if (parentFX.Checked != parentFXCheck)
                {
                    IniMiscz.Write("TransferToParentFX", parentFX.Checked ? "1" : "0", "LayerRepeater");
                }
            }
        }

        public static List<VideoEvent> GetSelectedVideoEvents(Project project, bool reverse = false)
        {
            List<VideoEvent> vEvents = new List<VideoEvent>();
            foreach (Track myTrack in project.Tracks)
            {
                if (myTrack.IsVideo())
                {
                    foreach (TrackEvent evnt in myTrack.Events)
                    {
                        if (evnt.Selected)
                        {
                            if (evnt.ActiveTake == null || evnt.ActiveTake.Media == null || evnt.ActiveTake.MediaStream == null)
                            {
                                continue;
                            }

                            vEvents.Add((VideoEvent)evnt);
                        }
                    }
                }
            }
            if (reverse)
            {
                vEvents.Reverse();
            }

            return vEvents;
        }
    }

    public static class L
    {
        public static string CurrentLanguage, Font, LayerRepeater, Count, ColorGradient, Settings, TransferToParentFX, Language, Clear, Cancel, OK, TooFewKeyframes, LanguageChange;

        // Some text localization.
        public static void Localize()
        {
            if (string.IsNullOrEmpty(CurrentLanguage))
            {
                string tmp =  LayerRepeaterClass.IniMiscz.Read("Language", "MisczTools");
                CurrentLanguage = string.IsNullOrEmpty(tmp) ? System.Globalization.CultureInfo.CurrentCulture.TwoLetterISOLanguageName : tmp;
            }
            
            switch (CurrentLanguage)
            {
                case "zh":
                    Font = "Microsoft Yahei UI";
                    LayerRepeater = "图层重复器"; Count = "图层数"; ColorGradient = "颜色渐变"; Settings = "设置"; TransferToParentFX = "转移到父轨 FX"; Language = "语言"; Clear = "清除"; Cancel = "取消"; OK = "确定";
                    TooFewKeyframes = "无法生成图层！请选择「至少含有 2 个平移/裁切关键帧」的事件！";
                    LanguageChange = "界面语言更改在重启脚本后才会生效！";
                    break;

                default:
                    Font = "Arial";
                    LayerRepeater = "LayerRepeater"; Count = "Count"; ColorGradient = "Color Gradient"; Settings = "Settings"; TransferToParentFX = "Transfer To Parent FX"; Language = "Language"; Clear = "Clear"; Cancel = "Cancel"; OK = "OK";
                    TooFewKeyframes = "Failed to generate layers! Please select an event with more than 1 Pan/Crop keyframes!";
                    LanguageChange = "UI language changes will not take effect until the script is restarted!";
                    break;
            }
        }
    }
}

namespace MyINI
{
    public class IniFile
    {
        string Path;
        string EXE = Assembly.GetExecutingAssembly().GetName().Name;
        [DllImport("kernel32", CharSet = CharSet.Unicode)]
        static extern long WritePrivateProfileString(string Section, string Key, string Value, string FilePath);
        [DllImport("kernel32", CharSet = CharSet.Unicode)]
        static extern int GetPrivateProfileString(string Section, string Key, string Default, StringBuilder RetVal, int Size, string FilePath);
        public IniFile(string IniPath = null)
        {
            Path = new FileInfo(IniPath ?? EXE + ".ini").FullName;
        }
        public string Read(string Key, string Section = null)
        {
            var RetVal = new StringBuilder(255);
            GetPrivateProfileString(Section ?? EXE, Key, "", RetVal, 255, Path);
            return RetVal.ToString();
        }
        public void Write(string Key, string Value, string Section = null)
        {
            WritePrivateProfileString(Section ?? EXE, Key, Value, Path);
        }
        public void DeleteKey(string Key, string Section = null)
        {
            Write(Key, null, Section ?? EXE);
        }
        public void DeleteSection(string Section = null)
        {
            Write(null, null, Section ?? EXE);
        }
        public bool KeyExists(string Key, string Section = null)
        {
            return Read(Key, Section).Length > 0;
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