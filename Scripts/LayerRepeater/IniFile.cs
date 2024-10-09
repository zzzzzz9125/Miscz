using System.Runtime.InteropServices;

namespace LayerRepeater
{
    public class IniFile
    {
        string Path;
        string EXE = System.Reflection.Assembly.GetExecutingAssembly().GetName().Name;
        [DllImport("kernel32", CharSet = CharSet.Unicode)]
        static extern long WritePrivateProfileString(string Section, string Key, string Value, string FilePath);
        [DllImport("kernel32", CharSet = CharSet.Unicode)]
        static extern int GetPrivateProfileString(string Section, string Key, string Default, System.Text.StringBuilder RetVal, int Size, string FilePath);
        public IniFile(string IniPath = null)
        {
            Path = new System.IO.FileInfo(IniPath ?? EXE + ".ini").FullName;
        }
        public string Read(string Key, string Section = null)
        {
            System.Text.StringBuilder RetVal = new System.Text.StringBuilder(255);
            GetPrivateProfileString(Section ?? EXE, Key, "", RetVal, 255, Path);
            return RetVal.ToString();
        }
        public bool ReadBool(string Key, string Section = null, bool Default = false)
        {
            string str = Read(Key, Section);
            return string.IsNullOrEmpty(str) ? Default : str == "1";
        }
        public int ReadInt(string Key, string Section = null, int Default = 0)
        {
            return int.TryParse(Read(Key, Section), out int tmp) ? tmp : Default;
        }
        public double ReadDouble(string Key, string Section = null, double Default = 0)
        {
            return double.TryParse(Read(Key, Section), out double tmp) ? tmp : Default;
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