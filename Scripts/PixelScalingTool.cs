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
        public const string VERSION = "v.1.2.5";
        public Vegas myVegas;
        TextBox scaleBox;
        TrackBar scaleBar;
        public void Main(Vegas vegas)
        {
            myVegas = vegas;
            myVegas.ResumePlaybackOnScriptExit = true;
            Project project = myVegas.Project;
            bool ctrlMode = ((Control.ModifierKeys & Keys.Control) != 0) ? true : false, isRevise = false;
            double scaleFactor = 0;
            ArrayList mediaList = new ArrayList();

            foreach (Track myTrack in myVegas.Project.Tracks)
            {
                if (myTrack.IsVideo())
                {
                    foreach (TrackEvent evnt in myTrack.Events)
                    {
                        if (evnt.Selected)
                        {
                            VideoEvent vEvent = (VideoEvent) evnt;
                            string filePath = vEvent.ActiveTake.Media.FilePath;
                            string oriPath = Path.Combine(Path.GetDirectoryName(filePath), Regex.Replace(Path.GetFileNameWithoutExtension(filePath), @"(_Scaled)$", "") + Path.GetExtension(filePath));

                            isRevise = (Regex.IsMatch(Path.GetFileNameWithoutExtension(filePath), @"(_Scaled)$") && File.Exists(oriPath)) || isRevise;
                            if (!File.Exists(oriPath))
                            {
                                oriPath = filePath;
                            }

                            if (vEvent.ActiveTake.Media.IsImageSequence())
                            {
                                oriPath = Path.Combine(Regex.Replace(Path.GetDirectoryName(filePath), @"(_Scaled)$", ""), Regex.Match(Path.GetFileName(filePath), string.Format(@"^(([^<>/\\\|:""\*\?]*)([0-9]+)\{0}(?=\s-\s))", Path.GetExtension(filePath))).Value);
                                isRevise = (Regex.IsMatch(Path.GetDirectoryName(filePath), @"(_Scaled)$") && File.Exists(oriPath)) || isRevise;
                                if (!File.Exists(oriPath))
                                {
                                    oriPath = Path.Combine(Path.GetDirectoryName(filePath), Regex.Match(Path.GetFileName(filePath), string.Format(@"^(([^<>/\\\|:""\*\?]*)([0-9]+)\{0}(?=\s-\s))", Path.GetExtension(filePath))).Value);
                                }
                            }

                            if (File.Exists(oriPath))
                            {
                                Media arrMedia = vEvent.ActiveTake.Media.IsImageSequence() ? project.MediaPool.AddImageSequence(oriPath, GetFramesCount(filePath), ((VideoStream)vEvent.ActiveTake.MediaStream).FrameRate) : Media.CreateInstance(project, oriPath);
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
                    double scaleValue = scaleFactor >= 1 ? scaleFactor : Math.Ceiling(Math.Max(1, Math.Min((double)project.Video.Width / vStream.Width, (double)project.Video.Height / vStream.Height)));

                    string filePath = arrMedia.FilePath;
                    string outputPath = Path.Combine(Path.GetDirectoryName(filePath), Path.GetFileNameWithoutExtension(filePath) + "_Scaled" + Path.GetExtension(filePath));
                    string renderCommand = string.Format("ffmpeg -y -loglevel 32 -i \"{0}\" -vf scale=iw*{1}:ih*{1} -sws_flags neighbor \"{2}\"", filePath, scaleValue, outputPath); 

                    if (arrMedia.IsImageSequence())
                    {
                        outputPath = Path.GetDirectoryName(filePath) + "_Scaled";

                        if (Directory.Exists(outputPath))
                        {
                            Directory.Delete(outputPath, true);
                        }

                        Directory.CreateDirectory(outputPath);
                        renderCommand = string.Format("cd /d \"{0}\" & (for %i in (*{3}) do (ffmpeg -y -loglevel 32 -i \"%i\" -vf scale=iw*{1}:ih*{1} -sws_flags neighbor \"{2}\\%i\" ))", Path.GetDirectoryName(filePath), scaleValue, outputPath, Path.GetExtension(filePath)); 
                        outputPath = Path.Combine(outputPath, Regex.Match(Path.GetFileName(filePath), string.Format(@"^(([^<>/\\\|:""\*\?]*)([0-9]+)\{0}(?=\s-\s))", Path.GetExtension(filePath))).Value);
                    }

                    p.Start();
                    while (p.StandardOutput.ReadLine() != "") { }
                    p.StandardInput.WriteLine(string.Format("({0}) 2>&1 & exit", renderCommand));
                    logText = string.Format("{0}\r\n\r\n{1}", p.StandardOutput.ReadLine(), Encoding.UTF8.GetString(Encoding.Default.GetBytes(p.StandardOutput.ReadToEnd())));
                    p.WaitForExit();
                    Media newMedia = arrMedia.IsImageSequence() ? project.MediaPool.AddImageSequence(outputPath, GetFramesCount(filePath), vStream.FrameRate) : Media.CreateInstance(project, outputPath);
                    arrMedia.ReplaceWith(newMedia);
                }
            }

            catch (Exception ex)
            {
                if (MessageBox.Show(string.Format("FFmpeg Rendering Error! {0}\n\nDo you want to see the FFmpeg logs?", ex.Message), "Rendering Error!", MessageBoxButtons.YesNo) == DialogResult.Yes)
                {
                    string logPath = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.DesktopDirectory), string.Format("ffmpeg-{0}.log", DateTime.Now.ToString("yyyyMMdd-HHmmss")));
                    LogFile myLogFile = new LogFile(myVegas, logPath);
                    myLogFile.AddLogEntry(string.Format("If anything goes wrong, please submit an issue to the Github page below with this log.\r\nGithub Page: https://github.com/zzzzzz9125/Miscz/issues\r\nThe log file has been saved to {0}.\r\n\r\n{1}", logPath, logText));
                    myLogFile.Close();
                    myLogFile.ShowLogAsDialog("FFmpeg Logs");
                }
            }
        }

        public static int GetFramesCount(string filePath)
        {
            int indexStart = 0, indexEnd = 0;
            int.TryParse(Regex.Match(filePath, string.Format(@"([0-9]+)(?=\{0}\s-\s)", Path.GetExtension(filePath))).Value, out indexStart);
            int.TryParse(Regex.Match(filePath, string.Format(@"([0-9]+)(?=(\{0})$)", Path.GetExtension(filePath))).Value, out indexEnd);
            int count = indexEnd - indexStart + 1;
            return count;
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