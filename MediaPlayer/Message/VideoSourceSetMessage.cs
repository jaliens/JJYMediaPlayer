using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace MediaPlayer.Message
{
    public class VideoSourceSetMessage
    {
        public enum SourceType
        {
            File,
            RTSP
        }

        public SourceType SelectedSourceType { get; set; }
    }
}
