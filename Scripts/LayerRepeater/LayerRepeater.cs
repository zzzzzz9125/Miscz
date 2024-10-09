#if !Sony
using ScriptPortal.Vegas;
#else
using Sony.Vegas;
#endif

namespace LayerRepeater
{

    public class LayerRepeaterClass
    {
        public Vegas myVegas;

        public void Main(Vegas vegas)
        {
            myVegas = vegas;
            myVegas.ResumePlaybackOnScriptExit = true;
            myVegas.UnloadScriptDomainOnScriptExit = true;
            LayerRepeaterArgs args = new LayerRepeaterArgs();
            L.Localize();

            System.Collections.Generic.List<VideoEvent> vEvents = myVegas.Project.GetSelectedVideoEvents(false);

            if (!Common.CtrlMode && !myVegas.PopUpWindow(out args))
            {
                return;
            }

            // to avoid problems later when creating track groups
            foreach (Track myTrack in myVegas.Project.Tracks)
            {
                myTrack.Selected = false;
            }

            foreach (VideoEvent vEvent in vEvents)
            {
                myVegas.RepeatLayer(vEvent, args);
            }
        }
    }
}

public class EntryPoint
{
    public void FromVegas(Vegas vegas)
    {
        LayerRepeater.LayerRepeaterClass test = new LayerRepeater.LayerRepeaterClass();
        test.Main(vegas);
    }
}