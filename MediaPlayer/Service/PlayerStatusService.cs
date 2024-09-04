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







        public IntPtr CreateDirectXWindow(IntPtr hInstance, int width, int height, IntPtr parentHwnd)
        {
            return createDirectXWindow(hInstance, width, height, parentHwnd);
        }


        public void PlayRtsp()
        {
            if (this.PlayerMode == Mode.Rtsp &&
                string.IsNullOrWhiteSpace(this._RtspAddress) == false)
            {
                playRtsp(this._RtspAddress);
            }
        }

        public void StopRtsp()
        {
            if (this.PlayerMode == Mode.Rtsp)
            {
                stopRtsp();
            }
        }



        public void CreatePlayer()
        {
            createPlayer();
        }

        public void RegisterWindowHandle(IntPtr hWnd)
        {
            registerWindowHandle(hWnd);
        }



        public void Play()
        {
            if (this.PlayerMode == Mode.File &&
                string.IsNullOrWhiteSpace(this._FileAddress) == false)
            {
                play(this._FileAddress);
            }
        }

        public void Stop()
        {
            if (this.PlayerMode == Mode.File)
            {
                stop();
            }
        }

        public void JumpPlayTime(double targetPercent)
        {
            if (this.PlayerMode == Mode.File)
            {
                jumpPlayTime(targetPercent);
            }
        }

        public void Pause()
        {
            if (this.PlayerMode == Mode.File)
            {
                pause();
            }
        }

        public void RegisterOnImgDecodeCallback(ImageCallback callback)
        {
            this._imageCallBack = callback;
            registerOnImgDecodeCallback(callback);
            GC.KeepAlive(this._imageCallBack);
        }

        public void RegisterOnVideoLengthCallback(OnVideoLengthCallbackFunction callback)
        {
            this._videoLengthCallback = callback;
            registerOnVideoLengthCallback(callback);
            GC.KeepAlive(this._videoLengthCallback);
        }

        public void RegisterOnVideoProgressCallback(OnVideoProgressCallbackFunction callback)
        {
            this._videoProgressCallback = callback;
            registerOnVideoProgressCallback(callback);
            GC.KeepAlive(this._videoProgressCallback);
        }

        public void RegisterOnBufferProgressCallback(OnBufferProgressCallbackFunction callback)
        {
            this._bufferProgressCallback = callback;
            registerOnBufferProgressCallback(callback);
            GC.KeepAlive(this._bufferProgressCallback);
        }

        public void RegisterOnStartCallback(OnStartCallbackFunction callback)
        {
            this._startCallback = callback;
            registerOnStartCallback(callback);
            GC.KeepAlive(this._startCallback);
        }

        public void RegisterOnPauseCallback(OnPauseCallbackFunction callback)
        {
            this._pauseCallback = callback;
            registerOnPauseCallback(callback);
            GC.KeepAlive(this._pauseCallback);
        }

        public void RegisterOnResumeCallback(OnResumeCallbackFunction callback)
        {
            this._resumeCallback = callback;
            registerOnResumeCallback(callback);
            GC.KeepAlive(this._resumeCallback);
        }

        public void RegisterOnStopCallback(OnStopCallbackFunction callback)
        {
            this._stopCallback = callback;
            registerOnStopCallback(callback);
            GC.KeepAlive(this._stopCallback);
        }

        public void RegisterOnRenderTimingCallback(OnRenderTimingCallbackFunction callback)
        {
            this._renderTimingCallback = callback;
            registerOnRenderTimingCallback(callback);
            GC.KeepAlive(this._renderTimingCallback);
        }

        public void RegisterOnVideoSizeCallback(OnVideoSizeCallbackFunction callback)
        {
            this._videoSizeCallback = callback;
            registerOnVideoSizeCallback(callback);
            GC.KeepAlive(this._videoSizeCallback);
        }






        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void ImageCallback(IntPtr data, int size, int width, int height);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void OnVideoLengthCallbackFunction(double length);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void OnVideoProgressCallbackFunction(double progress);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void OnBufferProgressCallbackFunction(double progress);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void OnBufferStartPosCallbackFunction(double progress);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void OnStartCallbackFunction();
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void OnPauseCallbackFunction();
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void OnResumeCallbackFunction();
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void OnStopCallbackFunction();
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void OnRenderTimingCallbackFunction();
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void OnVideoSizeCallbackFunction(int width, int height);


        private ImageCallback? _imageCallBack = null;
        private OnVideoLengthCallbackFunction? _videoLengthCallback = null;
        private OnVideoProgressCallbackFunction? _videoProgressCallback = null;
        private OnBufferProgressCallbackFunction? _bufferProgressCallback = null;
        private OnBufferStartPosCallbackFunction? _bufferStartPosCallback = null;
        private OnStartCallbackFunction? _startCallback = null;
        private OnPauseCallbackFunction? _pauseCallback = null;
        private OnResumeCallbackFunction? _resumeCallback = null;
        private OnStopCallbackFunction? _stopCallback = null;
        private OnRenderTimingCallbackFunction? _renderTimingCallback = null;
        private OnVideoSizeCallbackFunction? _videoSizeCallback = null;


        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern bool initRtspPlayer(string rtspAddress);

        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void playRtsp(string rtspPath);

        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void stopRtsp();

        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void registerWindowHandle(IntPtr hWnd);





        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern bool createPlayer();

        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern bool createPlayer(IntPtr hWnd);

        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void play(string filePath);

        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void pause();

        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void stop();

        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void jumpPlayTime(double targetPercent);

        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void cleanup();






        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern bool createDx11RenderScreenOnPlayer(IntPtr hWnd, int videoWidth, int videoHeight);

        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr createDirectXWindow(IntPtr hInstance, int width, int height, IntPtr parentHwnd);







        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void registerOnImgDecodeCallback(ImageCallback callback);

        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void registerOnVideoLengthCallback(OnVideoLengthCallbackFunction callback);

        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void registerOnVideoProgressCallback(OnVideoProgressCallbackFunction callback);

        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void registerOnBufferProgressCallback(OnBufferProgressCallbackFunction callback);

        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void registerOnStartCallback(OnStartCallbackFunction callback);

        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void registerOnPauseCallback(OnPauseCallbackFunction callback);

        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void registerOnResumeCallback(OnResumeCallbackFunction callback);

        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void registerOnStopCallback(OnStopCallbackFunction callback);

        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void registerOnRenderTimingCallback(OnRenderTimingCallbackFunction callback);

        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void registerOnVideoSizeCallback(OnVideoSizeCallbackFunction callback);




        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void RunDecodeExample1();

        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void RunDecodeAndSDLPlayExample();
    }
}
