using System;
using System.IO;
using System.Text;
using System.Drawing;
using System.Diagnostics;
using System.Collections;
using System.Windows.Forms;
using System.Text.RegularExpressions;

using ScriptPortal.Vegas;  // "ScriptPortal.Vegas" for Magix Vegas Pro 14 or above, "Sony.Vegas" for Sony Vegas Pro 13 or below

namespace Test_Script
{
    public class Class
    {
        public const string VERSION = "v.1.2.7";
        public Vegas myVegas;
        TextBox scaleBox;
        TrackBar scaleBar;
        ComboBox algorithmBox;
        string[] algorithmsList;
        public void Main(Vegas vegas)
        {
            myVegas = vegas;
            myVegas.ResumePlaybackOnScriptExit = true;
            Project project = myVegas.Project;
            bool ctrlMode = ((Control.ModifierKeys & Keys.Control) != 0) ? true : false, isRevise = false;
            double scaleFactor = 0;
            ArrayList mediaList = new ArrayList();
            string ffmpegPath = null;

            algorithmsList = new string[]{"neighbor", "bicubic", "fast_bilinear", "bilinear", "experimental", "area", "bicublin", "gauss", "sinc", "lanczos", "spline"};
            int indexAL = 0;

            foreach (string str in (Environment.GetEnvironmentVariable("PATH") ?? "").Split(';'))
            {
                if (!String.IsNullOrEmpty(str.Trim()) && File.Exists(Path.Combine(str.Trim(), "ffmpeg.exe")))
                {
                    ffmpegPath = Path.Combine(str.Trim(), "ffmpeg.exe");
                    break;
                }
            }

            if (!File.Exists(ffmpegPath))
            {
                if (DialogResult.Yes == MessageBox.Show("Detected that ffmpeg.exe is not added to Environment Variables!\nNote: If you just added it, please restart Vegas first, then retry.\n\nDo you want to continue anyway?", "ffmpeg.exe Not Found!", MessageBoxButtons.YesNo))
                {
                    ffmpegPath = "ffmpeg.exe";
                }

                else
                {
                    return;
                }
            }

            foreach (Track myTrack in myVegas.Project.Tracks)
            {
                if (myTrack.IsVideo())
                {
                    foreach (TrackEvent evnt in myTrack.Events)
                    {
                        if (evnt.Selected)
                        {
                            VideoEvent vEvent = (VideoEvent) evnt;
                            VideoStream vStream = (VideoStream) vEvent.ActiveTake.MediaStream;
                            string filePath = vEvent.ActiveTake.Media.FilePath;
                            string oriPath = Regex.Replace(filePath, @"(?<=_Scaled\.[A-Za-z0-9]+)(\.[A-Za-z0-9]+)*$", "");  // Delete ".yyy(.zzz)" from "AAA_Scaled.xxx.yyy(.zzz)"
                            oriPath = Regex.Replace(oriPath, @"_Scaled(?=\.[A-Za-z0-9]+$)", "");                            // Delete "_Scaled" from "AAA_Scaled.xxx"

                            if (vEvent.ActiveTake.Media.IsImageSequence())
                            {
                                filePath = Regex.Match(filePath, string.Format(@"^(.+\{0}(?=\s-\s))", Path.GetExtension(filePath))).Value;
                                oriPath = Path.Combine(Regex.Match(Path.GetDirectoryName(filePath), @"^(.+(?=(_Scaled)$))").Value, Path.GetFileName(filePath));
                                if (!File.Exists(oriPath) && Path.GetExtension(filePath).ToLower() == ".png")
                                {
                                    oriPath = Path.ChangeExtension(oriPath, ".gif");
                                    if (!File.Exists(oriPath))
                                    {
                                        oriPath = Path.ChangeExtension(oriPath, ".psd");
                                    }
                                }
                            }

                            if (!File.Exists(oriPath))
                            {
                                oriPath = filePath;
                            }

                            if (File.Exists(oriPath))
                            {
                                isRevise = (oriPath != filePath) || isRevise;
                                Media arrMedia = vEvent.ActiveTake.Media.IsImageSequence() ? project.MediaPool.AddImageSequence(oriPath, GetFramesCount(vStream), vStream.FrameRate) : Media.CreateInstance(project, oriPath);
                                arrMedia.GetVideoStreamByIndex(0).AlphaChannel = VideoAlphaType.Straight;
                                if (!mediaList.Contains(arrMedia))
                                {
                                    mediaList.Add(arrMedia);
                                    vEvent.ActiveTake.Media.ReplaceWith(arrMedia);
                                }
                            }
                        }
                    }
                }
            }

            if (ctrlMode || isRevise)
            {
                if (DialogResult.OK == ScaleWindow())
                {
                    double.TryParse(scaleBox.Text, out scaleFactor);
                    indexAL = algorithmBox.SelectedIndex;
                }

                else
                {
                    return;
                }
            }

            Process p = new Process();
            p.StartInfo.FileName = "cmd.exe";
            p.StartInfo.UseShellExecute = false;
            p.StartInfo.RedirectStandardInput = true;
            p.StartInfo.RedirectStandardOutput = true;
            p.StartInfo.RedirectStandardError = false;
            p.StartInfo.CreateNoWindow = true;

            string logText = "";
            try
            {
                foreach (Media arrMedia in mediaList)
                {
                    VideoStream vStream = arrMedia.GetVideoStreamByIndex(0);
                    double scaleValue = Math.Ceiling(Math.Min((double)project.Video.Width / vStream.Width, (double)project.Video.Height / vStream.Height));
                    if (scaleValue <= 1)
                    {
                        continue;
                    }
                    scaleValue = scaleFactor >= 1 ? scaleFactor : scaleValue;

                    string filePath = arrMedia.FilePath;
                    string[] speFormat = SpecialFormat(Path.GetExtension(filePath), arrMedia.IsImageSequence() || arrMedia.Length.Nanos == 0);
                    string outputPath = Path.Combine(Path.GetDirectoryName(filePath), Path.GetFileNameWithoutExtension(filePath) + "_Scaled" + Path.GetExtension(filePath) + speFormat[1]);
                    string renderCommand = string.Format("\"{0}\" -y -loglevel 32 -i \"{1}\" -vf scale=iw*{2}:ih*{2} -sws_flags {3} {4} \"{5}\"", ffmpegPath, filePath, scaleValue, algorithmsList[indexAL], speFormat[0], outputPath);

                    if (arrMedia.IsImageSequence())
                    {
                        filePath = Regex.Match(filePath, string.Format(@"^(.+\{0}(?=\s-\s))", Path.GetExtension(filePath))).Value;

                        outputPath = Path.GetDirectoryName(filePath) + "_Scaled";
                        if (Directory.Exists(outputPath))
                        {
                            Directory.Delete(outputPath, true);
                        }
                        Directory.CreateDirectory(outputPath);

                        string tmpExt = string.IsNullOrEmpty(speFormat[1]) ? Path.GetExtension(filePath) : speFormat[1];
                        renderCommand = string.Format("cd /d \"{1}\" & (for %i in (*{6}) do (\"{0}\" -y -loglevel 32 -i \"%i\" -vf scale=iw*{2}:ih*{2} -sws_flags {3} {4} \"{5}\\%~ni{7}\" ))", ffmpegPath, Path.GetDirectoryName(filePath), scaleValue, algorithmsList[indexAL], speFormat[0], outputPath, Path.GetExtension(filePath), tmpExt); 
                        outputPath = Path.Combine(outputPath, Path.GetFileNameWithoutExtension(filePath) + tmpExt);
                    }

                    p.Start();
                    while (!string.IsNullOrEmpty(p.StandardOutput.ReadLine())) { }
                    p.StandardInput.WriteLine(string.Format("echo off & ({0}) 2>&1 & exit", renderCommand));
                    logText += string.Format("\r\nInput Command:\r\n{0}\r\n\r\nOutput Logs:\r\n{1}", p.StandardOutput.ReadLine(), Encoding.UTF8.GetString(Encoding.Default.GetBytes(p.StandardOutput.ReadToEnd())));
                    p.WaitForExit();

                    Media newMedia = arrMedia.IsImageSequence() ? project.MediaPool.AddImageSequence(outputPath, GetFramesCount(vStream), vStream.FrameRate) : Media.CreateInstance(project, outputPath);
                    vStream = newMedia.GetVideoStreamByIndex(0);
                    vStream.AlphaChannel = VideoAlphaType.Straight;
                    arrMedia.ReplaceWith(newMedia);

                    Media oriMedia = newMedia.IsImageSequence() ? project.MediaPool.AddImageSequence(filePath, GetFramesCount(vStream), vStream.FrameRate) : Media.CreateInstance(project, filePath);
                    oriMedia.GetVideoStreamByIndex(0).AlphaChannel = VideoAlphaType.Straight;
                }
            }

            catch (Exception ex)
            {
                if (DialogResult.Yes == MessageBox.Show(string.Format("FFmpeg Rendering Error! {0}\n\nDo you want to see the FFmpeg logs?", ex.Message), "Rendering Error!", MessageBoxButtons.YesNo))
                {
                    string logPath = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.DesktopDirectory), string.Format("ffmpeg-{0}.log", DateTime.Now.ToString("yyyyMMdd-HHmmss")));
                    LogFile myLogFile = new LogFile(myVegas, logPath);
                    myLogFile.AddLogEntry(string.Format("If anything goes wrong, please submit an issue to the Github page below with this log.\r\nGithub Page: https://github.com/zzzzzz9125/Miscz/issues\r\nThe log file has been saved to {0}.\r\n\r\nError Message: {2}\r\n{1}", logPath, logText, ex.Message));
                    myLogFile.Close();
                    myLogFile.ShowLogAsDialog("FFmpeg Logs");
                }
            }
        }

        public static int GetFramesCount(VideoStream vStream)
        {
            int count = (int)Timecode.FromNanos((long)(vStream.Length.Nanos * vStream.FrameRate / vStream.Length.FrameRate)).FrameCount;
            return count;
        }

        public static string[] SpecialFormat(string format, bool isImage = false)
        {
            string[] str = new string[]{"", ""};
            switch (format.ToLower())
            {
                case ".png":
                    str[0] = "-pix_fmt rgb32";
                    break;

                case ".psd":
                    str = new string[]{"-pix_fmt rgb32", ".png"};
                    break;

                case ".gif":
                    str = isImage ? new string[]{"-pix_fmt rgb32", ".png"} : new string[]{"-c:v prores_ks -profile:v 4444", ".mov"};
                    break;
            }
            return str;
        }

        DialogResult ScaleWindow()
        {
            Form form = new Form();
            form.SuspendLayout();
            form.ShowInTaskbar = false;
            form.AutoSize = true;
            form.BackColor = Color.FromArgb(45,45,45);
            form.ForeColor = Color.FromArgb(200,200,200);
            if (double.Parse(Regex.Split(myVegas.Version, " ")[1]) < 15)
            {
            form.BackColor = Color.FromArgb(153,153,153);
            form.ForeColor = Color.FromArgb(0,0,0);
            }
            form.Font = new Font("Arial", 9);
            form.Text = "PixelScalingTool" + VERSION;
            form.FormBorderStyle = FormBorderStyle.FixedToolWindow;
            form.StartPosition = FormStartPosition.CenterScreen;
            form.AutoSizeMode = AutoSizeMode.GrowAndShrink;

            Panel panelBig = new Panel();
            panelBig.AutoSize = true;
            panelBig.AutoSizeMode = AutoSizeMode.GrowAndShrink;
            form.Controls.Add(panelBig);

            TableLayoutPanel l = new TableLayoutPanel();
            l.AutoSize = true;
            l.AutoSizeMode = AutoSizeMode.GrowAndShrink;
            l.GrowStyle = TableLayoutPanelGrowStyle.AddRows;
            l.ColumnCount = 3;
            l.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 100));
            l.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 130));
            l.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 50));
            panelBig.Controls.Add(l);

            Label label = new Label();
            label.Margin = new Padding(3, 12, 3, 6);
            label.Text = "Scale Factor";
            label.AutoSize = true;
            l.Controls.Add(label);

            scaleBar = new TrackBar();
            scaleBar.AutoSize = false;
            scaleBar.Height = label.Height;
            scaleBar.Margin = new Padding(0, 12, 0, 6);
            scaleBar.Dock = DockStyle.Fill;
            scaleBar.Minimum = 0;
            scaleBar.Maximum = 100;
            scaleBar.LargeChange = 1;
            scaleBar.SmallChange = 1;
            scaleBar.TickStyle = TickStyle.None;
            scaleBar.Value = 0;
            l.Controls.Add(scaleBar);
            scaleBar.ValueChanged += new EventHandler(scaleBar_ValueChanged);

            scaleBox = new TextBox();
            scaleBox.AutoSize = true;
            scaleBox.Margin = new Padding(3, 12, 3, 6);
            scaleBox.Text = "Auto";
            l.Controls.Add(scaleBox);
            scaleBox.TextChanged += new EventHandler(scaleBox_TextChanged);

            label = new Label();
            label.Margin = new Padding(3, 6, 3, 6);
            label.Text = "Algorithm";
            label.AutoSize = true;
            l.Controls.Add(label);

            algorithmBox = new ComboBox();
            algorithmBox.DataSource = algorithmsList;
            algorithmBox.DropDownStyle = ComboBoxStyle.DropDownList;
            algorithmBox.Margin = new Padding(3, 3, 3, 6);
            algorithmBox.Dock = DockStyle.Fill;
            l.Controls.Add(algorithmBox);
            l.SetColumnSpan(algorithmBox, 2);

            FlowLayoutPanel panel = new FlowLayoutPanel();
            panel.FlowDirection = FlowDirection.RightToLeft;
            panel.AutoSize = true;
            panel.AutoSizeMode = AutoSizeMode.GrowAndShrink;
            panel.Anchor = AnchorStyles.None;
            l.Controls.Add(panel);
            l.SetColumnSpan(panel, 3);
            panel.Font = new Font("Arial", 8);

            Button cancel = new Button();
            cancel.Text = "Cancel";
            cancel.DialogResult = DialogResult.Cancel;
            panel.Controls.Add(cancel);
            form.CancelButton = cancel;

            Button ok = new Button();
            ok.Text = "OK";
            ok.DialogResult = DialogResult.OK;
            panel.Controls.Add(ok);
            form.AcceptButton = ok;

            form.ResumeLayout();

		    DialogResult result = form.ShowDialog(myVegas.MainWindow);
            return result;
        }

        private void scaleBox_TextChanged(object sender, EventArgs e)
        {
            double a = 0;
            if (double.TryParse(((TextBox)sender).Text, out a))
            {
                a = Math.Min(a, 10000);
                if (scaleBar.Maximum < a)
                {
                    scaleBar.Maximum = (int)Math.Ceiling(a);
                }
                scaleBar.Value = (int)Math.Floor(Math.Max(a, scaleBar.Minimum));
            }
        }

        private void scaleBar_ValueChanged(object sender, EventArgs e)
        {
            scaleBox.Text = ((TrackBar)sender).Value < 1 ? "Auto" : string.Format("{0}", ((TrackBar)sender).Value);
        }
    }
}

public class EntryPoint
{
    public void FromVegas(Vegas vegas)
    //public void FromVegas(Vegas vegas, String scriptFile, XmlDocument scriptSettings, ScriptArgs args)
    {
        Test_Script.Class test = new Test_Script.Class();
        test.Main(vegas);
        Application.Exit();
    }
}