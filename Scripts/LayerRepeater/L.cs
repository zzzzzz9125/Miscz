namespace LayerRepeater
{
    public static class L
    {
        public static string CurrentLanguage, Font, LayerRepeater, Mode, Count, Speed, BpmNote, Range, Reverse, Mute,
                             Settings, RangeAdapt, TransferToParent, TrackMotion, TrackFx, Language, DarkMode,
                             Clear, Cancel, OK, UIChange, FxSplitEnable, FxSplitMessage, FxSplitCaption, LowVegasVersion,
                             RangeError1, RangeError2, RangeError3, RangeError4;
        public static string[] ModeType, SpeedType, RangeType, DarkModeType, FxSplitEnableType;

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
                    LayerRepeater = "ͼ���ظ���"; Mode = "ģʽ";  Count = "ͼ����"; Speed = "�ٶ�"; Range = "��Χ"; Reverse = "����"; Mute = "����ԭ�¼�";
                    Settings = "����"; RangeAdapt = "��Χ����Ӧ"; TransferToParent = "�����������Ϊ����"; TrackMotion = "����˶�"; TrackFx = "��� FX"; Language = "����"; DarkMode = "������ɫ";
                    Clear = "���"; Cancel = "ȡ��"; OK = "ȷ��";
                    
                    UIChange = "��������������ű���Ż���Ч��";
                    FxSplitEnable = "FX ���";
                    FxSplitMessage = "���¼��ϵĵ� {0} �� FX Ч����{1} ���ڶ���ؼ�֡��\n�Ƿ������������";
                    FxSplitCaption = "FX �������";
                    LowVegasVersion = "VEGAS Pro �汾���ͣ���֧�ָò�����";

                    RangeError1 = "�޷�����ͼ�㣡��ѡ�����ٺ��� 2 ��ƽ�ƹؼ�֡�����¼���";
                    RangeError2 = "�޷�����ͼ�㣡��ѡ�����ٺ��� 2 �� FX �ؼ�֡�����¼���";
                    RangeError3 = "�޷�����ͼ�㣡���λ�ò����¼���β��Χ�ڣ�";
                    RangeError4 = "�޷�����ͼ�㣡ѭ���������¼���β��Χ�ڣ�";

                    ModeType = new string[] { "�̶�����", "�̶��ٶ�" };
                    SpeedType = new string[] { "FPS", "BPM" };
                    RangeType = new string[] { "�����¼�", "ƽ�ƹؼ�֡", "FX �ؼ�֡", "���λ��", "ѭ����" };
                    DarkModeType = new string[] { "�Զ�", "�ڰ�", "�е�", "���", "��ɫ" };
                    FxSplitEnableType = new string[] { "ÿ��ȷ��", "ʼ��Ӧ��", "����" };
                    break;

                default:
                    Font = "Arial";
                    LayerRepeater = "LayerRepeater"; Mode = "Mode"; Count = "Count"; Speed = "Speed"; Range = "Range"; Reverse = "Reverse"; Mute = "Mute Origin";
                    Settings = "Settings"; RangeAdapt = "Range Adapt"; TransferToParent = "Track Properties To Parent"; TrackMotion = "Motion"; TrackFx = "FX"; Language = "Language"; DarkMode = "UI Color"; 
                    Clear = "Clear"; Cancel = "Cancel"; OK = "OK";

                    UIChange = "UI changes will not take effect until the script is restarted!";
                    FxSplitEnable = "FX Split";
                    FxSplitMessage = "There're some keyframes in FX {0}: {1} on this event.\nDo you want to split this animation?";
                    FxSplitCaption = "FX Animation Split";
                    LowVegasVersion = "Not supported due to low VEGAS Pro version!";

                    RangeError1 = "Failed to generate layers! Please select an event with more than 1 Pan keyframes!";
                    RangeError2 = "Failed to generate layers! Please select an event with more than 1 FX keyframes!";
                    RangeError3 = "Failed to generate layers! Cursor Position is not within the event range!";
                    RangeError4 = "Failed to generate layers! Loop region is not within the event range!";

                    ModeType = new string[] { "Fixed Count", "Fixed Speed" };
                    SpeedType = new string[] { "FPS", "BPM" };
                    RangeType = new string[] { "Whole Event", "Pan Kfs", "FX Kfs", "Cursor", "Loop Region" };
                    DarkModeType = new string[] { "Auto", "Dark", "Medium", "Light", "White" };
                    FxSplitEnableType = new string[] { "Dialog", "Always", "Disabled" };
                    break;
            }
        }
    }
}