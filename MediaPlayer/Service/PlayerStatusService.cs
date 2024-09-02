using CommunityToolkit.Mvvm.Messaging;
using MediaPlayer.Message;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace MediaPlayer.Service
{
    public class PlayerStatusService
    {
        public enum Status
        {
            Stopped,
            RtspConnected,
            RtspPlaying
        }

        public enum Mode
        {
            Rtsp,
            File
        }






        public static PlayerStatusService Instance { get; } = new PlayerStatusService();



        private Mode _PlayerMode = Mode.File;
        public Mode PlayerMode
        {
            get
            {
                return this._PlayerMode;
            }
            set
            {
                this._PlayerMode = value;
            }
        }


        private Status _PlayerStatus = Status.Stopped;
        public Status PlayerStatus
        {
            get
            {
                return this._PlayerStatus;
            }
            set
            {
                this._PlayerStatus = value;
                WeakReferenceMessenger.Default.Send(new PlayserStatusMessage()
                {
                    PlayerStatus = value
                });
            }
        }




        private string? _RtspAddress = string.Empty;
        public string? RtspAddress
        {
            get
            {
                return this._RtspAddress;
            }
            set
            {
                this._FileAddress = null;
                this._RtspAddress = value;
                if (value!=null)
                {
                    this.PlayerMode = Mode.Rtsp;
                    WeakReferenceMessenger.Default.Send(new VideoSourceSetMessage()
                    {
                        SelectedSourceType = VideoSourceSetMessage.SourceType.RTSP
                    });
                }
            }
        }
        private string? _FileAddress = string.Empty;
        public string? FileAddress {
            get
            {
                return this._FileAddress;
            }
            set
            {
                this._RtspAddress = null;
                this._FileAddress = value;
                if (value != null)
                {
                    this.PlayerMode = Mode.File;
                    WeakReferenceMessenger.Default.Send(new VideoSourceSetMessage()
                    {
                        SelectedSourceType = VideoSourceSetMessage.SourceType.File
                    });
                }
            }
        }







        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern bool initRtspPlayer(string rtspAddress);

        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void playRtsp(IntPtr hWnd);

        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void stopRtsp();





        public bool InitRtspPlayer(string rtspAddress)
        {
            bool res = initRtspPlayer(rtspAddress);
            if (res == true)
            {
                this.PlayerStatus = Status.RtspConnected;
            }
            return res;
        }

        public void PlayRtsp(IntPtr hWnd)
        {
            if (this.PlayerStatus == Status.RtspConnected)
            {
                playRtsp(hWnd);
            }
        }

        public void StopRtsp()
        {
            if (this.PlayerStatus == Status.RtspPlaying)
            {
                stopRtsp();
            }
        }














    }
}
