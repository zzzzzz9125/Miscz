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
                    LayerRepeater = "图层重复器"; Count = "图层数"; Mute = "将原始事件静音"; Settings = "设置"; TransferToParentFX = "转移到父轨 FX"; Language = "语言"; DarkMode = "界面颜色"; Clear = "清除"; Cancel = "取消"; OK = "确定";
                    TooFewKeyframes = "无法生成图层！请选择「至少含有 2 个平移/裁切关键帧」的事件！";
                    UIChange = "界面更改在重启脚本后才会生效！";
                    DarkModeType = new string[] { "自动", "黑暗", "中等", "轻度", "白色" };
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