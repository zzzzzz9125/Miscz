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
                    LayerRepeater = "图层重复器"; Mode = "模式";  Count = "图层数"; Speed = "速度"; Range = "范围"; Reverse = "反向"; Mute = "静音原事件";
                    Settings = "设置"; RangeAdapt = "范围自适应"; TransferToParent = "轨道属性升级为父轨"; TrackMotion = "轨道运动"; TrackFx = "轨道 FX"; Language = "语言"; DarkMode = "界面颜色";
                    Clear = "清除"; Cancel = "取消"; OK = "确定";
                    
                    UIChange = "界面更改在重启脚本后才会生效！";
                    FxSplitEnable = "FX 拆分";
                    FxSplitMessage = "该事件上的第 {0} 个 FX 效果：{1} 存在多个关键帧。\n是否拆分这个动画？";
                    FxSplitCaption = "FX 动画拆分";
                    LowVegasVersion = "VEGAS Pro 版本过低，不支持该参数！";

                    RangeError1 = "无法生成图层！请选择「至少含有 2 个平移关键帧」的事件！";
                    RangeError2 = "无法生成图层！请选择「至少含有 2 个 FX 关键帧」的事件！";
                    RangeError3 = "无法生成图层！光标位置不在事件首尾范围内！";
                    RangeError4 = "无法生成图层！循环区不在事件首尾范围内！";

                    ModeType = new string[] { "固定层数", "固定速度" };
                    SpeedType = new string[] { "FPS", "BPM" };
                    RangeType = new string[] { "整个事件", "平移关键帧", "FX 关键帧", "光标位置", "循环区" };
                    DarkModeType = new string[] { "自动", "黑暗", "中等", "轻度", "白色" };
                    FxSplitEnableType = new string[] { "每次确认", "始终应用", "禁用" };
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