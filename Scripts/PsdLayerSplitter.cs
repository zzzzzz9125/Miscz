using System;
using System.Windows.Forms;

using ScriptPortal.Vegas;  // "ScriptPortal.Vegas" for Magix Vegas Pro 14 or above, "Sony.Vegas" for Sony Vegas Pro 13 or below

namespace Test_Script
{

    public class Class
    {
        public Vegas myVegas;
        public void Main(Vegas vegas)
        {
            myVegas = vegas;
            myVegas.ResumePlaybackOnScriptExit = true;
            Project project = myVegas.Project;
            Tracks myTracks = project.Tracks;
            bool ctrlMode = ((Control.ModifierKeys & Keys.Control) != 0) ? true : false;

            for (int jj = myTracks.Count - 1; jj >= 0; jj--)
            {
                if (myTracks[jj].IsVideo())
                {
                    foreach (VideoEvent vEvent in myTracks[jj].Events)
                    {
                        VideoTrack myTrack = (VideoTrack) myTracks[jj];
                        if (vEvent.Selected)
                        {
                            if (vEvent.ActiveTake == null || vEvent.ActiveTake.Media == null || vEvent.ActiveTake.MediaStream == null)
                            {
                                continue;
                            }

                            int vStreamCount = vEvent.ActiveTake.Media.StreamCount(MediaType.Video);
                            if (vStreamCount < 2)
                            {
                                continue;
                            }

                            // If you hold down Ctrl and click the script icon on the toolbar, the selected event will be converted to Stream 1 (in a programming sense, it's a video stream with Index 0).
                            if (ctrlMode)
                            {
                                Take newTake = Take.CreateInstance(project, vEvent.ActiveTake.Media.GetVideoStreamByIndex(0));
                                vEvent.Takes.Clear();
                                vEvent.Takes.Add(newTake);
                            }

                            int vStreamIndex = GetVideoStreamIndex(vEvent);
                            if (vStreamIndex == 0)
                            {
                                vEvent.Mute = true;
                            }

                            int vStreamCountAdd = Mod(vStreamIndex - 1, vStreamCount);

                            TrackEventGroup grp = vEvent.Group;
                            if (grp == null)
                            {
                                grp = new TrackEventGroup(project);
                                project.Groups.Add(grp);
                                grp.Add(vEvent);
                            }

                            for (int i = 0; i < vStreamCountAdd; i++)
                            {
                                int j = i + jj;
                                VideoTrack newTrack = null;
                                if (j < myTracks.Count - 1)
                                {
                                    Track trackBelow = myTracks[j + 1];
                                    if (trackBelow.IsVideo() && !((VideoTrack)trackBelow).IsCompositingParent && (vStreamIndex == 0 ? ((VideoTrack)trackBelow).CompositeNestingLevel == myTrack.CompositeNestingLevel + 1 : ((VideoTrack)trackBelow).CompositeNestingLevel == myTrack.CompositeNestingLevel))
                                    {
                                        newTrack = (VideoTrack)trackBelow;
                                    }
                                }

                                if (newTrack == null)
                                {
                                    newTrack = new VideoTrack(project, j + 1, null);
                                    myTracks.Add(newTrack);
                                    newTrack.CompositeNestingLevel = myTrack.CompositeNestingLevel;
                                    if (vStreamIndex == 0)
                                    {
                                        newTrack.CompositeNestingLevel += 1;
                                    }
                                }

                                foreach (VideoEvent evnt in newTrack.Events)
                                {
                                    if (evnt.Start == vEvent.Start)
                                    {
                                        if (evnt.ActiveTake == null || evnt.ActiveTake.Media == null || evnt.ActiveTake.MediaStream == null || evnt.ActiveTake.Media == vEvent.ActiveTake.Media)
                                        {
                                            newTrack.Events.Remove(evnt);
                                        }
                                    }
                                }

                                VideoEvent newEvent = (VideoEvent) vEvent.Copy(newTrack, vEvent.Start);
                                Take newTake = Take.CreateInstance(project, vEvent.ActiveTake.Media.GetVideoStreamByIndex(vStreamCountAdd - i));
                                newEvent.Takes.Clear();
                                newEvent.Takes.Add(newTake);
                                newEvent.Selected = false;
                                newEvent.Mute = false;
                                grp.Add(newEvent);
                            }
                        }
                    }
                }
            }
        }

        public static int Mod(double a, double b)
        {
            int c = (int)(a - Math.Floor(a / b) * b);
            return c;
        }

        public static int GetVideoStreamIndex(VideoEvent vEvent)
        {
            int i = -1;
            foreach (MediaStream ms in vEvent.ActiveTake.Media.Streams)
            {
                if (ms.MediaType == MediaType.Video)
                {
                    i++;
                    if (ms == vEvent.ActiveTake.MediaStream)
                    {
                        return i;
                    }
                }
            }
            return 0;
        }
    }
}


public class EntryPoint
{
    public void FromVegas(Vegas vegas)
    {
        Test_Script.Class test = new Test_Script.Class();
        test.Main(vegas);
    }
}