namespace LayerRepeater
{
    public static class L
    {
        public static string CurrentLanguage, Font, LayerRepeater, Count, Mute, Settings, TransferToParentFX, Language, DarkMode, Clear, Cancel, OK, TooFewKeyframes, UIChange;
        public static string[] DarkModeType;

        // Some text localization.
        public static void Localize()
        {
            if (string.IsNullOrEmpty(CurrentLanguage))
            {
                string tmp = Common.IniMiscz.Read("Language", "MisczTools");
                CurrentLanguage = string.IsNullOrEmpty(tmp) ? System.Globalization.CultureInfo.CurrentCulture.TwoLetterISOLanguageName : tmp;
            }

            switch (CurrentLanguage)
            {
                case "zh":
                    Font = "Microsoft Yahei UI";
                    LayerRepeater = "ͼ���ظ���"; Count = "ͼ����"; Mute = "��ԭʼ�¼�����"; Settings = "����"; TransferToParentFX = "ת�Ƶ����� FX"; Language = "����"; DarkMode = "������ɫ"; Clear = "���"; Cancel = "ȡ��"; OK = "ȷ��";
                    TooFewKeyframes = "�޷�����ͼ�㣡��ѡ�����ٺ��� 2 ��ƽ��/���йؼ�֡�����¼���";
                    UIChange = "��������������ű���Ż���Ч��";
                    DarkModeType = new string[] { "�Զ�", "�ڰ�", "�е�", "���", "��ɫ" };
                    break;

                default:
                    Font = "Arial";
                    LayerRepeater = "LayerRepeater"; Count = "Count"; Mute = "Mute Original Event"; Settings = "Settings"; TransferToParentFX = "Transfer To Parent FX"; Language = "Language"; DarkMode = "UI Color"; Clear = "Clear"; Cancel = "Cancel"; OK = "OK";
                    TooFewKeyframes = "Failed to generate layers! Please select an event with more than 1 Pan/Crop keyframes!";
                    UIChange = "UI changes will not take effect until the script is restarted!";
                    DarkModeType = new string[] { "Auto", "Dark", "Medium", "Light", "White" };
                    break;
            }
        }
    }
}