using System;
using System.IO;
using System.Drawing;
using Microsoft.Win32;
using System.Diagnostics;
using System.Collections;
using System.Windows.Forms;
using System.Runtime.InteropServices;
using System.Text.RegularExpressions;

using ScriptPortal.Vegas;  // If you are using Sony Vegas Pro 13 or below, replace "ScriptPortal.Vegas" with "Sony.Vegas"

namespace Test_Script
{

    public class Class
    {
        public Vegas myVegas;
        bool canContinue, canClose, isPreCrop;
        float scrWidth, scrHeight, dFullWidth, dFullHeight;
        double dFullPixelAspect, frameRate;
        int[] count, spriteFrame, location, offset, preview, frameIndex, cut, frameRange;
        int language, countSelected;
        Form form, gridForm, gridFormSet;
        TextBox countXBox, countYBox, frameStartXBox, frameStartYBox, frameEndXBox, frameEndYBox, frameRateBox, startOffsetBox, topLeftXBox, topLeftYBox, bottomRightXBox, bottomRightYBox, cutXBox, cutYBox, previewXBox, previewYBox, loopOffsetBox, repeatFirstBox, repeatLastBox, repeatCountBox, safetyBox, gridDelayBox, autoDelayBox, gridHideDelayBox, scaleBox, gridOpacityBox;
        TrackBar countXBar, countYBar, safetyBar, gridDelayBar, autoDelayBar, gridHideDelayBar, scaleBar, gridOpacityBar;
        ComboBox directionBox, playbackBox, reimportBox, languageBox, cropModeBox;
        VideoEvent vEvent;
        VideoMotionBounds boundsPreCrop;
        VideoMotionKeyframe keyframePreview;
        Button showGridButton;
        TableLayoutPanel gridL;
        Color backColor, foreColor;
        ArrayList spritesArr;
        RegistryKey myReg;
        [DllImport("user32.dll", EntryPoint = "GetForegroundWindow")]
        public static extern IntPtr GetForegroundWindow();
        [DllImport("user32.dll", EntryPoint = "SetForegroundWindow")]
        public static extern bool SetForegroundWindow(IntPtr hWnd);
        public IntPtr Handle1;
        public void Main(Vegas vegas)
        {
            myVegas = vegas;
            Project project = myVegas.Project;
            bool ctrlMode = ((Control.ModifierKeys & Keys.Control) != 0) ? true : false;
            InterfaceType colorType;
            myVegas.GetInterfaceType(out colorType);
            switch (colorType)
            {
                case InterfaceType.Dark:
                case InterfaceType.Medium:
                    backColor = Color.FromArgb(45,45,45);
                    foreColor = Color.FromArgb(200,200,200);
                    break;
                case InterfaceType.Light:
                case InterfaceType.White:
                    backColor = Color.FromArgb(200,200,200);
                    foreColor = Color.FromArgb(45,45,45);
                    break;
            }
            string regPath = "Software\\MisczToolsForVegasPro\\SpriteSheetTool";
            if (ctrlMode)
            {
                Registry.CurrentUser.DeleteSubKeyTree(regPath);
            }
            myReg = Registry.CurrentUser.CreateSubKey(regPath);
            language = myReg.GetValue("Language") != null ? int.Parse((string)myReg.GetValue("Language")) : (myVegas.AppCultureInfo.TwoLetterISOLanguageName == "zh" ? 1 : 0);

            MediaBin spritesBin = null;
            foreach (IMediaBinNode node in project.MediaPool.RootMediaBin)
            {
                if (node.NodeType == MediaBinNodeType.Bin && ((MediaBin)node).Name == "Sprites")
                {
                    spritesBin = (MediaBin)node;
                    break;
                }
            }
            if (spritesBin == null)
            {
                spritesBin = new MediaBin(project, "Sprites");
                project.MediaPool.RootMediaBin.Add(spritesBin);
            }

            PlugInNode pluginBorder = myVegas.VideoFX.GetChildByUniqueID("{Svfx:com.vegascreativesoftware:border}");
            if (pluginBorder == null)
            {
                pluginBorder = myVegas.VideoFX.GetChildByUniqueID("{Svfx:com.sonycreativesoftware:border}");
            }
            scrWidth = project.Video.Width;
            scrHeight = project.Video.Height;
            foreach (Track myTrack in project.Tracks)
            {
                if (myTrack.IsVideo())
                {
                    VideoTrack myVideoTrack = (VideoTrack) myTrack;
                    foreach (TrackEvent evnt in myTrack.Events)
                    {
                        if (evnt.Selected)
                        {
                            if (vEvent != null)
                            {
                                vEvent.Selected = false;
                                Delay(300);
                            }

                            vEvent = (VideoEvent) evnt;

                            // Move the cursor
                            if(!(myVegas.Transport.CursorPosition.CompareTo(vEvent.Start) >= 0 && myVegas.Transport.CursorPosition.CompareTo(vEvent.End) <= 0))
                            {
                                Timecode cursor = Timecode.FromNanos(vEvent.Start.Nanos + vEvent.Length.Nanos / 4);
                                myVegas.Transport.CursorPosition = cursor;
                                myVegas.Transport.ViewCursor(true);
                            }

                            string filePath = vEvent.ActiveTake.Media.FilePath;
                            Take take0 = null, takeActiveSave = vEvent.ActiveTake, takeOriSave = null;
                            int number = -1, cropModeOri = 0;
                            bool isRevise = true;
                            if (Regex.IsMatch(Path.GetDirectoryName(filePath), @"_(Single)?(Crop)?$") && File.Exists(Regex.Replace(Path.GetDirectoryName(filePath), @"_(Single)?(Crop)?$", ".png")))
                            {
                                int.TryParse(Regex.Match(Path.GetFileNameWithoutExtension(filePath), @"((?<=_)[0-9][0-9][0-9])$").Value, out number);
                                cropModeOri = Regex.IsMatch(Path.GetDirectoryName(filePath), @"(_Single)$") ? 2 : 1;
                                filePath = Regex.Replace(Path.GetDirectoryName(filePath), @"_(Single)?(Crop)?$", ".png");
                            }
                            
                            else if (vEvent.ActiveTake.Media.IsImageSequence() || Regex.IsMatch(filePath, @"(_000\.png)$"))
                            {
                                if (File.Exists(Path.GetDirectoryName(filePath) + ".png"))
                                {
                                    number = 0;
                                    filePath = Path.GetDirectoryName(filePath) + ".png";
                                }

                                else if (File.Exists(Regex.Replace(Path.GetDirectoryName(filePath), @"(_[0-9][0-9])$", ".png")))
                                {
                                    int.TryParse(Regex.Match(Path.GetDirectoryName(filePath), @"((?<=_)[0-9][0-9])$").Value, out number);
                                    filePath = Regex.Replace(Path.GetDirectoryName(filePath), @"(_[0-9][0-9])$", ".png");
                                }
                            }

                            else if (Regex.IsMatch(filePath, @"((\.gif)|((_ProRes|_PNG)?\.mov))$"))
                            {                                
                                if (File.Exists(Regex.Replace(filePath, @"((\.gif)|((_ProRes|_PNG)?\.mov))$", ".png")))
                                {
                                    number = 0;
                                    filePath = Regex.Replace(filePath, @"((\.gif)|((_ProRes|_PNG)?\.mov))$", ".png");
                                }

                                else if (File.Exists(Regex.Replace(filePath, @"(_[0-9][0-9]((\.gif)|((_ProRes|_PNG)?\.mov)))$", ".png")))
                                {
                                    int.TryParse(Regex.Match(filePath, @"(?<=_)[0-9][0-9](?=((\.gif)|((_ProRes|_PNG)?\.mov))$)").Value, out number);
                                    filePath = Regex.Replace(filePath, @"(_[0-9][0-9]((\.gif)|((_ProRes|_PNG)?\.mov)))$", ".png");
                                }
                            }

                            else
                            {
                                isRevise = false;
                            }

                            if (!File.Exists(filePath))
                            {
                                myVegas.ShowError(LRZ("FileNotExistError"), string.Format(LRZ("FileNotExistErrorDetails"), filePath));
                                continue;
                            }

                            Media oriMedia = Media.CreateInstance(project, filePath);
                            spritesBin.Add(oriMedia);
                            for (int i = vEvent.Takes.Count - 1; i >= 0; i--)
                            {
                                Take take = vEvent.Takes[i];
                                if (take.MediaPath == filePath)
                                {
                                    takeOriSave = take;
                                    take0 = takeOriSave;
                                    vEvent.ActiveTake = takeOriSave;
                                    break;
                                }
                                else if (i == 0)
                                {
                                    take0 = vEvent.AddTake(oriMedia.GetVideoStreamByIndex(0), true);
                                }
                            }
                            vEvent.ResampleMode = VideoResampleMode.Disable;

                            vEvent.Selected = true;

                            VideoStream videoStream = (VideoStream)vEvent.ActiveTake.MediaStream;
                            dFullWidth = videoStream.Width;
                            dFullHeight = videoStream.Height;
                            dFullPixelAspect = videoStream.PixelAspectRatio;

                            if (!(myReg.GetValue("MultiMode") != null ? ((string)myReg.GetValue("MultiMode") == "1") : false) || count == null)
                            {
                                boundsPreCrop = null;
  
                                vEvent.VideoMotion.Keyframes.Clear();
                                keyframePreview = vEvent.VideoMotion.Keyframes[0];
                                int effectCount = vEvent.Effects.Count;

                                if (KeyframeChanged(keyframePreview))
                                {
                                    string message = LRZ("ResetCropCaution");
                                    string caption = LRZ("ResetCropCautionCaption");
                                    MessageBoxButtons buttons = MessageBoxButtons.YesNo;
                                    DialogResult result = MessageBox.Show(message, caption, buttons);
                                    if (result == DialogResult.Yes)
                                    {
                                        KeyframeReset(keyframePreview);
                                    }
                                    else
                                    {
                                        boundsPreCrop = keyframePreview.Bounds;
                                    }
                                }

                                // Add a white boarder to show the boundaries of the SpriteSheet image
                                Effect effectBorder = new Effect(pluginBorder);
                                vEvent.Effects.Insert(0, effectBorder);
                                effectBorder.ApplyBeforePanCrop = true;
                                OFXEffect ofxBorder = effectBorder.OFXEffect;
                                OFXChoiceParameter borderChoice = (OFXChoiceParameter)ofxBorder["Type"];
                                borderChoice.Value = borderChoice.Choices[2];
                                OFXDoubleParameter borderSize = (OFXDoubleParameter)ofxBorder["Size"];
                                borderSize.Value = 0.015;
                                myVegas.UpdateUI();

                                SpriteSheetSetWindow();

                                effectCount = vEvent.Effects.Count;

                                for (int i = effectCount - 1; i >= 0; i--)
                                {
                                    if (vEvent.Effects[i].PlugIn.UniqueID == pluginBorder.UniqueID)
                                    {
                                        vEvent.Effects.RemoveAt(i);
                                    }
                                }

                                if (!canContinue)
                                {
                                    vEvent.ActiveTake = takeActiveSave;
                                    if (!take0.Equals(takeOriSave) || isRevise)
                                    {
                                        if (!take0.Equals(takeOriSave))
                                        {
                                            vEvent.Takes.Remove(take0);
                                        }
                                        videoStream = (VideoStream)vEvent.ActiveTake.MediaStream;
                                        dFullWidth = videoStream.Width;
                                        dFullHeight = videoStream.Height;
                                        KeyframeReset(keyframePreview);
                                    }
                                    break;
                                }
                            }

                            int cropMode = myReg.GetValue("CropMode") != null ? int.Parse((string)myReg.GetValue("CropMode")) : 0;
                            if (spritesArr.Count == 1)
                            {
                                cropMode = 2;
                            }

                            int cols = location[2] - location[0] + 1, rows = location[3] - location[1] + 1;
                            double scaleFactor = myReg.GetValue("ScaleFactor") != null ? double.Parse((string)myReg.GetValue("ScaleFactor")) : 0;
                            if (scaleFactor < 1)
                            {
                                scaleFactor = Math.Ceiling(Math.Max(1, Math.Min(scrWidth / spriteFrame[0] / (cropMode == 1 ? cols : 1), scrHeight / spriteFrame[1] / (cropMode == 1 ? rows : 1))));
                            }

                            if (Path.GetFileName(Path.GetDirectoryName(filePath)) != "Sprites")
                            {
                                Directory.CreateDirectory(Path.Combine(Path.GetDirectoryName(filePath), "Sprites"));
                                string newPath = Path.Combine(Path.GetDirectoryName(filePath), "Sprites", Path.GetFileName(filePath));
                                File.Copy(filePath, newPath, true);
                                filePath = newPath;
                            }

                            if ((cropMode != cropModeOri) || !(myReg.GetValue("EnableRevise") != null ? (string)myReg.GetValue("EnableRevise") == "1" : true))
                            {
                                number = -1;
                                isRevise = false;
                            }

                            string outputDirectory = Path.Combine(Path.GetDirectoryName(filePath), Path.GetFileNameWithoutExtension(filePath) + (cropMode == 1 ? "_Crop" : cropMode == 2 ? "_Single" : number > 0 ? ("_" + string.Format("{0:00}", number)) : null));

                            if (number == -1)
                            {
                                number = 0;
                                if (cropMode == 0)
                                {
                                    while (Directory.Exists(outputDirectory) || File.Exists(outputDirectory + ".gif") || File.Exists(outputDirectory + ".mov") || File.Exists(outputDirectory + "_PNG.mov") || File.Exists(outputDirectory + "_ProRes.mov"))
                                    {
                                        number += 1;
                                        outputDirectory = Path.Combine(Path.GetDirectoryName(filePath), Path.GetFileNameWithoutExtension(filePath) + (number > 0 ? ("_" + string.Format("{0:00}", number)) : null));
                                    }
                                }
                                else
                                {
                                    while (File.Exists(Path.Combine(outputDirectory, Path.GetFileName(outputDirectory) + "_" + string.Format("{0:000}", number) + ".png")))
                                    {
                                        number += 1;
                                    }
                                }
                            }

                            if (Directory.Exists(outputDirectory) && cropMode == 0)
                            {
                                Directory.Delete(outputDirectory, true);
                            }

                            if (!Directory.Exists(outputDirectory))
                            {
                                Directory.CreateDirectory(outputDirectory);
                            }

                            int[] render = myReg.GetValue("Render") != null ? render = Array.ConvertAll(Regex.Split(Convert.ToString(myReg.GetValue("Render")), ","), int.Parse) : new int[] {0, 1, 0, 0, 0};

                            Process p = new Process();
                            p.StartInfo.FileName = "cmd.exe";
                            p.StartInfo.UseShellExecute = false;
                            p.StartInfo.RedirectStandardInput = true;
                            p.StartInfo.RedirectStandardOutput = true;
                            p.StartInfo.RedirectStandardError = false;
                            p.StartInfo.CreateNoWindow = true;

                            Form progressForm = new Form();
                            progressForm.ShowInTaskbar = false;
                            progressForm.AutoSize = true;
                            progressForm.BackColor = backColor;
                            progressForm.ForeColor = foreColor;
                            progressForm.Font = LRZFont(9);
                            progressForm.FormBorderStyle = FormBorderStyle.FixedToolWindow;
                            progressForm.StartPosition = FormStartPosition.CenterScreen;
                            progressForm.FormBorderStyle = FormBorderStyle.None;
                            progressForm.AutoSizeMode = AutoSizeMode.GrowAndShrink;

                            TableLayoutPanel pnl = new TableLayoutPanel();
                            pnl.AutoSize = true;
                            pnl.AutoSizeMode = AutoSizeMode.GrowAndShrink;
                            pnl.BorderStyle = BorderStyle.FixedSingle; 
                            pnl.ColumnCount = 2;
                            pnl.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 200));
                            pnl.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 110));
                            progressForm.Controls.Add(pnl);

                            ProgressBar prg = new ProgressBar();
                            prg.Width = 200;
                            prg.Height = 20;
                            prg.Minimum = 0;
                            prg.Maximum = 1 + spritesArr.Count + render[2] + render[3] + render[4];
                            prg.Value = 0;
                            prg.Margin = new Padding(10, 10, 10, 10);
                            pnl.Controls.Add(prg);

                            Label label = new Label();
                            label.Margin = new Padding(10, 10, 10, 10);
                            label.Text = LRZ("Rendering");
                            label.AutoSize = true;
                            pnl.Controls.Add(label);

                            if (cropMode == 0)
                            {
                                progressForm.Show(myVegas.TimelineWindow);
                            }

                            try
                            {
                                // PreCrop
                                if (location == null)
                                {
                                    location = new int[] {(int)keyframePreview.TopLeft.X / spriteFrame[0] + 1, (int)keyframePreview.TopLeft.Y / spriteFrame[1] + 1, (int)keyframePreview.BottomRight.X / spriteFrame[0], (int)keyframePreview.BottomRight.Y / spriteFrame[1]};
                                }
                                string preCropPath = cropMode == 0 ? (filePath + "_Crop.png") : Path.Combine(outputDirectory, Path.GetFileName(outputDirectory) + "_" + string.Format("{0:000}", number) +".png");
                                string renderParameter = "ffmpeg -y -i \"{0}\" -vf crop={1}:{2}:{3}:{4},scale=iw*{5}:ih*{5} -sws_flags neighbor";
                                string preCropCommand = string.Format(renderParameter, filePath, spriteFrame[0] * cols, spriteFrame[1] * rows, spriteFrame[0] * (location[0] - 1), spriteFrame[1] * (location[1] - 1), cropMode == 0 ? 1 : scaleFactor);
                                p.Start();
                                p.StandardInput.WriteLine(string.Format("{0} \"{1}\" & exit", preCropCommand, preCropPath));
                                p.WaitForExit();
                                prg.Value += 1;
                                label.Text = LRZ("Rendering") + new string('.', prg.Value / 6 % 4);
                                label.Refresh();

                                string reimportPath = preCropPath, openFolder = reimportPath;

                                if (cropMode == 0)
                                {
                                    // Render PNG Image Sequence
                                    for (int i = 0; i < spritesArr.Count; i++)
                                    {
                                        int r = (int)spritesArr[i] / cols;
                                        int c = (int)spritesArr[i] % cols;
                                        string renderCommand = string.Format(renderParameter, preCropPath, spriteFrame[0], spriteFrame[1], c * spriteFrame[0], r * spriteFrame[1], scaleFactor);
                                        string outputFile = Path.Combine(outputDirectory, Path.GetFileName(outputDirectory) + "_" + (cropMode == 2 ? string.Format("{0:000}", number) : string.Format("{0:000}", Mod(i - offset[0], spritesArr.Count))) +".png");
                                        p.Start();
                                        p.StandardInput.WriteLine(string.Format("{0} \"{1}\" & exit", renderCommand, outputFile));
                                        prg.Value += 1;
                                        label.Text = LRZ("Rendering") + new string('.', prg.Value / 6 % 4);
                                        label.Refresh();
                                    }
                                    p.WaitForExit();
                                    File.Delete(preCropPath);
                                    reimportPath = Path.Combine(outputDirectory, Path.GetFileName(outputDirectory) + "_000.png");
                                    openFolder = reimportPath;

                                    // Render Other formats
                                    if (render[2] > 0 || render[3] > 0 || render[4] > 0)
                                    {
                                        string [] renderCommand = new string[] {"-lavfi split[v],palettegen,[v]paletteuse", "-c:v copy", "-c:v prores_ks -profile:v 4444"};
                                        string [] renderPath = new string[] {".gif", (render[4] > 0 ? "_PNG.mov" : ".mov"), (render[3] > 0 ? "_ProRes.mov" : ".mov")};
                                        for (int i = 0; i < render.Length - 2; i++)
                                        {
                                            renderCommand[i] = string.Format("ffmpeg -y -r {0} -f image2 -i \"{1}\" {2}", frameRate, Path.Combine(outputDirectory, Path.GetFileName(outputDirectory) + "_%03d.png"), renderCommand[i]);
                                            renderPath[i] = Path.Combine(Path.GetDirectoryName(outputDirectory), Path.GetFileNameWithoutExtension(filePath) + (number > 0 ? ("_" + string.Format("{0:00}", number)) : null) + renderPath[i]);
                                            if (render[i + 2] > 0)
                                            {
                                                p.Start();
                                                p.StandardInput.WriteLine(string.Format("{0} \"{1}\" & exit", renderCommand[i], renderPath[i]));
                                                openFolder = renderPath[i];
                                                prg.Value += 1;
                                                label.Text = LRZ("Rendering") + new string('.', prg.Value / 6 % 4);
                                                label.Refresh();
                                            }
                                        }
                                        p.WaitForExit();

                                        if (render[0] > 0)
                                        {
                                            reimportPath = renderPath[render[0] - 1];
                                            openFolder = reimportPath;
                                        }

                                        if (render[0] > 0 && render[1] <= 0 && Directory.Exists(outputDirectory))
                                        {
                                            Directory.Delete(outputDirectory, true);
                                        }
                                    }
                                }

                                if ((myReg.GetValue("DisplayInFolder") != null ? ((string)myReg.GetValue("DisplayInFolder") == "1") : false) && File.Exists(openFolder))
                                {
                                    ExplorerFile(openFolder);
                                }

                                for (int i = vEvent.Takes.Count - 1; i >= 0; i--)
                                {
                                    Take take = vEvent.Takes[i];

                                    if (take.MediaPath == reimportPath)
                                    {
                                        vEvent.ActiveTake = take;
                                    }

                                    else if (i != 0 && take.MediaPath == filePath || (take.Media.IsImageSequence() && take.Equals(takeActiveSave)))
                                    {
                                        vEvent.Takes.RemoveAt(i);
                                    }

                                    else if (i == 0 && vEvent.ActiveTake.MediaPath != reimportPath)
                                    {
                                        Media importedMedia;
                                        if (cropMode == 0 && Regex.IsMatch(reimportPath, @"(_000\.png)$"))
                                        {
                                            importedMedia = project.MediaPool.AddImageSequence(reimportPath, spritesArr.Count, frameRate);
                                        }
                                        else
                                        {
                                            importedMedia = Media.CreateInstance(project, reimportPath);
                                        }
                                        spritesBin.Add(importedMedia);
                                        importedMedia.GetVideoStreamByIndex(0).AlphaChannel = VideoAlphaType.Straight;
                                        vEvent.AddTake(importedMedia.GetVideoStreamByIndex(0), true);
                                        vEvent.ResampleMode = VideoResampleMode.Disable;
                                        vEvent.Loop = myReg.GetValue("EnableLoop") != null ? ((string)myReg.GetValue("EnableLoop") == "1") : true;
                                    }
                                }

                                // To fix the problem that media files are not refreshed in Revise Mode
                                if (isRevise)
                                {
                                    string tmpPath = Path.Combine(Path.GetDirectoryName(filePath), Path.GetFileNameWithoutExtension(filePath) + "_tmp" + Path.GetExtension(filePath));
                                    File.Copy(reimportPath, tmpPath, true);
                                    Media tmpMedia = Media.CreateInstance(project, tmpPath);
                                    vEvent.ActiveTake.Media.ReplaceWith(tmpMedia);
                                    Media importedMedia;
                                    if (cropMode == 0 && Regex.IsMatch(reimportPath, @"(_000\.png)$"))
                                    {
                                        importedMedia = project.MediaPool.AddImageSequence(reimportPath, spritesArr.Count, frameRate);
                                    }
                                    else
                                    {
                                        importedMedia = Media.CreateInstance(project, reimportPath);
                                    }
                                    spritesBin.Add(importedMedia);
                                    tmpMedia.ReplaceWith(importedMedia);
                                    File.Delete(tmpPath);
                                }

                                dFullWidth = (cropMode == 1 ? cols : 1) * spriteFrame[0] * (float)scaleFactor;
                                dFullHeight = (cropMode == 1 ? rows : 1) * spriteFrame[1] * (float)scaleFactor;

                                KeyframeReset(keyframePreview);
                            }

                            catch (Exception ex)
                            {
                                MessageBox.Show(ex.Message);
                            }

                            finally
                            {
                                if (cropMode == 0)
                                {
                                    progressForm.Close();
                                    myVegas.UpdateUI();
                                }
                            }
                        }
                    }
                }
            }

            // Remove unused media
            foreach (IMediaBinNode node in spritesBin)
            {
                if (node.NodeType == MediaBinNodeType.MediaRef && ((Media)node).UseCount == 0)
                {
                    project.MediaPool.Remove(((Media)node).FilePath);
                }
            }
        }

        public static void Delay(int milliSecond)
        {
            int start = Environment.TickCount;
            while (Math.Abs(Environment.TickCount - start) < milliSecond)
            {
                Application.DoEvents();
            }
        }

        public VideoMotionKeyframe KeyframeReset(VideoMotionKeyframe keyframe)
        {
            keyframe.Rotation = 0.0;
            keyframe.Bounds = BoundsGenerate(dFullWidth, dFullHeight);
            boundsPreCrop = null;
            return keyframe;
        }

        public VideoMotionKeyframe KeyframePreCrop(VideoMotionKeyframe keyframe)
        {
            keyframe.Rotation = 0;
            VideoMotionBounds bounds = new VideoMotionBounds(keyframe.TopLeft, keyframe.TopRight, keyframe.BottomRight, keyframe.BottomLeft);

            bounds.TopLeft = new VideoMotionVertex((location[0] - 1) * spriteFrame[0], (location[1] - 1) * spriteFrame[1]);
            bounds.TopRight = new VideoMotionVertex(location[2] * spriteFrame[0], (location[1] - 1) * spriteFrame[1]);
            bounds.BottomLeft = new VideoMotionVertex((location[0] - 1) * spriteFrame[0], location[3] * spriteFrame[1]);
            bounds.BottomRight = new VideoMotionVertex(location[2] * spriteFrame[0], location[3] * spriteFrame[1]);
            float x = keyframe.TopLeft.X, y = keyframe.TopLeft.Y;
            keyframe.Bounds = bounds;
            keyframe.MoveBy(new VideoMotionVertex(x, y));
            bounds = keyframe.Bounds;
            boundsPreCrop = bounds;
            return keyframe;
        }

        public bool KeyframeChanged(VideoMotionKeyframe keyframe)
        {
            if (keyframe.Rotation != 0 || !PointEqual(keyframe.Center, new VideoMotionVertex(dFullWidth / 2, dFullHeight / 2)))
            {
                return true;
            }

            if (BoundsEqual(keyframe.Bounds, BoundsGenerate(dFullWidth, dFullHeight)))
            {
                return false;
            }

            else if (BoundsEqual(keyframe.Bounds, BoundsGenerate(scrWidth, scrHeight)))
            {
                return false;
            }

            else
            {
                return true;
            }
        }

        public static VideoMotionBounds BoundsGenerate(float width, float height, float moveByX = 0f, float moveByY = 0f)
        {
            VideoMotionBounds bounds = new VideoMotionBounds(new VideoMotionVertex(moveByX, moveByY), new VideoMotionVertex(width + moveByX, moveByY), new VideoMotionVertex(width + moveByX, height + moveByY), new VideoMotionVertex(moveByX, height + moveByY));
            return bounds;
        }

        public static bool BoundsEqual(VideoMotionBounds bounds1, VideoMotionBounds bounds2)
        {
            if (PointEqual(bounds1.TopLeft, bounds2.TopLeft) && PointEqual(bounds1.TopRight, bounds2.TopRight) && PointEqual(bounds1.BottomLeft, bounds2.BottomLeft) && PointEqual(bounds1.BottomRight, bounds2.BottomRight))
            {
                return true;
            }
            else
            {
                return false;
            }
        }

        public static bool PointEqual(VideoMotionVertex point1, VideoMotionVertex point2)
        {
            bool pointEqual = Math.Round(point1.X - point2.X, 4) == 0 && Math.Round(point1.Y - point2.Y, 4) == 0;
            return pointEqual;
        }

        public static double PointDistance(VideoMotionVertex point1, VideoMotionVertex point2)
        {
            double distance = Math.Sqrt(Math.Pow(point1.X - point2.X, 2) + Math.Pow(point1.Y - point2.Y, 2));
            return distance;
        }

        public static int Mod(double a, double b)
        {
            int c = (int)(a - Math.Floor(a / b) * b);
            return c;
        }

        private void SpriteSheetSetWindow()
        {
            string countReg = (boundsPreCrop == null) ? "Count" : "CountCroped";
            count = myReg.GetValue(countReg) != null ? Array.ConvertAll(Regex.Split(Convert.ToString(myReg.GetValue(countReg)), ","), int.Parse) : new int[] {8, 4};
            frameRate = myReg.GetValue("FrameRate") == null ? 10 : Convert.ToDouble(myReg.GetValue("FrameRate"));
            offset = new int[] {0, 0};
            frameRange = new int[] {1, 1, count[0], count[1]};
            frameIndex = new int[] {0, count[0] * count[1] - 1};
            spriteFrame = new int[] {(int) PointDistance(keyframePreview.TopLeft, keyframePreview.TopRight), (int) PointDistance(keyframePreview.TopLeft, keyframePreview.BottomLeft)};
            cut = new int[] {1, 1};
            isPreCrop = (myReg.GetValue("PreCropAtStart") != null ? (string)myReg.GetValue("PreCropAtStart") == "1" : true) && !KeyframeChanged(keyframePreview);
            canContinue = false;
            canClose = false;

            form = new Form();
            form.SuspendLayout();
            form.ShowInTaskbar = false;
            form.AutoSize = true;
            form.BackColor = backColor;
            form.ForeColor = foreColor;
            form.Font = LRZFont(9);
            form.FormBorderStyle = FormBorderStyle.SizableToolWindow;
            if (myReg.GetValue("FormLocation") != null)
            {
                int[] arr = Array.ConvertAll(Regex.Split(Convert.ToString(myReg.GetValue("FormLocation")), ","), int.Parse);
                form.Location = new Point(Math.Max(0, Math.Min(arr[0], Screen.GetWorkingArea(form).Width * 4 / 5)), Math.Max(0, Math.Min(arr[1], Screen.GetWorkingArea(form).Height * 4 / 5)));
                form.StartPosition = FormStartPosition.Manual;
            }
            else
            {
                form.StartPosition = FormStartPosition.WindowsDefaultBounds;
            }
            form.AutoSizeMode = AutoSizeMode.GrowAndShrink;
            form.Text = LRZ("FormTitle");
            form.FormClosing += new FormClosingEventHandler(form_FormClosing);
            form.Load += new EventHandler(form_Load);

            Panel panelBig = new Panel();
            panelBig.AutoSize = true;
            panelBig.AutoSizeMode = AutoSizeMode.GrowAndShrink;
            form.Controls.Add(panelBig);

            TableLayoutPanel l = new TableLayoutPanel();
            l.AutoSize = true;
            l.AutoSizeMode = AutoSizeMode.GrowAndShrink;
            l.GrowStyle = TableLayoutPanelGrowStyle.AddRows;
            l.ColumnCount = 3;
            l.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 160));
            l.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 65));
            l.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 65));

            panelBig.Controls.Add(l);
            
            ToolTip tt = new ToolTip();

            Label label = new Label();
            label.Margin = new Padding(6, 6, 6, 6);
            label.Text = LRZ("CountText");
            label.AutoSize = true;
            l.Controls.Add(label);
            tt.SetToolTip(label, LRZ("CountTips"));

            countYBox = new TextBox();
            countYBox.Text = string.Format("{0}", count[1]);
            l.Controls.Add(countYBox);
            tt.SetToolTip(countYBox, LRZ("CountTips"));
            countYBox.TextChanged += new EventHandler(countYBox_TextChanged);

            countXBox = new TextBox();
            countXBox.Text = string.Format("{0}", count[0]);
            l.Controls.Add(countXBox);
            tt.SetToolTip(countXBox, LRZ("CountTips"));
            countXBox.TextChanged += new EventHandler(countXBox_TextChanged);

            countYBar = new TrackBar();
            countYBar.AutoSize = false;
            countYBar.Height = countYBox.Height;
            countYBar.Margin = new Padding(0, 5, 0, 5);
            countYBar.Dock = DockStyle.Fill;
            countYBar.Minimum = 1;
            countYBar.Maximum = 20;
            countYBar.LargeChange = 2;
            countYBar.SmallChange = 1;
            countYBar.TickStyle = TickStyle.None;
            countYBar.Value = Math.Min(countYBar.Maximum, Math.Max(count[1], countYBar.Minimum));
            l.Controls.Add(countYBar);
            l.SetColumnSpan(countYBar, 3);
            countYBar.ValueChanged += new EventHandler(countYBar_ValueChanged);

            countXBar = new TrackBar();
            countXBar.AutoSize = false;
            countXBar.Height = countXBox.Height;
            countXBar.Margin = new Padding(0, 5, 0, 5);
            countXBar.Dock = DockStyle.Fill;
            countXBar.Minimum = 1;
            countXBar.Maximum = 20;
            countXBar.LargeChange = 2;
            countXBar.SmallChange = 1;
            countXBar.TickStyle = TickStyle.None;
            countXBar.Value = Math.Min(countXBar.Maximum, Math.Max(count[0], countXBar.Minimum));
            l.Controls.Add(countXBar);
            l.SetColumnSpan(countXBar, 3);
            countXBar.ValueChanged += new EventHandler(countXBar_ValueChanged);

            label = new Label();
            label.Margin = new Padding(6, 6, 6, 6);
            label.Text = LRZ("FrameStartText");
            label.Tag = "Frame";
            label.AutoSize = true;
            l.Controls.Add(label);
            tt.SetToolTip(label, LRZ("FrameStartTips"));

            frameStartYBox = new TextBox();
            frameStartYBox.Text = string.Format("{0}", frameRange[1]);
            frameStartYBox.Tag = "Frame";
            l.Controls.Add(frameStartYBox);
            tt.SetToolTip(frameStartYBox, LRZ("FrameStartTips"));
            
            frameStartXBox = new TextBox();
            frameStartXBox.Text = string.Format("{0}", frameRange[0]);
            frameStartXBox.Tag = "Frame";
            l.Controls.Add(frameStartXBox);
            tt.SetToolTip(frameStartXBox, LRZ("FrameStartTips"));

            label = new Label();
            label.Margin = new Padding(6, 6, 6, 6);
            label.Text = LRZ("FrameEndText");
            label.Tag = "Frame";
            label.AutoSize = true;
            l.Controls.Add(label);
            tt.SetToolTip(label, LRZ("FrameEndTips"));

            frameEndYBox = new TextBox();
            frameEndYBox.Text = string.Format("{0}", frameRange[3]);
            frameEndYBox.Tag = "Frame";
            l.Controls.Add(frameEndYBox);
            tt.SetToolTip(frameEndYBox, LRZ("FrameEndTips"));
            
            frameEndXBox = new TextBox();
            frameEndXBox.Text = string.Format("{0}", frameRange[2]);
            frameEndXBox.Tag = "Frame";
            l.Controls.Add(frameEndXBox);
            tt.SetToolTip(frameEndXBox, LRZ("FrameEndTips"));

            label = new Label();
            label.Margin = new Padding(6, 6, 6, 6);
            label.Text = LRZ("TopLeftText");
            label.Tag = "PreCrop";
            label.AutoSize = true;
            l.Controls.Add(label);
            tt.SetToolTip(label, LRZ("TopLeftTips"));

            topLeftYBox = new TextBox();
            topLeftYBox.Text = string.Format("{0}", 1);
            topLeftYBox.Tag = "PreCrop";
            l.Controls.Add(topLeftYBox);
            tt.SetToolTip(topLeftYBox, LRZ("TopLeftTips"));
            
            topLeftXBox = new TextBox();
            topLeftXBox.Text = string.Format("{0}", 1);
            topLeftXBox.Tag = "PreCrop";
            l.Controls.Add(topLeftXBox);
            tt.SetToolTip(topLeftXBox, LRZ("TopLeftTips"));

            label = new Label();
            label.Margin = new Padding(6, 6, 6, 6);
            label.Text = LRZ("BottomRightText");
            label.Tag = "PreCrop";
            label.AutoSize = true;
            l.Controls.Add(label);
            tt.SetToolTip(label, LRZ("BottomRightTips"));

            bottomRightYBox = new TextBox();
            bottomRightYBox.Text = countYBox.Text;
            bottomRightYBox.Tag = "PreCrop";
            l.Controls.Add(bottomRightYBox);
            tt.SetToolTip(bottomRightYBox, LRZ("BottomRightTips"));
            
            bottomRightXBox = new TextBox();
            bottomRightXBox.Text = countXBox.Text;
            bottomRightXBox.Tag = "PreCrop";
            l.Controls.Add(bottomRightXBox);
            tt.SetToolTip(bottomRightXBox, LRZ("BottomRightTips"));

            label = new Label();
            label.Margin = new Padding(6, 6, 6, 6);
            label.Text = LRZ("FrameRateText");
            label.Tag = "Frame";
            label.AutoSize = true;
            l.Controls.Add(label);
            tt.SetToolTip(label, LRZ("FrameRateTips"));

            frameRateBox = new TextBox();
            frameRateBox.Text = string.Format("{0}", frameRate);
            frameRateBox.Tag = "Frame";
            l.Controls.Add(frameRateBox);
            tt.SetToolTip(frameRateBox, LRZ("FrameRateTips"));
            frameRateBox.Leave += new EventHandler(frameRateBox_Leave);

            label = new Label();
            label.Margin = new Padding(2, 6, 2, 6);
            label.Text = "fps";
            label.Tag = "Frame";
            label.AutoSize = true;
            l.Controls.Add(label);

            label = new Label();
            label.Margin = new Padding(6, 6, 6, 6);
            label.Text = LRZ("OffsetText");
            label.Tag = "Frame";
            label.AutoSize = true;
            l.Controls.Add(label);
            tt.SetToolTip(label, LRZ("OffsetTips"));

            startOffsetBox = new TextBox();
            startOffsetBox.Text = string.Format("{0}", offset[0]);
            startOffsetBox.Tag = "Frame";
            l.Controls.Add(startOffsetBox);
            tt.SetToolTip(startOffsetBox, LRZ("OffsetTips"));
            // startOffsetBox.TextChanged += new EventHandler(refreshIndex_Handler);

            loopOffsetBox = new TextBox();
            loopOffsetBox.Text = string.Format("{0}", offset[1]);
            loopOffsetBox.Tag = "Frame";
            l.Controls.Add(loopOffsetBox);
            tt.SetToolTip(loopOffsetBox, LRZ("OffsetTips"));
            loopOffsetBox.TextChanged += new EventHandler(refreshIndex_Handler);

            label = new Label();
            label.Margin = new Padding(6, 6, 6, 6);
            label.Text = LRZ("DirectionText");
            label.Tag = "Frame";
            label.AutoSize = true;
            l.Controls.Add(label);
            tt.SetToolTip(label, LRZ("DirectionTips"));

            directionBox = new ComboBox();
            directionBox.DataSource = LRZArr("DirectionChoices");
            directionBox.DropDownStyle = ComboBoxStyle.DropDownList;
            directionBox.Dock = DockStyle.Fill;
            directionBox.Tag = "Frame";
            l.Controls.Add(directionBox);
            l.SetColumnSpan(directionBox, 2);
            tt.SetToolTip(directionBox, LRZ("DirectionTips"));
            directionBox.SelectedIndexChanged += new EventHandler(refreshIndex_Handler);

            label = new Label();
            label.Margin = new Padding(6, 6, 6, 6);
            label.Text = LRZ("PlaybackText");
            label.Tag = "Frame";
            label.AutoSize = true;
            l.Controls.Add(label);
            tt.SetToolTip(label, LRZ("PlaybackTips"));

            playbackBox = new ComboBox();
            playbackBox.DataSource = LRZArr("PlaybackChoices");
            playbackBox.DropDownStyle = ComboBoxStyle.DropDownList;
            playbackBox.Dock = DockStyle.Fill;
            playbackBox.Tag = "Frame";
            l.Controls.Add(playbackBox);
            l.SetColumnSpan(playbackBox, 2);
            tt.SetToolTip(playbackBox, LRZ("PlaybackTips"));
            playbackBox.SelectedIndexChanged += new EventHandler(refreshIndex_Handler);

            label = new Label();
            label.Margin = new Padding(6, 6, 6, 6);
            label.Text = LRZ("RepeatText");
            label.Tag = "Frame";
            label.AutoSize = true;
            l.Controls.Add(label);
            tt.SetToolTip(label, LRZ("RepeatTips"));

            repeatFirstBox = new TextBox();
            repeatFirstBox.Text = string.Format("{0}", 0);
            repeatFirstBox.Tag = "Frame";
            l.Controls.Add(repeatFirstBox);
            tt.SetToolTip(repeatFirstBox, LRZ("RepeatTips"));

            repeatLastBox = new TextBox();
            repeatLastBox.Text = string.Format("{0}", 0);
            repeatLastBox.Tag = "Frame";
            l.Controls.Add(repeatLastBox);
            tt.SetToolTip(repeatLastBox, LRZ("RepeatTips"));

            label = new Label();
            label.Margin = new Padding(6, 6, 6, 6);
            label.Text = LRZ("RepeatCountText");
            label.Tag = "Frame";
            label.AutoSize = true;
            l.Controls.Add(label);
            tt.SetToolTip(label, LRZ("RepeatCountTips"));

            repeatCountBox = new TextBox();
            repeatCountBox.Text = string.Format("{0}", 0);
            repeatCountBox.Tag = "Frame";
            l.Controls.Add(repeatCountBox);
            tt.SetToolTip(repeatCountBox, LRZ("RepeatCountTips"));

            label = new Label();
            label.Tag = "Frame";
            l.Controls.Add(label);

            label = new Label();
            label.Margin = new Padding(6, 6, 6, 6);
            label.Text = LRZ("CutText");
            label.Tag = "Frame";
            label.AutoSize = true;
            l.Controls.Add(label);
            tt.SetToolTip(label, LRZ("CutTips"));

            cutXBox = new TextBox();
            cutXBox.Text = string.Format("{0}", cut[0]);
            cutXBox.Tag = "Frame";
            l.Controls.Add(cutXBox);
            tt.SetToolTip(cutXBox, LRZ("CutTips"));
            cutXBox.TextChanged += new EventHandler(refreshIndex_Handler);

            cutYBox = new TextBox();
            cutYBox.Text = string.Format("{0}", cut[1]);
            cutYBox.Tag = "Frame";
            l.Controls.Add(cutYBox);
            tt.SetToolTip(cutYBox, LRZ("CutTips"));
            cutYBox.TextChanged += new EventHandler(refreshIndex_Handler);

            label = new Label();
            label.Margin = new Padding(6, 6, 6, 6);
            label.Text = LRZ("ScaleFactorText");
            label.Tag = "Frame";
            label.AutoSize = true;
            l.Controls.Add(label);

            scaleBox = new TextBox();
            scaleBox.AutoSize = true;
            scaleBox.Tag = "Frame";
            l.Controls.Add(scaleBox);
            scaleBox.TextChanged += new EventHandler(scaleBox_TextChanged);
            tt.SetToolTip(scaleBox, LRZ("ScaleFactorTips"));

            label = new Label();
            label.Tag = "Frame";
            l.Controls.Add(label);

            double scaleFactor = myReg.GetValue("ScaleFactor") != null ? double.Parse((string)myReg.GetValue("ScaleFactor")) : 0;

            scaleBar = new TrackBar();
            scaleBar.AutoSize = false;
            scaleBar.Height = scaleBox.Height;
            scaleBar.Tag = "Frame";
            scaleBar.Margin = new Padding(0, 5, 0, 5);
            scaleBar.Dock = DockStyle.Fill;
            scaleBar.Minimum = 0;
            scaleBar.Maximum = 100;
            scaleBar.LargeChange = 10;
            scaleBar.SmallChange = 1;
            scaleBar.TickStyle = TickStyle.None;
            scaleBar.Value = (int)Math.Floor(Math.Min(scaleBar.Maximum, Math.Max(scaleFactor, scaleBar.Minimum)));
            l.Controls.Add(scaleBar);
            l.SetColumnSpan(scaleBar, 3);
            scaleBar.ValueChanged += new EventHandler(scaleBar_ValueChanged);
            tt.SetToolTip(scaleBar, LRZ("ScaleFactorTips"));

            scaleBox.Text = scaleFactor < 1 ? LRZ("Auto") : string.Format("{0}", scaleFactor);

            label = new Label();
            label.Margin = new Padding(6, 6, 6, 6);
            label.Text = LRZ("CropModeText");
            label.Tag = "PreCrop";
            label.AutoSize = true;
            l.Controls.Add(label);
            tt.SetToolTip(label, LRZ("CropModeTips"));

            cropModeBox = new ComboBox();
            cropModeBox.DataSource = LRZArr("CropModeChoices");
            cropModeBox.DropDownStyle = ComboBoxStyle.DropDownList;
            cropModeBox.Dock = DockStyle.Fill;
            cropModeBox.Tag = "PreCrop";
            l.Controls.Add(cropModeBox);
            l.SetColumnSpan(cropModeBox, 2);
            tt.SetToolTip(cropModeBox, LRZ("CropModeTips"));
            cropModeBox.SelectedIndexChanged += new EventHandler(cropMode_SelectedIndexChanged);

            CheckBox enableLoop = new CheckBox();
            enableLoop.Text = LRZ("EnableLoopText");
            enableLoop.Margin = new Padding(0, 3, 0, 3);
            enableLoop.AutoSize = true;
            enableLoop.Checked = myReg.GetValue("EnableLoop") != null ? ((string)myReg.GetValue("EnableLoop") == "1") : true;
            enableLoop.Tag = "Frame";
            l.Controls.Add(enableLoop);
            enableLoop.CheckedChanged += new EventHandler(enableLoop_CheckedChanged);


            CheckBox autoMode = new CheckBox();
            autoMode.Text = LRZ("AutoModeText");
            autoMode.AutoSize = true;
            autoMode.Checked = myReg.GetValue("AutoMode") != null ? ((string)myReg.GetValue("AutoMode") == "1") : true;
            autoMode.Tag = "Frame";
            l.Controls.Add(autoMode);
            l.SetColumnSpan(autoMode, 2);
            autoMode.CheckedChanged += new EventHandler(autoMode_CheckedChanged);

            FlowLayoutPanel panel = new FlowLayoutPanel();
            panel.FlowDirection = FlowDirection.RightToLeft;
            panel.AutoSize = true;
            panel.AutoSizeMode = AutoSizeMode.GrowAndShrink;
            panel.Anchor = AnchorStyles.Bottom;
            l.Controls.Add(panel);
            l.SetColumnSpan(panel, 3);
            panel.Font = LRZFont(8);

            Size buttonSize = new Size (80, 25);
            Button cancel = new Button();
            cancel.Size = buttonSize;
            cancel.Text = LRZ("Cancel");
            panel.Controls.Add(cancel);
            form.CancelButton = cancel;
            cancel.Click += new EventHandler(cancel_Click);

            Button ok = new Button();
            ok.Size = buttonSize;
            ok.Text = LRZ("OK");
            panel.Controls.Add(ok);
            form.AcceptButton = ok;
            ok.Click += new EventHandler(ok_Click);

            Button Settings = new Button();
            Settings.Size = buttonSize;
            Settings.Text = LRZ("Settings");
            panel.Controls.Add(Settings);
            Settings.Click += new EventHandler(Settings_Click);

            Button resetCropBox = new Button();
            resetCropBox.Size = buttonSize;
            resetCropBox.Text = LRZ("ResetCrop");
            panel.Controls.Add(resetCropBox);
            resetCropBox.Click += new EventHandler(resetCropBox_Click);

            showGridButton = new Button();
            showGridButton.Size = buttonSize;
            showGridButton.Text = LRZ("ShowGrid");
            panel.Controls.Add(showGridButton);
            showGridButton.Click += new EventHandler(showGridButton_Click);

            Button renderAs = new Button();
            renderAs.Size = buttonSize;
            renderAs.Text = LRZ("RenderAs");
            panel.Controls.Add(renderAs);
            renderAs.Click += new EventHandler(renderAs_Click);

            preCropSet();
            showGrid();

            while (!canClose && !canContinue)
            {

                IntPtr a = GetForegroundWindow();
                try
                {
                    Delay(300);
                }
                catch
                {

                }

                if (GetForegroundWindow() == myVegas.MainWindow.Handle)
                {
                    if (gridForm != null)
                    {
                        if (a == gridForm.Handle || a == Handle1)
                        {
                            int gridHideDelay = myReg.GetValue("DelayGridHide") != null ? int.Parse((string)myReg.GetValue("DelayGridHide")) : 1200;
                            if (gridHideDelay > 0)
                            {
                                if (true == (bool)showGridButton.Tag)
                                {
                                    showGridButton.Tag = false;
                                    gridForm.Hide();
                                    showGridButton.Text = LRZ("ShowGrid");
                                    Delay(gridHideDelay);
                                }
                                showGridButton.Tag = true;
                                gridForm.Show();
                                SetForegroundWindow(gridForm.Handle);
                                showGridButton.Text = LRZ("HideGrid");
                            }
                        }
                        else if (a == myVegas.MainWindow.Handle)
                        {
                            gridForm.Show();
                        }
                    }
                }

                if (!canClose)
                {
                    if (form.Location.X > Screen.GetWorkingArea(form).Width - 30 || form.Location.Y > Screen.GetWorkingArea(form).Height - 30)
                    {
                        form.Location = new Point(Math.Min(form.Location.X, Screen.GetWorkingArea(form).Width - 30), Math.Min(form.Location.Y, Screen.GetWorkingArea(form).Height - 30));
                    }

                    if (true == (bool)showGridButton.Tag)
                    {
                        if (gridForm.Location.X > Screen.GetWorkingArea(gridForm).Width - 30 || gridForm.Location.Y > Screen.GetWorkingArea(gridForm).Height - 30)
                        {
                            gridForm.Location = new Point(Math.Min(gridForm.Location.X, Screen.GetWorkingArea(gridForm).Width - 30), Math.Min(gridForm.Location.Y, Screen.GetWorkingArea(gridForm).Height - 30));
                        }
                    }
                }
            }
        }

        private void form_Load(object sender, EventArgs e)
        {
            Handle1 = form.Handle;
            cropModeBox.SelectedIndex = myReg.GetValue("CropMode") != null ? int.Parse((string)myReg.GetValue("CropMode")) : 0;
        }

        private void ok_Click(object sender, EventArgs e)
        {
            int countSelectedSaved = countSelected;
            countSelected += 1;

            changeColor(countSelected < 2);
            
            if (countSelected == countSelectedSaved + 1)
            {
                if (isPreCrop)
                {
                    preCropOk();
                }
                else
                {
                    spriteOk();
                }
            }
        }

        private void spriteOk()
        {
            gridForm.Hide();
            count = new int[] {int.Parse(countXBox.Text), int.Parse(countYBox.Text)};
            spriteFrame = new int[] {spriteFrame[0] / count[0], spriteFrame[1] / count[1]};
            frameRange = new int[] {int.Parse(frameStartXBox.Text), int.Parse(frameStartYBox.Text), int.Parse(frameEndXBox.Text), int.Parse(frameEndYBox.Text)};
            offset = new int[] {int.Parse(startOffsetBox.Text), int.Parse(loopOffsetBox.Text)};
            int[] repeat = new int[] {int.Parse(repeatFirstBox.Text), int.Parse(repeatLastBox.Text), int.Parse(repeatCountBox.Text)};
            if (repeat[0] > repeat[1])
            {
                int tmp = repeat[0];
                repeat[0] = repeat[1];
                repeat[1] = tmp;
            }
            ArrayList repeatArr = new ArrayList();
            if (spritesArr == null)
            {
                spritesArr = new ArrayList();
                spritesArr.Add(0);
                if ((myReg.GetValue("CropMode") != null ? (string)myReg.GetValue("CropMode") : "0") == "1")
                {
                    spritesArr.Add(0);
                }
            }
            for (int i = 0; i < repeat[2]; i++)
            {
                for (int j = repeat[0]; j <= repeat[1]; j++)
                {
                    repeatArr.Add(j + Math.Min((int)spritesArr[0], (int)spritesArr[spritesArr.Count - 1]));
                }
            }
            for (int i = 0; i < spritesArr.Count; i++)
            {
                if ((int)spritesArr[i] == repeat[1])
                {
                    spritesArr.InsertRange(i + 1, repeatArr);
                    break;
                }
            }
            if (playbackBox.SelectedIndex > 0)
            {
                repeatArr.Reverse();
                for (int i = spritesArr.Count - 1; i >= 0; i--)
                {
                    if ((int)spritesArr[i] == repeat[1])
                    {
                        spritesArr.InsertRange(i, repeatArr);
                        break;
                    }
                }
            }
            cut = new int[] {int.Parse(cutXBox.Text), int.Parse(cutYBox.Text)};
            canContinue = true;
            form.Close();
        }
        private void cancel_Click(object sender, EventArgs e)
        {
            canClose = true;
            form.Close();
        }

        private void form_FormClosing(object sender, EventArgs e)
        {
            myReg.SetValue("FormLocation", ((Form)sender).Location.X + "," + ((Form)sender).Location.Y);
            canClose = true;
        }

        private void showGridButton_Click(object sender, EventArgs e)
        {
            showGrid();
        }
        private void countXBox_TextChanged(object sender, EventArgs e)
        {
            try
            {
                int safetyThreshold = 300;
                if (myReg.GetValue("SafetyThreshold") != null)
                {
                    safetyThreshold = int.Parse((string)myReg.GetValue("SafetyThreshold"));
                }
                if (count[0] != int.Parse(countXBox.Text))
                {
                    if (int.Parse(countXBox.Text) * count[1] > safetyThreshold)
                    {
                        return;
                    }
                    count[0] = int.Parse(countXBox.Text);
                    if (count[0] >= countXBar.Minimum && count[0] <= countXBar.Maximum)
                    {
                        countXBar.Value = count[0];
                    }
                    myReg.SetValue(isPreCrop ? "Count" : "CountCroped", count[0] + "," + count[1]);
                    if (gridForm != null && gridForm.Visible)
                    {
                        refreshGrid();
                    }
                    if (int.Parse(frameStartXBox.Text) > count[0])
                    {
                        frameStartXBox.Text = string.Format("{0}", count[0]);
                    }

                    frameEndXBox.Text = string.Format("{0}", count[0]);

                    if (int.Parse(topLeftXBox.Text) > count[0])
                    {
                        topLeftXBox.Text = string.Format("{0}", count[0]);
                    }
                    bottomRightXBox.Text = string.Format("{0}", count[0]);
                }
            }
            catch
            {

            }

        }

        private void countYBox_TextChanged(object sender, EventArgs e)
        {
            try
            {
                int safetyThreshold = myReg.GetValue("SafetyThreshold") != null ? int.Parse((string)myReg.GetValue("SafetyThreshold")) : 300;

                if (count[1] != int.Parse(countYBox.Text))
                {
                    if (int.Parse(countYBox.Text) * count[0] > safetyThreshold)
                    {
                        return;
                    }
                    count[1] = int.Parse(countYBox.Text);
                    if (count[1] >= countYBar.Minimum && count[1] <= countYBar.Maximum)
                    {
                        countYBar.Value = count[1];
                    }
                    myReg.SetValue(isPreCrop ? "Count" : "CountCroped", count[0] + "," + count[1]);
                    if (gridForm != null && gridForm.Visible)
                    {
                        refreshGrid();
                    }
                    if (int.Parse(frameStartYBox.Text) > count[1])
                    {
                        frameStartYBox.Text = string.Format("{0}", count[1]);
                    }

                    if (int.Parse(frameEndYBox.Text) > count[1])
                    {
                        frameEndYBox.Text = string.Format("{0}", count[1]);
                    }

                    if (int.Parse(topLeftYBox.Text) > count[1])
                    {
                        topLeftYBox.Text = string.Format("{0}", count[1]);
                    }
                    bottomRightYBox.Text = string.Format("{0}", count[1]);
                }
            }
            catch
            {

            }

        }

        private void countXBar_ValueChanged(object sender, EventArgs e)
        {
            if (count[0] != ((TrackBar)sender).Value)
            {
                countXBox.Text = string.Format("{0}", ((TrackBar)sender).Value);
            }
        }

        private void countYBar_ValueChanged(object sender, EventArgs e)
        {
            if (count[1] != ((TrackBar)sender).Value)
            {
                countYBox.Text = string.Format("{0}", ((TrackBar)sender).Value);
            }
        }

        private void frameRateBox_Leave(object sender, EventArgs e)
        {
            try
            {
                frameRate = double.Parse(frameRateBox.Text);
                myReg.SetValue("FrameRate", frameRate.ToString());
            }
            catch
            {

            }

        }

        private void cropMode_SelectedIndexChanged(object sender, EventArgs e)
        {
            myReg.SetValue("CropMode", ((ComboBox)sender).SelectedIndex.ToString());
        }

        private void enableLoop_CheckedChanged(object sender, EventArgs e)
        {
            myReg.SetValue("EnableLoop", (((CheckBox)sender).Checked ? 1 : 0).ToString());
        }

        private void autoMode_CheckedChanged(object sender, EventArgs e)
        {
            myReg.SetValue("AutoMode", (((CheckBox)sender).Checked ? 1 : 0).ToString());
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
            scaleBox.Text = ((TrackBar)sender).Value < 1 ? LRZ("Auto") : string.Format("{0}", ((TrackBar)sender).Value);
        }

        private void preCropSet()
        {
            form.Hide();
            foreach (Control ctrl in countXBox.Parent.Controls)
            {
                string str = (String) ctrl.Tag;
                ctrl.Visible = str == "PreCrop" ? isPreCrop : str == "Frame" ? !isPreCrop : true;
            }
            form.Text = isPreCrop ? (LRZ("FormTitle") + " - " + LRZ("PreCrop")) : LRZ("FormTitle");
            countSelected = 0;
            form.ResumeLayout();
            form.Show(myVegas.TimelineWindow);
        }

        public void preCropOk(bool cropMode = false)
        {
            try
            {
                spriteFrame = new int[] {spriteFrame[0] / count[0], spriteFrame[1] / count[1]};
                location = new int[] {int.Parse(topLeftXBox.Text), int.Parse(topLeftYBox.Text), int.Parse(bottomRightXBox.Text), int.Parse(bottomRightYBox.Text)};

                VideoMotionKeyframe keyframe = vEvent.VideoMotion.Keyframes[0];
                KeyframePreCrop(keyframe);
                myVegas.UpdateUI();
                count = new int[] {location[2] - location[0] + 1, location[3] - location[1] + 1};

                spriteFrame = new int[] {spriteFrame[0] * count[0], spriteFrame[1] * count[1]};
                countXBox.Text = string.Format("{0}", count[0]);
                countYBox.Text = string.Format("{0}", count[1]);
                myReg.SetValue("CountCroped", count[0] + "," + count[1]);
                if (int.Parse(frameStartXBox.Text) > count[0])
                {
                    frameStartXBox.Text = string.Format("{0}", count[0]);
                }
                if (int.Parse(frameStartYBox.Text) > count[1])
                {
                    frameStartYBox.Text = string.Format("{0}", count[1]);
                }
                frameEndXBox.Text = string.Format("{0}", count[0]);
                frameEndYBox.Text = string.Format("{0}", count[1]);
                int start = (int.Parse(frameStartYBox.Text) - 1) * count[0] + int.Parse(frameStartXBox.Text) - 1;
                int end = (int.Parse(frameEndYBox.Text) - 1) * count[0] + int.Parse(frameEndXBox.Text) - 1;
                ToIndexes(start, end);
                isPreCrop = !isPreCrop;
                countXBar.Value = count[0];
                countYBar.Value = count[1];
                if (!cropMode)
                {
                    refreshGrid(true);
                }
            }
            catch
            {

            }
        }

        private void Settings_Click(object sender, EventArgs e)
        {
            Form settingsForm = new Form();
            settingsForm.StartPosition = FormStartPosition.CenterScreen;
            settingsForm.FormBorderStyle = FormBorderStyle.FixedToolWindow;
            settingsForm.MaximizeBox = false;
            settingsForm.MinimizeBox = false;
            settingsForm.HelpButton = false;
            settingsForm.ShowInTaskbar = false;
            settingsForm.AutoSize = true;
            settingsForm.Owner = form;
            settingsForm.TopMost = true;
            settingsForm.AutoSize = true;
            settingsForm.BackColor = backColor;
            settingsForm.ForeColor = foreColor;
            settingsForm.AutoSizeMode = AutoSizeMode.GrowAndShrink;
            settingsForm.Text = LRZ("Settings");
            settingsForm.Font = LRZFont(9);
            settingsForm.Load += new EventHandler(settingsForm_Load);

            Panel panelBig = new Panel();
            panelBig.AutoSize = true;
            panelBig.AutoSizeMode = AutoSizeMode.GrowAndShrink;
            settingsForm.Controls.Add(panelBig);

            TableLayoutPanel settingsL = new TableLayoutPanel();
            settingsL.AutoSize = true;
            settingsL.AutoSizeMode = AutoSizeMode.GrowAndShrink;
            settingsL.GrowStyle = TableLayoutPanelGrowStyle.AddRows;
            settingsL.ColumnCount = 3;
            settingsL.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, language == 1 ? 120 : 140));
            settingsL.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 50));
            settingsL.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 110));
            panelBig.Controls.Add(settingsL);

            ToolTip tt = new ToolTip();

            Label label = new Label();
            label.Margin = new Padding(6, 6, 6, 6);
            label.Text = LRZ("SafetyText");
            label.AutoSize = true;
            settingsL.Controls.Add(label);

            safetyBox = new TextBox();
            safetyBox.AutoSize = true;
            settingsL.Controls.Add(safetyBox);
            safetyBox.TextChanged += new EventHandler(safetyBox_TextChanged);
            tt.SetToolTip(safetyBox, LRZ("SafetyTips"));

            int safetyThreshold = myReg.GetValue("SafetyThreshold") != null ? int.Parse((string)myReg.GetValue("SafetyThreshold")) : 300;

            safetyBar = new TrackBar();
            safetyBar.AutoSize = false;
            safetyBar.Height = safetyBox.Height;
            safetyBar.Margin = new Padding(0, 5, 0, 5);
            safetyBar.Dock = DockStyle.Fill;
            safetyBar.Minimum = 50;
            safetyBar.Maximum = 1000;
            safetyBar.LargeChange = 100;
            safetyBar.SmallChange = 50;
            safetyBar.TickStyle = TickStyle.None;
            safetyBar.Value = Math.Min(safetyBar.Maximum, Math.Max(safetyThreshold, safetyBar.Minimum));
            settingsL.Controls.Add(safetyBar);
            safetyBar.ValueChanged += new EventHandler(safetyBar_ValueChanged);
            tt.SetToolTip(safetyBar, LRZ("SafetyTips"));

            safetyBox.Text = string.Format("{0}", safetyBar.Value);

            label = new Label();
            label.Margin = new Padding(6, 6, 6, 6);
            label.Text = LRZ("GridDelayText");
            label.AutoSize = true;
            settingsL.Controls.Add(label);

            gridDelayBox = new TextBox();
            gridDelayBox.AutoSize = true;
            settingsL.Controls.Add(gridDelayBox);
            gridDelayBox.TextChanged += new EventHandler(gridDelayBox_TextChanged);
            tt.SetToolTip(gridDelayBox, LRZ("GridDelayTips"));

            int gridDelay = myReg.GetValue("DelayGrid") != null ? int.Parse((string)myReg.GetValue("DelayGrid")) : 35;

            gridDelayBar = new TrackBar();
            gridDelayBar.AutoSize = false;
            gridDelayBar.Height = gridDelayBox.Height;
            gridDelayBar.Margin = new Padding(0, 5, 0, 5);
            gridDelayBar.Dock = DockStyle.Fill;
            gridDelayBar.Minimum = 0;
            gridDelayBar.Maximum = 200;
            gridDelayBar.LargeChange = 20;
            gridDelayBar.SmallChange = 5;
            gridDelayBar.TickStyle = TickStyle.None;
            gridDelayBar.Value = Math.Min(gridDelayBar.Maximum, Math.Max(gridDelay, gridDelayBar.Minimum));
            settingsL.Controls.Add(gridDelayBar);
            gridDelayBar.ValueChanged += new EventHandler(gridDelayBar_ValueChanged);
            tt.SetToolTip(gridDelayBar, LRZ("GridDelayTips"));

            gridDelayBox.Text = string.Format("{0}", gridDelayBar.Value);

            label = new Label();
            label.Margin = new Padding(6, 6, 6, 6);
            label.Text = LRZ("GridHideDelayText");
            label.AutoSize = true;
            settingsL.Controls.Add(label);

            gridHideDelayBox = new TextBox();
            gridHideDelayBox.AutoSize = true;
            settingsL.Controls.Add(gridHideDelayBox);
            gridHideDelayBox.TextChanged += new EventHandler(gridHideDelayBox_TextChanged);
            tt.SetToolTip(gridHideDelayBox, LRZ("GridHideDelayTips"));

            int gridHideDelay = myReg.GetValue("DelayGridHide") != null ? int.Parse((string)myReg.GetValue("DelayGridHide")) : 1200;

            gridHideDelayBar = new TrackBar();
            gridHideDelayBar.AutoSize = false;
            gridHideDelayBar.Height = gridHideDelayBox.Height;
            gridHideDelayBar.Margin = new Padding(0, 5, 0, 5);
            gridHideDelayBar.Dock = DockStyle.Fill;
            gridHideDelayBar.Minimum = 0;
            gridHideDelayBar.Maximum = 2500;
            gridHideDelayBar.LargeChange = 200;
            gridHideDelayBar.SmallChange = 100;
            gridHideDelayBar.TickStyle = TickStyle.None;
            gridHideDelayBar.Value = Math.Min(gridHideDelayBar.Maximum, Math.Max(gridHideDelay, gridHideDelayBar.Minimum));
            settingsL.Controls.Add(gridHideDelayBar);
            gridHideDelayBar.ValueChanged += new EventHandler(gridHideDelayBar_ValueChanged);
            tt.SetToolTip(gridHideDelayBar, LRZ("GridHideDelayTips"));

            gridHideDelayBox.Text = string.Format("{0}", gridHideDelayBar.Value);

            label = new Label();
            label.Margin = new Padding(6, 6, 6, 6);
            label.Text = LRZ("AutoDelayText");
            label.AutoSize = true;
            settingsL.Controls.Add(label);

            autoDelayBox = new TextBox();
            autoDelayBox.AutoSize = true;
            settingsL.Controls.Add(autoDelayBox);
            autoDelayBox.TextChanged += new EventHandler(autoDelayBox_TextChanged);
            tt.SetToolTip(autoDelayBox, LRZ("AutoDelayTips"));

            int autoDelay = myReg.GetValue("DelayAuto") != null ? int.Parse((string)myReg.GetValue("DelayAuto")) : 1000;

            autoDelayBar = new TrackBar();
            autoDelayBar.AutoSize = false;
            autoDelayBar.Height = autoDelayBox.Height;
            autoDelayBar.Margin = new Padding(0, 5, 0, 5);
            autoDelayBar.Dock = DockStyle.Fill;
            autoDelayBar.Minimum = 0;
            autoDelayBar.Maximum = 2500;
            autoDelayBar.LargeChange = 200;
            autoDelayBar.SmallChange = 100;
            autoDelayBar.TickStyle = TickStyle.None;
            autoDelayBar.Value = Math.Min(autoDelayBar.Maximum, Math.Max(autoDelay, autoDelayBar.Minimum));
            settingsL.Controls.Add(autoDelayBar);
            autoDelayBar.ValueChanged += new EventHandler(autoDelayBar_ValueChanged);
            tt.SetToolTip(autoDelayBar, LRZ("AutoDelayTips"));

            autoDelayBox.Text = string.Format("{0}", autoDelayBar.Value);

            CheckBox enableRevise = new CheckBox();
            enableRevise.Text = LRZ("EnableReviseText");
            enableRevise.Checked = myReg.GetValue("EnableRevise") != null ? (string)myReg.GetValue("EnableRevise") == "1" : true;
            enableRevise.AutoSize = false;
            enableRevise.Margin = new Padding(0, 3, 0, 3);
            enableRevise.Anchor = AnchorStyles.Left|AnchorStyles.Right;
            settingsL.Controls.Add(enableRevise);

            CheckBox multiMode = new CheckBox();
            multiMode.Text = LRZ("MultiModeText");
            multiMode.Checked = myReg.GetValue("MultiMode") != null ? ((string)myReg.GetValue("MultiMode") == "1") : false;
            multiMode.AutoSize = false;
            multiMode.Margin = new Padding(0, 3, 0, 3);
            multiMode.Anchor = AnchorStyles.Left|AnchorStyles.Right;
            settingsL.Controls.Add(multiMode);
            settingsL.SetColumnSpan(multiMode, 2);

            bool preCropAS = myReg.GetValue("PreCropAtStart") != null ? (string)myReg.GetValue("PreCropAtStart") == "1" : true;
            CheckBox preCropAtStart = new CheckBox();
            preCropAtStart.Text = LRZ("PreCropAtStartText");
            preCropAtStart.Checked = preCropAS;
            preCropAtStart.AutoSize = false;
            preCropAtStart.Margin = new Padding(0, 3, 0, 3);
            preCropAtStart.Anchor = AnchorStyles.Left|AnchorStyles.Right;
            settingsL.Controls.Add(preCropAtStart);
            settingsL.SetColumnSpan(preCropAtStart, 3);

            label = new Label();
            label.Margin = new Padding(6, 6, 6, 6);
            label.Text = LRZ("Language");
            label.AutoSize = true;
            settingsL.Controls.Add(label);

            languageBox = new ComboBox();
            languageBox.DataSource = new string [] {"English", "简体中文"};
            languageBox.Tag = language;
            languageBox.DropDownStyle = ComboBoxStyle.DropDownList;
            languageBox.Dock = DockStyle.Fill;
            settingsL.Controls.Add(languageBox);
            settingsL.SetColumnSpan(languageBox, 2);

            FlowLayoutPanel panel = new FlowLayoutPanel();
            panel.FlowDirection = FlowDirection.RightToLeft;
            panel.AutoSize = true;
            panel.AutoSizeMode = AutoSizeMode.GrowAndShrink;
            panel.Anchor = AnchorStyles.None;
            settingsL.Controls.Add(panel);
            settingsL.SetColumnSpan(panel, 3);
            panel.Font = LRZFont(8);

            Button cancel = new Button();
            cancel.Text = LRZ("Cancel");
            cancel.DialogResult = DialogResult.Cancel;
            panel.Controls.Add(cancel);
            settingsForm.CancelButton = cancel;

            Button settingsOk = new Button();
            settingsOk.Text = LRZ("OK");
            settingsOk.DialogResult = DialogResult.OK;
            panel.Controls.Add(settingsOk);
            settingsForm.AcceptButton = settingsOk;

            DialogResult result = settingsForm.ShowDialog();
            if (DialogResult.OK == result)
            {
                myReg.SetValue("SafetyThreshold", Convert.ToString(safetyBar.Value));

                try
                {
                    int product = int.Parse(countXBox.Text) * int.Parse(countYBox.Text);
                    if (product > safetyThreshold && product <= safetyBar.Value)
                    {
                        string tmp = countXBox.Text;
                        countXBox.Text = "1";
                        countXBox.Text = tmp;
                    }
                }
                catch
                {

                }
                myReg.SetValue("DelayAuto", autoDelayBar.Value.ToString());
                myReg.SetValue("DelayGrid", gridDelayBar.Value.ToString());
                myReg.SetValue("DelayGridHide", gridHideDelayBar.Value.ToString());
                myReg.SetValue("EnableRevise", (enableRevise.Checked ? 1 : 0).ToString());
                myReg.SetValue("MultiMode", (multiMode.Checked ? 1 : 0).ToString());
                myReg.SetValue("PreCropAtStart", (preCropAtStart.Checked ? 1 : 0).ToString());

                if (languageBox.SelectedIndex != language)
                {
                    language = languageBox.SelectedIndex;
                    myReg.SetValue("Language", language.ToString());
                    form.Close();
                    SpriteSheetSetWindow();
                }

                else if ((preCropAtStart.Checked != preCropAS) && (preCropAtStart.Checked ^ isPreCrop))
                {
                    isPreCrop = !isPreCrop;
                    preCropSet();
                }
            }
        }

        private void settingsForm_Load(object sender, EventArgs e)
        {
            languageBox.SelectedIndex = (int)languageBox.Tag;
        }

        private void safetyBox_TextChanged(object sender, EventArgs e)
        {
            try
            {
                safetyBar.Value = Math.Min(safetyBar.Maximum, Math.Max(int.Parse(((TextBox)sender).Text), safetyBar.Minimum));
            }
            catch
            {

            }
        }

        private void safetyBar_ValueChanged(object sender, EventArgs e)
        {
            safetyBox.Text = string.Format("{0}", ((TrackBar)sender).Value);
        }

        private void gridDelayBox_TextChanged(object sender, EventArgs e)
        {
            try
            {
                gridDelayBar.Value = Math.Min(gridDelayBar.Maximum, Math.Max(int.Parse(((TextBox)sender).Text), gridDelayBar.Minimum));
            }
            catch
            {

            }
        }

        private void gridDelayBar_ValueChanged(object sender, EventArgs e)
        {
            gridDelayBox.Text = string.Format("{0}", ((TrackBar)sender).Value);
        }

        private void autoDelayBox_TextChanged(object sender, EventArgs e)
        {
            try
            {
                autoDelayBar.Value = Math.Min(autoDelayBar.Maximum, Math.Max(int.Parse(((TextBox)sender).Text), autoDelayBar.Minimum));
            }
            catch
            {

            }
        }

        private void autoDelayBar_ValueChanged(object sender, EventArgs e)
        {
            autoDelayBox.Text = string.Format("{0}", ((TrackBar)sender).Value);
        }

        private void resetCropBox_Click(object sender, EventArgs e)
        {
            resetCrop();
        }

        private void gridHideDelayBox_TextChanged(object sender, EventArgs e)
        {
            try
            {
                gridHideDelayBar.Value = int.Parse(((TextBox)sender).Text);
            }
            catch
            {

            }
        }

        private void gridHideDelayBar_ValueChanged(object sender, EventArgs e)
        {
            gridHideDelayBox.Text = string.Format("{0}", ((TrackBar)sender).Value);
        }

        private void resetCrop()
        {
            KeyframeReset(keyframePreview);
            spriteFrame = new int[] {(int) PointDistance(keyframePreview.TopLeft, keyframePreview.TopRight), (int) PointDistance(keyframePreview.TopLeft, keyframePreview.BottomLeft)};
            count = myReg.GetValue("Count") != null ? Array.ConvertAll(Regex.Split(Convert.ToString(myReg.GetValue("Count")), ","), int.Parse) : new int[] {8, 4};

            try
            {
                countXBox.Text = string.Format("{0}", count[0]);
                countYBox.Text = string.Format("{0}", count[1]);
                topLeftXBox.Text = string.Format("{0}", 1);
                topLeftYBox.Text = string.Format("{0}", 1);
                bottomRightXBox.Text = string.Format("{0}", count[0]);
                bottomRightYBox.Text = string.Format("{0}", count[1]);
                frameEndXBox.Text = string.Format("{0}", count[0]);
                frameEndYBox.Text = string.Format("{0}", count[1]);
                isPreCrop = true;
                countXBar.Value = count[0];
                countYBar.Value = count[1];
                refreshGrid(true);
            }
            catch
            {

            }
        }

        private void refreshGrid(bool needPreCropSet = false)
        {
            if (false == (bool) showGridButton.Tag)
            {
                return;
            }
            if (needPreCropSet)
            {
                form.Hide();
            }
            gridForm.Hide();
            gridForm.Opacity = (myReg.GetValue("GridOpacity") != null ? int.Parse((string)myReg.GetValue("GridOpacity")) : 40) / 100.0;
            preview = myReg.GetValue("GridPreview") != null ? Array.ConvertAll(Regex.Split(Convert.ToString(myReg.GetValue("GridPreview")), ","), int.Parse) : new int[] {480,270};

            double gridSizeFactor = Math.Min(1.0 * preview[0] / spriteFrame[0], 1.0 * preview[1] / spriteFrame[1]);
            gridForm.Size = new Size((int)(gridSizeFactor * spriteFrame[0] + 40), (int)(gridSizeFactor * spriteFrame[1] + 70));

            if (myReg.GetValue("GridCenter") != null)
            {
                int[] arr = Array.ConvertAll(Regex.Split(Convert.ToString(myReg.GetValue("GridCenter")), ","), int.Parse);
                gridForm.Location = new Point(Math.Max(0, Math.Min(arr[0] - gridForm.Size.Width / 2, Screen.GetWorkingArea(gridForm).Width * 4 / 5)), Math.Max(0, Math.Min(arr[1] - gridForm.Size.Height / 2, Screen.GetWorkingArea(gridForm).Height * 4 / 5)));
            }
            else
            {
                gridForm.StartPosition = FormStartPosition.CenterScreen;
            }

            try
            {
                gridForm.Controls.Remove(gridL);
            }
            catch
            {

            }
            gridL = new TableLayoutPanel();
            gridL.GrowStyle = TableLayoutPanelGrowStyle.AddRows;
            gridL.ColumnCount = count[0] + 2;
            gridL.RowCount = count[1] + 2;
            gridL.Dock = DockStyle.Fill;
            Label label = new Label();
            gridL.Controls.Add(label);
            gridL.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 15));
            gridL.RowStyles.Add(new RowStyle(SizeType.Absolute, 15));
            for (int i = 0; i < count[0]; i++)
            {
                gridL.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
                label = new Label();
                label.Text = Convert.ToString((i + 1) % 10);
                label.TextAlign = ContentAlignment.MiddleCenter;
                label.Dock = DockStyle.Fill;
                gridL.Controls.Add(label);
            }
            for (int i = 0; i < count[0] * count[1]; i++)
            {
                if (i % count[0] == 0)
                {
                    gridL.RowStyles.Add(new RowStyle(SizeType.Percent, 100));
                    label = new Label();
                    label.AutoSize = true;
                    gridL.Controls.Add(label);
                    label = new Label();
                    label.Text = Convert.ToString((i / count[0] + 1) % 10);
                    label.TextAlign = ContentAlignment.MiddleCenter;
                    label.Dock = DockStyle.Fill;
                    gridL.Controls.Add(label);
                }
                Button selectButton = new Button();
                selectButton.Tag = new int[] {i, ToIndex(i)};
                selectButton.Margin = new Padding(0, 0, 0, 0);
                selectButton.Dock = DockStyle.Fill;
                selectButton.MouseDown += new MouseEventHandler(selectButton_MouseDown);
                gridL.Controls.Add(selectButton);
            }
            gridForm.Controls.Add(gridL);
            myVegas.UpdateUI();
            gridForm.Show();
            SetForegroundWindow(Handle1);
            if (needPreCropSet)
            {
                Delay(1);
                preCropSet();
            }
        }

        private void refreshIndex_Handler(object sender, EventArgs e)
        {
            if (countSelected > 1)
            {
                refreshColor();
                try
                {
                    int start = (int.Parse(frameStartYBox.Text) - 1) * count[0] + int.Parse(frameStartXBox.Text) - 1;
                    int end = (int.Parse(frameEndYBox.Text) - 1) * count[0] + int.Parse(frameEndXBox.Text) - 1;
                    foreach (Control ctrl in gridL.Controls)
                    {
                        if (ctrl is Button)
                        {
                            int i = ((int[])ctrl.Tag)[0];
                            if (i == start || i == end)
                            {
                                ctrl.BackColor = Color.FromArgb(255,0,0);
                            }
                        }
                    }
                    ToIndexes(start, end);
                }
                catch
                {

                }
                countSelected = (countSelected + 3) / 2 * 2;
                refreshIndex();
                changeColor();
            }
        }

        private void refreshIndex()
        {
            foreach (Control ctrl in gridL.Controls)
            {
                if (ctrl is Button)
                {
                    try
                    {
                        ((int[])ctrl.Tag)[1] = ToIndex(((int[])ctrl.Tag)[0]);

                        // For Test Only
                        // ctrl.Text = Convert.ToString(((int[])ctrl.Tag)[1]);
                    }
                    catch
                    {

                    }
                }
            }
        }
        private void refreshColor()
        {
            foreach (Control ctrl in gridL.Controls)
            {
                if (ctrl is Button)
                {
                    ctrl.BackColor = backColor;
                }
            }
        }

        private void ToIndexes(int start, int end)
        {
            frameIndex[1] = frameIndex[0] + 1; // Make sure that frameIndex[1] is larger than frameIndex[0] to avoid problems
            frameIndex[0] = ToIndex(start, true);
            frameIndex[1] = frameIndex[0] + 1;
            frameIndex[1] = ToIndex(end);
            if (frameIndex[0] > frameIndex[1])
            {
                frameIndex[0] = frameIndex[1] + 1; // Make sure that frameIndex[0] is larger than frameIndex[1] to avoid problems
                frameIndex[1] = ToIndex(end, true);
                frameIndex[0] = frameIndex[1] + 1;
                frameIndex[0] = ToIndex(start);
            }
        }

        private int ToIndex(int i, bool isFrameIndex = false)
        {
            int r = i / count[0];
            int c = i % count[0];
            int index = ToIndex(r, c, isFrameIndex);
            return index;
        }

        private int ToIndex(int r, int c, bool isFrameIndex = false)
        {
            int directionIndex = directionBox.SelectedIndex;
            bool verticalRead = directionIndex == 2 || directionIndex == 3 || directionIndex == 6 || directionIndex == 7;
            bool backwardRead = directionIndex == 1 || directionIndex == 3 || directionIndex == 5 || directionIndex == 7;
            bool sshapedRead = directionIndex == 4 || directionIndex == 5 || directionIndex == 6 || directionIndex == 7;
            if (verticalRead) 
            {
                int a = r;
                r = c;
                c = a;
            }
            cut = new int[] {int.Parse(cutXBox.Text), int.Parse(cutYBox.Text)};
            int sum = count[0] / cut[0] * count[1] / cut[1];
            int cols = verticalRead ? count[1] / cut[1] : count[0] / cut[0];
            int colsCut = verticalRead ? cut[1] : cut[0];
            int rr = r / (sum / cols);
            int cc = c / cols;
            c = c % cols;
            if (sshapedRead && !isFrameIndex)
            {
                if (r % 2 != 0)
                {
                    c = cols - 1 - c;
                }
                int min = Math.Min(frameIndex[0], frameIndex[1]);
                if (min / cols / sum == rr && min / sum == cc && min / cols % (sum / cols) % 2 != 0)
                {
                    c = cols - 1 - c;
                }
            }
            if (backwardRead)
            {
                c = cols - 1 - c;
                cc = colsCut - 1 - cc;
            }

            int index = r * cols % sum + rr * sum * colsCut + c + cc * sum;
            return index;
        }

        private void showGrid(int countSelectedInput = 0)
        {
            countSelected = countSelectedInput;

            if (gridForm == null)
            {
                gridForm = new Form();
                gridForm.FormBorderStyle = FormBorderStyle.SizableToolWindow;
                gridForm.AutoSize = false;
                gridForm.BackColor = backColor;
                gridForm.ForeColor = foreColor;
                gridForm.Text = LRZ("GridTitle");
                gridForm.Font = LRZFont(9);
                gridForm.Owner = form;
                gridForm.ShowInTaskbar = false;
                gridForm.StartPosition = FormStartPosition.Manual;
                gridL = new TableLayoutPanel();
                showGridButton.Tag = true;
                gridForm.FormClosing += new FormClosingEventHandler(gridForm_FormClosing);
                refreshGrid();
                gridForm.LocationChanged += new EventHandler(gridForm_LocationChanged);
                showGridButton.Text = LRZ("HideGrid");

                gridFormSet = new Form();
                gridFormSet.StartPosition = FormStartPosition.CenterParent;
                gridFormSet.FormBorderStyle = FormBorderStyle.FixedToolWindow;
                gridFormSet.ShowInTaskbar = false;
                gridFormSet.Owner = form;
                gridFormSet.TopMost = true;
                gridFormSet.AutoSize = true;
                gridFormSet.BackColor = backColor;
                gridFormSet.ForeColor = foreColor;
                gridFormSet.AutoSizeMode = AutoSizeMode.GrowAndShrink;
                gridFormSet.Text = LRZ("GridDisplaySettings");
                gridFormSet.Font = LRZFont(9);

                Panel panelBig = new Panel();
                panelBig.AutoSize = true;
                panelBig.AutoSizeMode = AutoSizeMode.GrowAndShrink;
                gridFormSet.Controls.Add(panelBig);

                TableLayoutPanel gridSetL = new TableLayoutPanel();
                gridSetL.AutoSize = true;
                gridSetL.AutoSizeMode = AutoSizeMode.GrowAndShrink;
                gridSetL.GrowStyle = TableLayoutPanelGrowStyle.AddRows;
                gridSetL.ColumnCount = 4;
                gridSetL.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 105));
                gridSetL.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 125));
                gridSetL.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 45));
                gridSetL.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 45));
                panelBig.Controls.Add(gridSetL);

                Label label = new Label();
                label.Margin = new Padding(6, 6, 0, 6);
                label.Text = LRZ("GridOpacityText");
                label.AutoSize = true;
                gridSetL.Controls.Add(label);

                gridOpacityBar = new TrackBar();
                gridOpacityBar.AutoSize = false;
                
                gridOpacityBar.Margin = new Padding(0, 5, 0, 5);
                gridOpacityBar.Dock = DockStyle.Fill;
                gridOpacityBar.Minimum = 20;
                gridOpacityBar.Maximum = 80;
                gridOpacityBar.LargeChange = 10;
                gridOpacityBar.SmallChange = 1;
                gridOpacityBar.TickStyle = TickStyle.None;
                gridOpacityBar.Value = Math.Min(gridOpacityBar.Maximum, Math.Max(myReg.GetValue("GridOpacity") != null ? int.Parse((string)myReg.GetValue("GridOpacity")) : 40, gridOpacityBar.Minimum));
                gridSetL.Controls.Add(gridOpacityBar);
                gridSetL.SetColumnSpan(gridOpacityBar, 2);
                gridOpacityBar.ValueChanged += new EventHandler(gridOpacityBar_ValueChanged);

                gridOpacityBox = new TextBox();
                gridOpacityBox.AutoSize = true;
                gridOpacityBox.Text = string.Format("{0}", gridOpacityBar.Value);
                gridSetL.Controls.Add(gridOpacityBox);
                gridOpacityBox.TextChanged += new EventHandler(gridOpacityBox_TextChanged);
                gridOpacityBar.Height = gridOpacityBox.Height;

                label = new Label();
                label.Margin = new Padding(6, 6, 6, 6);
                label.Text = LRZ("PreviewText");
                label.AutoSize = true;
                gridSetL.Controls.Add(label);
                gridSetL.SetColumnSpan(label, 2);

                previewXBox = new TextBox();
                previewXBox.Text = string.Format("{0}", preview[0]);
                gridSetL.Controls.Add(previewXBox);

                previewYBox = new TextBox();
                previewYBox.Text = string.Format("{0}", preview[1]);
                gridSetL.Controls.Add(previewYBox);

                CheckBox autoMatch = new CheckBox();
                autoMatch.Text = LRZ("AutoMatchText");
                autoMatch.Checked = true;
                autoMatch.CheckedChanged += new EventHandler(autoMatch_CheckedChanged);
                autoMatch.AutoSize = false;
                autoMatch.Margin = new Padding(6, 0, 6, 0);
                autoMatch.Anchor = AnchorStyles.Left|AnchorStyles.Right;
                gridSetL.Controls.Add(autoMatch);
                gridSetL.SetColumnSpan(autoMatch, 4);

                if (autoMatch.Checked)
                {
                    previewXBox.TextChanged += new EventHandler(previewXBox_TextChanged);
                    previewYBox.TextChanged += new EventHandler(previewYBox_TextChanged);
                }

                FlowLayoutPanel panel = new FlowLayoutPanel();
                panel.FlowDirection = FlowDirection.RightToLeft;
                panel.AutoSize = true;
                panel.AutoSizeMode = AutoSizeMode.GrowAndShrink;
                panel.Anchor = AnchorStyles.None;
                gridSetL.Controls.Add(panel);
                gridSetL.SetColumnSpan(panel, 4);
                panel.Font = LRZFont(8);

                Button cancel = new Button();
                cancel.Text = LRZ("Cancel");
                cancel.DialogResult = DialogResult.Cancel;
                panel.Controls.Add(cancel);
                gridFormSet.CancelButton = cancel;

                Button gridOk = new Button();
                gridOk.Text = LRZ("OK");
                gridOk.DialogResult = DialogResult.OK;
                panel.Controls.Add(gridOk);
                gridFormSet.AcceptButton = gridOk;
            }
            else if (gridForm.Visible == false)
            {
                gridForm.Owner = form;
                showGridButton.Tag = true;
                refreshGrid();
                SetForegroundWindow(gridForm.Handle);
                showGridButton.Text = LRZ("HideGrid");
            }
            else
            {
                showGridButton.Tag = false;
                gridForm.Hide();
                showGridButton.Text = LRZ("ShowGrid");
            }
        }

        private void autoMatch_CheckedChanged(object sender, EventArgs e)
        {
            CheckBox checkBox = (CheckBox) sender;
            if (checkBox.Checked)
            {
                previewXBox.TextChanged += new EventHandler(previewXBox_TextChanged);
                previewYBox.TextChanged += new EventHandler(previewYBox_TextChanged);
            }
            else
            {
                previewXBox.TextChanged -= new EventHandler(previewXBox_TextChanged);
                previewYBox.TextChanged -= new EventHandler(previewYBox_TextChanged);
            }
        }

        private void previewXBox_TextChanged(object sender, EventArgs e)
        {
            if (((TextBox)sender).Focused)
            {
                try
                {
                    previewYBox.Text = string.Format("{0}", Math.Round(int.Parse(previewXBox.Text) * scrHeight / scrWidth));
                }
                catch
                {
                    
                }
            }
        }

        private void previewYBox_TextChanged(object sender, EventArgs e)
        {
            if (((TextBox)sender).Focused)
            {
                try
                {
                    previewXBox.Text = string.Format("{0}", Math.Round(int.Parse(previewYBox.Text) * scrWidth / scrHeight));
                }
                catch
                {
                    
                }
            }
        }

        private void gridOpacityBox_TextChanged(object sender, EventArgs e)
        {
            try
            {
                gridOpacityBar.Value = int.Parse(((TextBox)sender).Text);
            }
            catch
            {

            }
        }

        private void gridOpacityBar_ValueChanged(object sender, EventArgs e)
        {
            gridOpacityBox.Text = string.Format("{0}", ((TrackBar)sender).Value);
        }

        private void gridForm_LocationChanged(object sender, EventArgs e)
        {
            myReg.SetValue("GridCenter", (gridForm.Location.X + gridForm.Size.Width / 2) + "," + (gridForm.Location.Y + gridForm.Size.Height / 2));
        }

        private void gridForm_FormClosing(object sender, EventArgs e)
        {
            showGridButton.Tag = false;
            gridForm = null;
            showGridButton.Text = LRZ("ShowGrid");
        }

        private void selectButton_MouseDown(object sender, MouseEventArgs e)
        {
            Button button = (Button) sender;
            int i = ((int[]) button.Tag)[0], countSelectedSaved = countSelected, a = countSelectedSaved % 2;
            if (e.Button == MouseButtons.Right)
            {
                foreach (Control ctrl in gridL.Controls)
                {
                    if (ctrl is Button && ctrl.BackColor != backColor)
                    {
                        refreshColor();
                        countSelected = (countSelectedSaved + 3) / 2 * 2;
                        return;
                    }
                }
                if (countSelectedSaved % 2 == 0)
                {
                    if (isPreCrop)
                    {
                        DialogResult result = gridFormSet.ShowDialog();
                        if (DialogResult.OK == result)
                        {
                            myReg.SetValue("GridOpacity", gridOpacityBar.Value.ToString());
                            int tmp1, tmp2;
                            if (int.TryParse(previewXBox.Text, out tmp1) && int.TryParse(previewYBox.Text, out tmp2))
                            {
                                myReg.SetValue("GridPreview", tmp1 + "," + tmp2);
                            }
                            else
                            {
                                return;
                            }
                            refreshGrid();
                        }
                    }
                    else
                    {
                        resetCrop();
                    }
                }
                return;
            }

            if (isPreCrop && ((cropModeBox.SelectedIndex == 1 && a == 1) || cropModeBox.SelectedIndex == 2))
            {
                refreshColor();
                button.BackColor = Color.FromArgb(0,255,0);
                if (cropModeBox.SelectedIndex == 2)
                {
                    topLeftXBox.Text = string.Format("{0}", i % count[0] + 1);
                    topLeftYBox.Text = string.Format("{0}", i / count[0] + 1);
                    bottomRightXBox.Text = string.Format("{0}", i % count[0] + 1);
                    bottomRightYBox.Text = string.Format("{0}", i / count[0] + 1);
                }
                else
                {
                    bottomRightXBox.Text = string.Format("{0}", Math.Max(i % count[0] + 1, int.Parse(topLeftXBox.Text)));
                    bottomRightYBox.Text = string.Format("{0}", Math.Max(i / count[0] + 1, int.Parse(topLeftYBox.Text)));
                    topLeftXBox.Text = string.Format("{0}", Math.Min(i % count[0] + 1, int.Parse(topLeftXBox.Text)));
                    topLeftYBox.Text = string.Format("{0}", Math.Min(i / count[0] + 1, int.Parse(topLeftYBox.Text)));
                }
                if (gridForm != null)
                {
                    gridForm.Hide();
                }
                preCropOk(true);
                spriteOk();
            }
            countSelected = countSelectedSaved + 1;

            switch (a)
            {
                case 0:
                {
                    refreshColor();
                    button.BackColor = Color.FromArgb(255,0,0);
                    if (isPreCrop)
                    {
                        topLeftXBox.Text = string.Format("{0}", i % count[0] + 1);
                        topLeftYBox.Text = string.Format("{0}", i / count[0] + 1);
                    }
                    else
                    {
                        frameStartXBox.Text = string.Format("{0}", i % count[0] + 1);
                        frameStartYBox.Text = string.Format("{0}", i / count[0] + 1);
                    }
                    break;
                }
                case 1:
                {
                    button.BackColor = Color.FromArgb(255,0,0);
                    if (isPreCrop)
                    {
                        bottomRightXBox.Text = string.Format("{0}", Math.Max(i % count[0] + 1, int.Parse(topLeftXBox.Text)));
                        bottomRightYBox.Text = string.Format("{0}", Math.Max(i / count[0] + 1, int.Parse(topLeftYBox.Text)));
                        topLeftXBox.Text = string.Format("{0}", Math.Min(i % count[0] + 1, int.Parse(topLeftXBox.Text)));
                        topLeftYBox.Text = string.Format("{0}", Math.Min(i / count[0] + 1, int.Parse(topLeftYBox.Text)));
                    }
                    else
                    {
                        frameEndXBox.Text = string.Format("{0}", i % count[0] + 1);
                        frameEndYBox.Text = string.Format("{0}", i / count[0] + 1);
                        int start = (int.Parse(frameStartYBox.Text) - 1) * count[0] + int.Parse(frameStartXBox.Text) - 1;
                        int end = (int.Parse(frameEndYBox.Text) - 1) * count[0] + int.Parse(frameEndXBox.Text) - 1;
                        ToIndexes(start, end);
                        refreshIndex();
                    }

                    changeColor();
                    if (countSelected == countSelectedSaved + 1)
                    {
                        if (isPreCrop)
                        {
                            preCropOk();
                        }
                        else if (myReg.GetValue("AutoMode") != null ? ((string)myReg.GetValue("AutoMode") == "1") : true)
                        {
                            spriteOk();
                        }
                    }
                    break;
                }
            }
        }

        private void changeColor(bool shouldDelay = true)
        {
            int autoDelay = myReg.GetValue("DelayAuto") != null ? int.Parse((string)myReg.GetValue("DelayAuto")) : 1000;

            if (gridForm == null)
            {
                showGrid(countSelected);
            }

            SetForegroundWindow(gridForm.Handle);

            if (isPreCrop)
            {
                foreach (Control ctrl in gridL.Controls)
                {
                    if (ctrl is Button)
                    {
                        int i = ((int[]) ctrl.Tag)[0];
                        if (i % count[0] + 2 > int.Parse(topLeftXBox.Text) && i % count[0] < int.Parse(bottomRightXBox.Text) && i / count[0] + 2 > int.Parse(topLeftYBox.Text) && i / count[0] < int.Parse(bottomRightYBox.Text))
                        {
                            ctrl.BackColor = Color.FromArgb(0,255,0);
                        }
                    }
                }
                Delay(1000);
            }
            else
            {
                offset = new int[] {int.Parse(startOffsetBox.Text), int.Parse(loopOffsetBox.Text)};
                spritesArr = new ArrayList();
                int gridDelay = myReg.GetValue("DelayGrid") != null ? int.Parse((string)myReg.GetValue("DelayGrid")) : 35;
                if (!changeColor(shouldDelay ? gridDelay : 0))
                {
                    return;
                }
                if (playbackBox.SelectedIndex > 0)
                {
                    Delay(gridDelay * 5);
                    if (!changeColor(shouldDelay ? gridDelay : 0, true))
                    {
                        return;
                    }
                }
                if (shouldDelay)
                {
                    Delay(autoDelay);
                }
            }
        }

        private bool changeColor(int delay, bool reverse = false)
        {
            int countSelectedSaved = countSelected;
            int a = frameIndex[0], b = frameIndex[1];
            if (a > b)
            {
                int c = a;
                a = b;
                b = c;
            }
            for (int j = a; j <= b; j++)
            {
                if (countSelected != countSelectedSaved)
                {
                    return false;
                }
                if (((playbackBox.SelectedIndex == 2 && !reverse) || playbackBox.SelectedIndex == 3) && (j == b))
                {
                    continue;
                }
                foreach (Control ctrl in gridL.Controls)
                {
                    if (ctrl is Button)
                    {
                        int i = ((int[]) ctrl.Tag)[1];
                        int jj = frameIndex[0] > frameIndex[1] ? (a + b - j) : j;
                        jj = reverse ? (a + b - jj) : jj;
                        jj = (jj - a + offset[1]) % (b - a + 1) + a;

                        if (jj == i)
                        {
                            spritesArr.Add(((int[]) ctrl.Tag)[0]);
                            ctrl.BackColor = reverse ? Color.FromArgb(255,165,0) : Color.FromArgb(0,255,0);
                            Delay(delay);
                            break;
                        }
                    }
                }
            }
            return true;
        }

        private void renderAs_Click(object sender, EventArgs e)
        {
            Form renderAsForm = new Form();
            renderAsForm.SuspendLayout();
            renderAsForm.MaximizeBox = false;
            renderAsForm.MinimizeBox = false;
            renderAsForm.HelpButton = false;
            renderAsForm.ShowInTaskbar = false;
            renderAsForm.AutoSize = true;
            renderAsForm.BackColor = backColor;
            renderAsForm.ForeColor = foreColor;
            renderAsForm.Owner = form;
            renderAsForm.TopMost = true;
            renderAsForm.StartPosition = FormStartPosition.CenterScreen;
            renderAsForm.FormBorderStyle = FormBorderStyle.FixedToolWindow;
            renderAsForm.AutoSizeMode = AutoSizeMode.GrowAndShrink;
            renderAsForm.Text = LRZ("RenderAsSettings");
            renderAsForm.Font = LRZFont(9);
            renderAsForm.Load += new EventHandler(renderAsForm_Load);

            Panel panelBig = new Panel();
            panelBig.AutoSize = true;
            panelBig.AutoSizeMode = AutoSizeMode.GrowAndShrink;
            renderAsForm.Controls.Add(panelBig);

            TableLayoutPanel renderAsL = new TableLayoutPanel();
            renderAsL.AutoSize = true;
            renderAsL.AutoSizeMode = AutoSizeMode.GrowAndShrink;
            renderAsL.GrowStyle = TableLayoutPanelGrowStyle.AddRows;
            renderAsL.ColumnCount = 2;
            renderAsL.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 80));
            renderAsL.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 135));
            panelBig.Controls.Add(renderAsL);

            ToolTip tt = new ToolTip();

            int renderCount = 4;
            int[] render = new int[] {0, 1, 0, 0, 0};
            if (myReg.GetValue("Render") != null)
            {
                int[] arr = Array.ConvertAll(Regex.Split(Convert.ToString(myReg.GetValue("Render")), ","), int.Parse);
                for (int i = 0; i < render.Length && i < arr.Length; i++)
                {
                    render[i] = arr[i];
                }
            }

            string [] choicesText = LRZArr("RenderAsChoices");
            for (int i = 0; i < renderCount; i++)
            {
                CheckBox renderAsChoices = new CheckBox();
                renderAsChoices.Text = choicesText[i];
                renderAsChoices.Tag = i;
                renderAsChoices.AutoSize = true;
                renderAsChoices.Checked = (render[i + 1] > 0) ? true : false;
                renderAsChoices.Margin = new Padding(6, 2, 6, 2);
                renderAsChoices.Anchor = AnchorStyles.Left|AnchorStyles.Right;
                renderAsL.Controls.Add(renderAsChoices);
                renderAsL.SetColumnSpan(renderAsChoices, 2);
                renderAsChoices.CheckedChanged += new EventHandler(renderAsChoices_CheckedChanged);
            }

            Label label = new Label();
            label.Margin = new Padding(6, 6, 6, 6);
            label.Text = LRZ("Reimport");
            label.Dock = DockStyle.Fill;
            renderAsL.Controls.Add(label);

            reimportBox = new ComboBox();
            reimportBox.DataSource = new string [] {".png", ".gif", ".mov (PNG)", ".mov (ProRes)"};
            reimportBox.Tag = render[0];
            reimportBox.Dock = DockStyle.Fill;
            reimportBox.DropDownStyle = ComboBoxStyle.DropDownList;
            renderAsL.Controls.Add(reimportBox);
            tt.SetToolTip(reimportBox, LRZ("Reimport"));
            reimportBox.SelectedIndexChanged += new EventHandler(reimportBox_SelectedIndexChanged);

            CheckBox displayInFolder = new CheckBox();
            displayInFolder.Text = LRZ("DisplayInFolderText");
            displayInFolder.Tag = 114514;
            displayInFolder.AutoSize = true;
            displayInFolder.Checked = myReg.GetValue("DisplayInFolder") != null ? ((string)myReg.GetValue("DisplayInFolder") == "1") : false;
            displayInFolder.Margin = new Padding(6, 6, 6, 6);
            displayInFolder.Anchor = AnchorStyles.Left|AnchorStyles.Right;
            renderAsL.Controls.Add(displayInFolder);
            renderAsL.SetColumnSpan(displayInFolder, 2);

            FlowLayoutPanel panel = new FlowLayoutPanel();
            panel.FlowDirection = FlowDirection.RightToLeft;
            panel.AutoSize = true;
            panel.AutoSizeMode = AutoSizeMode.GrowAndShrink;
            panel.Anchor = AnchorStyles.None;
            renderAsL.Controls.Add(panel);
            renderAsL.SetColumnSpan(panel, 2);
            panel.Font = LRZFont(8);

            Button cancel = new Button();
            cancel.Text = LRZ("Cancel");
            cancel.DialogResult = DialogResult.Cancel;
            panel.Controls.Add(cancel);
            renderAsForm.CancelButton = cancel;

            Button renderAsOk = new Button();
            renderAsOk.Text = LRZ("OK");
            renderAsOk.DialogResult = DialogResult.OK;
            panel.Controls.Add(renderAsOk);
            renderAsForm.AcceptButton = renderAsOk;

            renderAsForm.ResumeLayout();
            DialogResult result = renderAsForm.ShowDialog();
            if (DialogResult.OK == result)
            {
                myReg.SetValue("DisplayInFolder", (displayInFolder.Checked ? 1 : 0).ToString());
                render[0] = reimportBox.SelectedIndex;
                for (int i = 1; i < render.Length; i++)
                {
                    foreach (Control ctrl in renderAsL.Controls)
                    {
                        if (ctrl is CheckBox && i == (int)ctrl.Tag + 1)
                        {
                            render[i] = ((CheckBox)ctrl).Checked ? 1 : 0;
                        }
                    }
                }
                myReg.SetValue("Render", string.Join(",", render));
            }
        }

        private void renderAsForm_Load(object sender, EventArgs e)
        {
            reimportBox.SelectedIndex = (int)reimportBox.Tag;
        }

        private void renderAsChoices_CheckedChanged(object sender, EventArgs e)
        {
            CheckBox ckb = (CheckBox)sender;

            if (reimportBox.SelectedIndex == (int)ckb.Tag && !ckb.Checked)
            {
                foreach (Control ctrl in ckb.Parent.Controls)
                {
                    if (ctrl is CheckBox)
                    {
                        CheckBox ckb0 = (CheckBox)ctrl;
                        if ((int)ckb0.Tag <= 3 && ckb0.Checked)
                        {
                            reimportBox.SelectedIndex = (int)ckb0.Tag;
                            return;
                        }
                    }
                }

                foreach (Control ctrl in ckb.Parent.Controls)
                {
                    if (ctrl is CheckBox)
                    {
                        CheckBox ckb0 = (CheckBox)ctrl;
                        if ((int)ckb0.Tag == 0)
                        {
                            ckb0.Checked = true;
                            reimportBox.SelectedIndex = 0;
                            return;
                        }
                    }
                }
            }
        }

        private void reimportBox_SelectedIndexChanged(object sender, EventArgs e)
        {
            ComboBox cbb = (ComboBox)sender;
            foreach (Control ctrl in cbb.Parent.Controls)
            {
                if (ctrl is CheckBox)
                {
                    CheckBox ckb = (CheckBox)ctrl;
                    if (cbb.SelectedIndex == (int)ckb.Tag)
                    {
                        ckb.Checked = true;
                    }
                }
            }
        }

        // Multilanguage Support
        // If you can translate this script into other languages, please contact me.
        public string LRZ(string str)
        {
            string str0 = null;
            switch(language)
            {
                case 1:
                    switch(str)
                    {
                        case "ResetCropCaution":
                            str0 = "检测到当前图像已被裁切，是否重置裁切图像？";
                            break;

                        case "ResetCropCautionCaption":
                            str0 = "重置裁切提示";
                            break;

                        case "FileNotExistError":
                            str0 = "当前操作过程中发生错误。\n\n文件不存在，请确认文件路径是否合法。";
                            break;

                        case "FileNotExistErrorDetails":
                            str0 = "文件 {0} 不存在。";
                            break;

                        case "FormTitle":
                            str0 = "SpriteSheet 工具 v.1.2.0";
                            break;

                        case "CountText":
                            str0 = "行个数 和 列个数";
                            break;

                        case "CountTips":
                            str0 = "该 SpriteSheet 的 行个数 和 列个数 。注意顺序不能填反。";
                            break;

                        case "FrameStartText":
                            str0 = "开始帧的 行列 坐标";
                            break;

                        case "FrameStartTips":
                            str0 = "开始的第一帧的 行 和 列 坐标。";
                            break;

                        case "FrameEndText":
                            str0 = "结束帧的 行列 坐标";
                            break;

                        case "FrameEndTips":
                            str0 = "结束的最后一帧的 行 和 列 坐标。";
                            break;

                        case "TopLeftText":
                            str0 = "裁切左上 行列 坐标";
                            break;

                        case "TopLeftTips":
                            str0 = "裁切矩形左上角的 行 和 列 坐标。";
                            break;

                        case "BottomRightText":
                            str0 = "裁切右下 行列 坐标";
                            break;

                        case "BottomRightTips":
                            str0 = "裁切矩形右下角的 行 和 列 坐标。";
                            break;

                        case "FrameRateText":
                            str0 = "帧率";
                            break;

                        case "FrameRateTips":
                            str0 = "帧率。";
                            break;

                        case "OffsetText":
                            str0 = "起始帧&&循环内偏移";
                            break;

                        case "OffsetTips":
                            str0 = "起始帧偏移：播放时的总体的起始帧偏移量。\n循环内偏移：循环内偏移：单个循环内的帧偏移量。\n两者的功能稍微有些不同，注意区别。";
                            break;

                        case "DirectionText":
                            str0 = "读取方向";
                            break;

                        case "DirectionTips":
                            str0 = "Sprite 表的读取方向。";
                            break;

                        case "PlaybackText":
                            str0 = "播放模式";
                            break;

                        case "PlaybackTips":
                            str0 = "Sprite 表的播放模式。";
                            break;

                        case "RepeatText":
                            str0 = "重复范围";
                            break;

                        case "RepeatTips":
                            str0 = "重复范围中首帧和末帧的编号。";
                            break;

                        case "RepeatCountText":
                            str0 = "重复次数";
                            break;

                        case "RepeatCountTips":
                            str0 = "需要重复的次数。0 表示不重复。";
                            break;

                        case "CutText":
                            str0 = "切割（竖切×横切）";
                            break;

                        case "CutTips":
                            str0 = "将 Sprite 表先切割成若干小块，按照顺序读取。\n单个小块读取完毕后再读取下一小块。\n默认为1×1，即不切割。";
                            break;

                        case "CropModeText":
                            str0 = "裁切模式";
                            break;

                        case "CropModeTips":
                            str0 = "正常：正常进行预裁切操作。\n仅裁切：裁切图像后，直接渲染并导入进 Vegas。仅需点击网格两次。\n单帧裁切：“仅裁切”的升级版，用于裁切单帧。仅需点击网格一次。";
                            break;

                        case "EnableLoopText":
                            str0 = "是否启用事件循环";
                            break;

                        case "AutoModeText":
                            str0 = "自动模式";
                            break;

                        case "Cancel":
                            str0 = "取消";
                            break;

                        case "OK":
                            str0 = "确定";
                            break;

                        case "ResetCrop":
                            str0 = "重置裁切";
                            break;

                        case "ShowGrid":
                            str0 = "显示网格";
                            break;

                        case "HideGrid":
                            str0 = "隐藏网格";
                            break;

                        case "RenderAs":
                            str0 = "渲染设置";
                            break;

                        case "PreCrop":
                            str0 = "预裁切";
                            break;

                        case "Back":
                            str0 = "返回";
                            break;

                        case "GridTitle":
                            str0 = "可视化网格选择工具";
                            break;

                        case "GridDisplaySettings":
                            str0 = "网格显示设置";
                            break;

                        case "GridOpacityText":
                            str0 = "网格不透明度";
                            break;

                        case "PreviewText":
                            str0 = "预览窗口显示大小（宽×高）";
                            break;

                        case "AutoMatchText":
                            str0 = "修改值时自动匹配项目比例";
                            break;

                        case "RenderAsSettings":
                            str0 = "渲染 设置";
                            break;

                        case "Reimport":
                            str0 = "重新导入";
                            break;

                        case "DisplayInFolderText":
                            str0 = "渲染后在文件夹中显示";
                            break;

                        case "Settings":
                            str0 = "设置";
                            break;

                        case "SafetyText":
                            str0 = "安全阈值";
                            break;

                        case "SafetyTips":
                            str0 = "用于设置 行×列 的最大值。当 行×列 的值超过安全阈值时，不进行网格刷新，\n以避免不小心输错行/列时，网格生成过多，直接造成软件卡死的情况。";
                            break;

                        case "GridDelayText":
                            str0 = "网格延迟";
                            break;

                        case "GridDelayTips":
                            str0 = "用于设置 网格涂色 的延迟时间，以毫秒作为单位。\n此值不会影响预裁切的涂色，预裁切涂色是瞬时完成的。";
                            break;

                        case "GridHideDelayText":
                            str0 = "网格隐藏延迟";
                            break;

                        case "GridHideDelayTips":
                            str0 = "用于设置 点击旁边时自动隐藏网格 的延迟时间，以毫秒作为单位。\n点击“隐藏网格”按钮后，不会计算延迟，此时仅当重新点击按钮或重新点击旁边时，才会恢复网格的显示。";
                            break;

                        case "AutoDelayText":
                            str0 = "自动模式延迟";
                            break;

                        case "AutoDelayTips":
                            str0 = "用于设置 自动模式 的延迟时间，以毫秒作为单位。\n此值不会影响预裁切时的自动跳转延迟。";
                            break;

                        case "ScaleFactorText":
                            str0 = "缩放倍率";
                            break;

                        case "ScaleFactorTips":
                            str0 = "用于设置 渲染出来的文件相较于原素材 的缩放倍率。\n“自动”表示缩放至当前工程大小。";
                            break;

                        case "Auto":
                            str0 = "自动";
                            break;

                        case "EnableReviseText":
                            str0 = "启用修改";
                            break;

                        case "MultiModeText":
                            str0 = "启用多事件模式";
                            break;

                        case "PreCropAtStartText":
                            str0 = "启动时进入预裁切窗口";
                            break;

                        case "Language":
                            str0 = "界面语言";
                            break;

                        case "Rendering":
                            str0 = "渲染中";
                            break;
                    }
                    break;

                default:
                    switch(str)
                    {
                        case "ResetCropCaution":
                            str0 = "Detected that the current image has been cropped. Do you want to reset the cropped image?";
                            break;

                        case "ResetCropCautionCaption":
                            str0 = "Crop Resetting Caution";
                            break;

                        case "FileNotExistError":
                            str0 = "An error occurred during the current operation.\n\nFile does not exist, please make sure the file path is valid.";
                            break;

                        case "FileNotExistErrorDetails":
                            str0 = "File {0} does not exist.";
                            break;

                        case "FormTitle":
                            str0 = "SpriteSheetTool v.1.2.0";
                            break;

                        case "CountText":
                            str0 = "Rows && Columns";
                            break;

                        case "CountTips":
                            str0 = "The number counts of rows and columns of the SpriteSheet. Be careful NOT to reverse the order.";
                            break;

                        case "FrameStartText":
                            str0 = "Start Frame Coord";
                            break;

                        case "FrameStartTips":
                            str0 = "The Row & Column coordinate of the start frame.";
                            break;

                        case "FrameEndText":
                            str0 = "End Frame Coord";
                            break;

                        case "FrameEndTips":
                            str0 = "The Row & Column coordinate of the end frame.";
                            break;

                        case "TopLeftText":
                            str0 = "PreCrop TL Coord";
                            break;

                        case "TopLeftTips":
                            str0 = "The Row & Column coordinate for the Top-Left corner of the PreCrop rectangle.";
                            break;

                        case "BottomRightText":
                            str0 = "PreCrop BR Coord";
                            break;

                        case "BottomRightTips":
                            str0 = "The Row & Column coordinate for the Bottom-Right corner of the PreCrop rectangle.";
                            break;

                        case "FrameRateText":
                            str0 = "Frame Rate";
                            break;

                        case "FrameRateTips":
                            str0 = "The frame rate.";
                            break;

                        case "OffsetText":
                            str0 = "Start / Loop Offset";
                            break;

                        case "OffsetTips":
                            str0 = "Start Offset: the start frame offset as a whole at playback time.\nLoop Offset: the start frame offset in a single loop.\nThe function of the two is slightly different, pay attention to the difference.";
                            break;

                        case "DirectionText":
                            str0 = "Reading Direction";
                            break;

                        case "DirectionTips":
                            str0 = "The reading direction of the sprites.";
                            break;

                        case "PlaybackText":
                            str0 = "Playback Mode";
                            break;

                        case "PlaybackTips":
                            str0 = "The playback mode when playing the sprites.";
                            break;

                        case "RepeatText":
                            str0 = "Repeat Range";
                            break;

                        case "RepeatTips":
                            str0 = "The indexes of the first and last frame in the repeating range.";
                            break;

                        case "RepeatCountText":
                            str0 = "Repeat Count";
                            break;

                        case "RepeatCountTips":
                            str0 = "The number of repetitions required. 0 means no repetition.";
                            break;

                        case "CutText":
                            str0 = "Cut (V. x H.)";
                            break;

                        case "CropModeText":
                            str0 = "Crop Mode";
                            break;

                        case "CropModeTips":
                            str0 = "Normal: PreCrop the image Normally.\nCrop Only: Crop the image, then directly render and import it into Vegas. You just need to click on the grid twice.\nCrop Single: An upgraded version of Crop Only, used for single frames. You just need to click on the grid once.";
                            break;

                        case "EnableLoopText":
                            str0 = "Enable Event Loop";
                            break;

                        case "CutTips":
                            str0 = "Cut the SpriteSheet into several small blocks first and read them in sequence.\nThe next block is NOT read until the current block has been read.\n1 x 1, the default value, means no cutting.";
                            break;

                        case "AutoModeText":
                            str0 = "Auto Mode";
                            break;

                        case "Cancel":
                            str0 = "Cancel";
                            break;

                        case "OK":
                            str0 = "OK";
                            break;

                        case "ResetCrop":
                            str0 = "ResetCrop";
                            break;

                        case "ShowGrid":
                            str0 = "ShowGrid";
                            break;

                        case "HideGrid":
                            str0 = "HideGrid";
                            break;

                        case "RenderAs":
                            str0 = "RenderAs";
                            break;

                        case "PreCrop":
                            str0 = "PreCrop";
                            break;

                        case "Back":
                            str0 = "Back";
                            break;

                        case "GridTitle":
                            str0 = "Visual Grid Select Tool";
                            break;

                        case "GridDisplaySettings":
                            str0 = "Grid Display Settings";
                            break;

                        case "GridOpacityText":
                            str0 = "Grid Opacity";
                            break;

                        case "PreviewText":
                            str0 = "Preview Window Size (W. x H.)";
                            break;

                        case "AutoMatchText":
                            str0 = "Auto Match Project Aspect";
                            break;

                        case "RenderAsSettings":
                            str0 = "RenderAs Settings";
                            break;

                        case "Reimport":
                            str0 = "Reimport";
                            break;

                        case "DisplayInFolderText":
                            str0 = "Display In Folder";
                            break;

                        case "Settings":
                            str0 = "Settings";
                            break;

                        case "SafetyText":
                            str0 = "Safety Threshold";
                            break;

                        case "SafetyTips":
                            str0 = "The maximum value of rows x columns. When the value of the R. x C. exceeds the safety threshold, NO grid refresh is performed,\nto avoid excessive grid generation when you accidentally input the wrong row/column, which directly causes software deadlock.";
                            break;

                        case "GridDelayText":
                            str0 = "Grid Delay";
                            break;

                        case "GridDelayTips":
                            str0 = "The delay time for grid coloring, in milliseconds.\nThis value will NOT affect the PreCrop coloring, which is done instantaneously.";
                            break;

                        case "GridHideDelayText":
                            str0 = "Grid Hide Delay";
                            break;

                        case "GridHideDelayTips":
                            str0 = "The delay time for automatically hiding the grid when you click the side position, in milliseconds.\n\nAfter clicking the HideGrid button, this value will NOT be used, and the display of the grid will be\nrestored only when the button or the side position is re-clicked.";
                            break;

                        case "AutoDelayText":
                            str0 = "Auto Mode Delay";
                            break;

                        case "AutoDelayTips":
                            str0 = "The delay time for Auto Mode, in milliseconds.\nThis value doesn't affect the delay during PreCropping.";
                            break;

                        case "ScaleFactorText":
                            str0 = "Scale Factor";
                            break;

                        case "ScaleFactorTips":
                            str0 = "The scale factor of the rendered file relative to the source material.\nAuto: Scale to the current project size.";
                            break;

                        case "Auto":
                            str0 = "Auto";
                            break;

                        case "EnableReviseText":
                            str0 = "Enable Revise";
                            break;

                        case "MultiModeText":
                            str0 = "Multi-Event Mode";
                            break;

                        case "PreCropAtStartText":
                            str0 = "Enter PreCrop Window At Start";
                            break;

                        case "Language":
                            str0 = "UI Language";
                            break;

                        case "Rendering":
                            str0 = "Rendering";
                            break;
                    }
                    break;
            }
            return str0;
        }

        public string [] LRZArr(string str)
        {
            string [] str0 = null;
            switch(language)
            {
                case 1:
                    switch(str)
                    {
                        case "DirectionChoices":
                            str0 = new string [] {"水平正向", "水平反向", "竖直正向", "竖直反向", "水平正向(S型)", "水平反向(S型)", "竖直正向(S型)", "竖直反向(S型)"};
                            break;

                        case "PlaybackChoices":
                            str0 = new string [] {"正常", "正常+倒放", "正+倒 (半合并)" ,"正+倒 (全合并)"};
                            break;

                        case "RenderAsChoices":
                            str0 = new string [] {".png 图像序列", ".gif 图片", ".mov 格式 (PNG 编码)", ".mov 格式 (ProRes 编码)"};
                            break;

                        case "CropModeChoices":
                            str0 = new string [] {"正常", "仅裁切", "单帧裁切"};
                            break;
                    }
                    break;

                default:
                    switch(str)
                    {
                        case "DirectionChoices":
                            str0 = new string [] {"H. Forward", "H. Backward", "V. Forward", "V. Backward", "H. Forward   (S)", "H. Backward (S)", "V. Forward   (S)", "V. Backward (S)"};
                            break;

                        case "PlaybackChoices":
                            str0 = new string [] {"Normal", "Normal+Reverse", "N.+R.(Half Merge)" , "N.+R.(Full Merge)"};
                            break;

                        case "RenderAsChoices":
                            str0 = new string [] {".png image sequence", ".gif", ".mov (PNG coding)", ".mov (ProRes coding)"};
                            break;

                        case "CropModeChoices":
                            str0 = new string [] {"Normal", "Crop Only", "Crop Single"};
                            break;
                    }
                    break;
            }
            return str0;
        }

        public Font LRZFont(float fontSize)
        {
            Font font = new Font("Arial", fontSize);
            switch(language)
            {
                case 1:
                    font = new Font("Microsoft Yahei UI", fontSize);
                    break;
            }
            return font;
        }

        [DllImport("shell32.dll", ExactSpelling = true)]
        private static extern void ILFree(IntPtr pidlList);

        [DllImport("shell32.dll", CharSet = CharSet.Unicode, ExactSpelling = true)]
        private static extern IntPtr ILCreateFromPathW(string pszPath);

        [DllImport("shell32.dll", ExactSpelling = true)]
        private static extern int SHOpenFolderAndSelectItems(IntPtr pidlList, uint cild, IntPtr children, uint dwFlags);

        public static void ExplorerFile(string filePath)
        {
            if (!File.Exists(filePath) && !Directory.Exists(filePath))
                return;

            if (Directory.Exists(filePath))
                Process.Start(@"explorer.exe", "/select,\"" + filePath + "\"");
            else
            {
                IntPtr pidlList = ILCreateFromPathW(filePath);
                if (pidlList != IntPtr.Zero)
                {
                    try
                    {
                        Marshal.ThrowExceptionForHR(SHOpenFolderAndSelectItems(pidlList, 0, IntPtr.Zero, 0));
                    }
                    finally
                    {
                        ILFree(pidlList);
                    }
                }
            }
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
    }
}