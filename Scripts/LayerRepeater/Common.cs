#if !Sony
using ScriptPortal.Vegas;
#else
using Sony.Vegas;
#endif

using System.IO;
using System.Linq;
using System.Drawing;
using System.Windows.Forms;
using System.Collections.Generic;

namespace LayerRepeater
{
    public static class Common
    {
        public static int VegasVersion = System.Diagnostics.FileVersionInfo.GetVersionInfo(Application.ExecutablePath).FileMajorPart;
        public static bool CtrlMode => (Control.ModifierKeys & Keys.Control) != 0;
        public static string appFolder = System.Environment.GetFolderPath(System.Environment.SpecialFolder.ApplicationData);
        public static string IniFolder = Path.Combine(VegasVersion < 14 ? Path.Combine(appFolder, "Sony") : appFolder, "VEGAS Pro", VegasVersion + ".0");
        public static IniFile IniMiscz = new IniFile(Path.Combine(IniFolder, "MisczTools.ini"));

        public static Color[] GetColors(this Vegas myVegas)
        {
            int darkModeType = IniMiscz.ReadInt("DarkModeType", "MisczTools", 0);
            if (darkModeType < 0 || darkModeType > 4)
            {
                darkModeType = 0;
            }

            int myVegasVersion = VegasVersion;
#if !Sony
            if (darkModeType == 0 && myVegasVersion > 14)
            {
                darkModeType = myVegas.GetDarkType() + 1;
            }
#endif
            if (myVegasVersion > 18)
            {
                darkModeType += 4;
            }

            int[,] colorValues = new int[9, 2]
            {
              // {back, fore}
                 {153, 25} , // Earlier, for Vegas Pro 13 - 14
                 {45, 220} , // DarkEarly, for Vegas Pro 15 - 18
                 {94, 220} , // MediumEarly
                 {146, 29} , // LightEarly
                 {210, 35} , // WhiteEarly
                 {34, 220} , // Dark, for Vegas Pro 19 - 22
                 {68, 255} , // Medium
                 {187, 17} , // Light
                 {238, 51}   // White
            };

            return new Color[] { Color.FromArgb(colorValues[darkModeType, 0], colorValues[darkModeType, 0], colorValues[darkModeType, 0]), Color.FromArgb(colorValues[darkModeType, 1], colorValues[darkModeType, 1], colorValues[darkModeType, 1]) };
        }

#if !Sony
        // not supported in older versions, so I used a single method separately
        public static int GetDarkType(this Vegas myVegas)
        {
            myVegas.GetInterfaceType(out InterfaceType type);
            return (int)type;
        }

        public static void ApplyAfterComposite(this Effect ef, bool b)
        {
            ef.ApplyAfterComposite = b;
        }
#endif
        public static List<VideoEvent> GetSelectedVideoEvents(this Vegas vegas, bool SortByTime = false)
        {
            return vegas.Project.GetSelectedVideoEvents(SortByTime);
        }

        public static List<VideoEvent> GetSelectedVideoEvents(this Project Project, bool SortByTime = false)
        {
            return Project.GetSelectedEvents(MediaType.Video, SortByTime).ConvertAll(ev => ev as VideoEvent);
        }

        public static List<AudioEvent> GetSelectedAudioEvents(this Project Project, bool SortByTime = false)
        {
            return Project.GetSelectedEvents(MediaType.Audio, SortByTime).ConvertAll(ev => ev as AudioEvent);
        }

        public static List<TrackEvent> GetSelectedEvents(this Project Project, MediaType type = MediaType.Unknown, bool SortByTime = false)
        {
            var selectedEvents = new List<TrackEvent>();
            foreach (Track trk in Project.Tracks)
            {
                if ((trk.IsAudio() && type == MediaType.Video) || (trk.IsVideo() && type == MediaType.Audio))
                {
                    continue;
                }
                selectedEvents.AddRange(trk.Events.Where(ev => ev.Selected));
            }

            if (SortByTime)
            {
                selectedEvents = selectedEvents.SortByTime();
            }
            return selectedEvents;
        }

        public static List<TrackEvent> SortByTime(this List<TrackEvent> events)
        {
            var sortedEvents = new List<TrackEvent>(events);
            sortedEvents.Sort(delegate (TrackEvent a, TrackEvent b)
            {
                int startCompare = Comparer<Timecode>.Default.Compare(a.Start, b.Start);
                if (startCompare != 0)
                    return startCompare;

                int endCompare = Comparer<Timecode>.Default.Compare(a.End, b.End);
                if (endCompare != 0)
                    return endCompare;
                return Comparer<int>.Default.Compare(a.Track.Index, b.Track.Index);
            });
            return sortedEvents;
        }

        public static List<OFXKeyframe> SortByTime(this List<OFXKeyframe> kfs)
        {
            var sortedKfs = new List<OFXKeyframe>(kfs);
            sortedKfs.Sort(delegate (OFXKeyframe a, OFXKeyframe b)
            {
                int startCompare = Comparer<Timecode>.Default.Compare(a.Time, b.Time);
                return startCompare;
            });
            return sortedKfs;
        }
    }
}