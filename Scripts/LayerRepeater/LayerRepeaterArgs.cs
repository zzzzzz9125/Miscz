using ScriptPortal.Vegas;

namespace LayerRepeater
{
    public class LayerRepeaterArgs
    {
        public int Mode = 0, Count = 0, SpeedType = 0, SpeedOperatorType = 0, RangeType = 0, FxSplitType = 0;
        public double Velocity = 0, Speed = 0, SpeedMultiplier = 0, TrueSpeed = 0;
        public bool VelocityEnable = false, Reverse = false, Mute = true, ProgressSync = false, RangeAdapt = true, TransferToParentMotion = true, TransferToParentFx = true;

        public LayerRepeaterArgs(int mode, int count, int rangeType, bool velocityEnable, double velocity, bool reverse, bool mute, bool progressSync, bool transferToParentMotion, bool transferToParentFx)
        {
            Mode = mode;
            Count = count;
            RangeType = rangeType;
            VelocityEnable = velocityEnable;
            Velocity = velocity;
            Reverse = reverse;
            Mute = mute;
            ProgressSync = progressSync;
            TransferToParentMotion = transferToParentMotion;
            TransferToParentFx = transferToParentFx;
        }

        public LayerRepeaterArgs()
        {

        }

        public static LayerRepeaterArgs LoadFromIni()
        {
            LayerRepeaterArgs args = new LayerRepeaterArgs
            {
                Mode = Common.IniMiscz.ReadInt("ModeType", "LayerRepeater", 0),
                Count = Common.IniMiscz.ReadInt("Count", "LayerRepeater", 50),
                Speed = Common.IniMiscz.ReadDouble("Speed", "LayerRepeater", 2),
                SpeedType = Common.IniMiscz.ReadInt("SpeedType", "LayerRepeater", 0),
                SpeedOperatorType = Common.IniMiscz.ReadInt("SpeedOperatorType", "LayerRepeater", 0),
                SpeedMultiplier = Common.IniMiscz.ReadDouble("SpeedMultiplier", "LayerRepeater", 1),
                RangeType = Common.IniMiscz.ReadInt("RangeType", "LayerRepeater", 1),
                VelocityEnable = Common.IniMiscz.ReadBool("VelocityEnable", "LayerRepeater", false),
                Velocity = Common.IniMiscz.ReadInt("Velocity", "LayerRepeater", 0),
                Reverse = Common.IniMiscz.ReadBool("Reverse", "LayerRepeater", false),
                Mute = Common.IniMiscz.ReadBool("Mute", "LayerRepeater", true),
                ProgressSync = Common.IniMiscz.ReadBool("ProgressSync", "LayerRepeater", false),
                TransferToParentMotion = Common.IniMiscz.ReadBool("TransferToParentMotion", "LayerRepeater", true),
                TransferToParentFx = Common.IniMiscz.ReadBool("TransferToParentFx", "LayerRepeater", true)
            };
            if (args.Count < 2)
            {
                args.Count = 50;
            }

            if (args.Speed <= 0)
            {
                args.Speed = 2;
            }

            if (args.SpeedMultiplier <= 0)
            {
                args.SpeedMultiplier = 1;
            }
            return args;
        }

        public void SaveToIni()
        {
            if (Count > 1 || Speed > 0)
            {
                Common.IniMiscz.Write("ModeType", Mode.ToString(), "LayerRepeater");
                if (Count > 1)
                {
                    Common.IniMiscz.Write("Count", Count.ToString(), "LayerRepeater");
                }
                if (Speed > 0)
                {
                    Common.IniMiscz.Write("Speed", Speed.ToString(), "LayerRepeater");
                }
            }
            Common.IniMiscz.Write("SpeedType", SpeedType.ToString(), "LayerRepeater");
            Common.IniMiscz.Write("SpeedOperatorType", SpeedOperatorType.ToString(), "LayerRepeater");
            Common.IniMiscz.Write("SpeedMultiplier", SpeedMultiplier.ToString(), "LayerRepeater");
            Common.IniMiscz.Write("RangeType", RangeType.ToString(), "LayerRepeater");
            Common.IniMiscz.Write("Reverse", Reverse ? "1" : "0", "LayerRepeater");
            Common.IniMiscz.Write("VelocityEnable", VelocityEnable ? "1" : "0", "LayerRepeater");
            Common.IniMiscz.Write("Velocity", Velocity.ToString(), "LayerRepeater");
            Common.IniMiscz.Write("Mute", Mute ? "1" : "0", "LayerRepeater");
            Common.IniMiscz.Write("ProgressSync", ProgressSync ? "1" : "0", "LayerRepeater");
            Common.IniMiscz.Write("TransferToParentMotion", TransferToParentMotion ? "1" : "0", "LayerRepeater");
            Common.IniMiscz.Write("TransferToParentFx", TransferToParentFx ? "1" : "0", "LayerRepeater");         
        }

        public void GetSettingsFromIni()
        {
            RangeAdapt = Common.IniMiscz.ReadBool("RangeAdapt", "LayerRepeater", true);
            FxSplitType = Common.IniMiscz.ReadInt("FxSplitType", "LayerRepeater", 0);
        }

        public void SetSpeedParameters(double speed, int speedType, int speedOperatorType, double speedMultiplier)
        {
            Speed = speed;
            SpeedType = speedType;
            SpeedOperatorType = speedOperatorType;
            SpeedMultiplier = speedMultiplier;
        }

        public void RefreshTrueSpeed()
        {
            TrueSpeed = Speed;
            if (SpeedOperatorType == 1)
            {
                TrueSpeed /= SpeedMultiplier != 0 ? SpeedMultiplier : 1;
            }
            else
            {
                TrueSpeed *= SpeedMultiplier;
            }

            // BPM to FPS
            if (SpeedType == 1)
            {
                TrueSpeed /= 60;
            }
        }
    }
}
