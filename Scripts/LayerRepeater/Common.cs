#define Sony

using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text;
using System.Windows.Forms;
using MyINI;

#if MAGIX
using ScriptPortal.Vegas;
#else
using Sony.Vegas;
#endif

namespace LayerRepeater
{
    public static class Common
    {
        public static int VegasVersion = System.Diagnostics.FileVersionInfo.GetVersionInfo(Application.ExecutablePath).FileMajorPart;
        public static bool CtrlMode => (Control.ModifierKeys & Keys.Control) != 0;
        public static string appFolder = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData);
        public static string IniFolder = Path.Combine(VegasVersion < 14 ? Path.Combine(appFolder, "Sony") : appFolder, "VEGAS Pro", VegasVersion + ".0");
        public static IniFile IniMiscz = new IniFile(Path.Combine(IniFolder, "MisczTools.ini"));

        public static Color[] GetColors(this Vegas myVegas)
        {
            int.TryParse(IniMiscz.Read("DarkModeType", "MisczTools"), out int darkModeType);
            if (darkModeType < 0 || darkModeType > 4)
            {
                darkModeType = 0;
            }

           int myVegasVersion = VegasVersion;
#if MAGIX
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

#if MAGIX
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
			return Project.GetSelectedEvents(MediaType.Video, SortByTime).ConvertAll<VideoEvent>(ev => ev as VideoEvent);
        }

        public static List<AudioEvent> GetSelectedAudioEvents(this Project Project, bool SortByTime = false)
        {
            return Project.GetSelectedEvents(MediaType.Audio, SortByTime).ConvertAll<AudioEvent>(ev => ev as AudioEvent);
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
			sortedEvents.Sort(delegate(TrackEvent a, TrackEvent b)
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