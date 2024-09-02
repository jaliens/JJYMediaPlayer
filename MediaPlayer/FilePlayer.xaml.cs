using Microsoft.Win32;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Ink;
using System.Windows.Input;
using System.Windows.Markup;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Media.Media3D;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Windows.Forms.Integration;
using System.Threading;
using System.Windows.Interop;
using MediaPlayer.Mvvm;
using MediaPlayer.Service;
using CommunityToolkit.Mvvm.Messaging;
using OpenCvSharp.Internal.Vectors;
using MediaPlayer.Message;

namespace MediaPlayer
{
    /// <summary>
    /// FilePlayer.xaml에 대한 상호 작용 논리
    /// </summary>
    public partial class FilePlayer : UserControl
    {
        const uint SWP_NOZORDER = 0x0004;

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        private delegate void ImageCallback(IntPtr data, int size, int width, int height);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        private delegate void OnVideoLengthCallbackFunction(double length);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        private delegate void OnVideoProgressCallbackFunction(double progress);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        private delegate void OnBufferProgressCallbackFunction(double progress);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        private delegate void OnBufferStartPosCallbackFunction(double progress);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        private delegate void OnStartCallbackFunction();
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        private delegate void OnPauseCallbackFunction();
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        private delegate void OnResumeCallbackFunction();
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        private delegate void OnStopCallbackFunction();
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        private delegate void OnRenderTimingCallbackFunction();
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        private delegate void OnVideoSizeCallbackFunction(int width, int height);

        ImageCallback? _imageCallBack = null;
        OnVideoLengthCallbackFunction? _videoLengthCallback = null;
        OnVideoProgressCallbackFunction? _videoProgressCallback = null;
        OnBufferProgressCallbackFunction? _bufferProgressCallback = null;
        OnBufferStartPosCallbackFunction? _bufferStartPosCallback = null;
        OnStartCallbackFunction? _startCallback = null;
        OnPauseCallbackFunction? _pauseCallback = null;
        OnResumeCallbackFunction? _resumeCallback = null;
        OnStopCallbackFunction? _stopCallback = null;
        OnRenderTimingCallbackFunction? _renderTimingCallback = null;
        OnVideoSizeCallbackFunction? _videoSizeCallback = null;
        
        private int _videoWidth = 0;
        private int _videoHeight = 0;

        private IntPtr _directXWindowHandle = IntPtr.Zero;


        private bool _isPaused = false;

        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void RegisterOnImgDecodeCallback(ImageCallback callback);

        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void RegisterOnVideoLengthCallback(OnVideoLengthCallbackFunction callback);

        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void RegisterOnVideoProgressCallback(OnVideoProgressCallbackFunction callback);

        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void RegisterOnBufferProgressCallback(OnBufferProgressCallbackFunction callback);

        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void RegisterOnStartCallback(OnStartCallbackFunction callback);

        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void RegisterOnPauseCallback(OnPauseCallbackFunction callback);

        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void RegisterOnResumeCallback(OnResumeCallbackFunction callback);

        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void RegisterOnStopCallback(OnStopCallbackFunction callback);

        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void RegisterOnRenderTimingCallback(OnRenderTimingCallbackFunction callback);

        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void RegisterOnVideoSizeCallback(OnVideoSizeCallbackFunction callback);




        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern bool CreatePlayer();

        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void OpenFileStream(string? filePath, out int videoWidth, out int videoHeight);

        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void Play();

        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void Pause();

        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void Stop();

        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void JumpPlayTime(double targetPercent);

        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void Cleanup();








        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void RunDecodeExample1();

        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void RunDecodeAndSDLPlayExample();




















        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern bool CreateDx11RenderScreenOnPlayer(IntPtr hWnd, int videoWidth, int videoHeight);

        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr CreateDirectXWindow(IntPtr hInstance, int width, int height, IntPtr parentHwnd);

        [DllImport("user32.dll", SetLastError = true)]
        private static extern bool SetWindowPos(IntPtr hWnd, IntPtr hWndInsertAfter, int X, int Y, int cx, int cy, uint uFlags);


        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void RenderTestRectangleDx11();
















        public FilePlayer()
        {
            this.InitializeComponent();
            this.Loaded += this.OnFilePlayer_Loaded;
            this.SizeChanged += OnFilePlayer_SizeChanged;

            this._imageCallBack = this.OnImageCallback;
            this._videoLengthCallback = this.OnVideoLengthCallback;
            this._videoProgressCallback = this.OnVideoProgressCallback;
            this._bufferProgressCallback = this.OnBufferProgressCallback;
            this._bufferStartPosCallback = this.OnBufferStartPosCallback;
            this._pauseCallback = this.OnPauseCallback;
            this._resumeCallback = this.OnResumeCallback;
            this._stopCallback = this.OnStopCallback;
            this._renderTimingCallback = this.OnRenderTimingCallback;
            this._videoSizeCallback = this.OnVideoSizeCallback;

            this.RegisterToMessage();
        }

        private void RegisterToMessage()
        {
            WeakReferenceMessenger.Default.Register<VideoSourceSetMessage>(this, this.OnVideoSourceSetMessage);
            WeakReferenceMessenger.Default.Register<PlayserStatusMessage>(this, this.OnPlayserStatusMessage);
        }


        private void OnPlayserStatusMessage(object recipient, PlayserStatusMessage message)
        {

        }

        /// <summary>
        /// 영상 소스 설정 결정 시 호출
        /// </summary>
        /// <param name="recipient"></param>
        /// <param name="message"></param>
        private void OnVideoSourceSetMessage(object recipient, VideoSourceSetMessage message)
        {
            switch (message.SelectedSourceType)
            {
                case VideoSourceSetMessage.SourceType.File:
                    {
                    }
                    break;
                case VideoSourceSetMessage.SourceType.RTSP:
                    {
                        this.InitRtspPlayer();
                    }
                    break;
                default:
                    break;
            }
        }

        private void InitRtspPlayer()
        {
            if (string.IsNullOrWhiteSpace(PlayerStatusService.Instance.RtspAddress) == true)
            {
                return;
            }
            //this._videoWidth = 1280;
            //this._videoHeight = 720;
            this.CreateWin32Window();
            //CreateDx11RenderScreenOnPlayer(this._directXWindowHandle, this._videoWidth, this._videoHeight);
            PlayerStatusService.Instance.InitRtspPlayer(PlayerStatusService.Instance.RtspAddress);
        }

        private void OnStopCallback()
        {
            this._isPaused = false;
            Application.Current.Dispatcher.Invoke(() =>
            {
                this.btn_play.Visibility = Visibility.Visible;
                this.btn_pause.Visibility = Visibility.Hidden;
            });
        }

        private void OnRenderTimingCallback()
        {
            Application.Current.Dispatcher.Invoke(() =>
            {

            });
        }

        /// <summary>
        /// 플레이어 라이브러리에서 동영상 스트리밍의 비디오 사이즈가 결정되면 호출됨
        /// </summary>
        /// <param name="width"></param>
        /// <param name="height"></param>
        private void OnVideoSizeCallback(int width, int height)
        {
            this._videoWidth = width;
            this._videoHeight = height;
            this.UpdateDirectXWindowPosition();
        }

        private void OnResumeCallback()
        {
            this._isPaused = false;
        }

        private void OnPauseCallback()
        {
            this._isPaused = true;
        }

        private void OnFilePlayer_Loaded(object sender, RoutedEventArgs e)
        {
            CreatePlayer();

            if (this._imageCallBack != null)
            {
                RegisterOnImgDecodeCallback(this._imageCallBack);
            }
            if (this._videoLengthCallback != null)
            {
                RegisterOnVideoLengthCallback(this._videoLengthCallback);
            }
            if (this._videoProgressCallback != null)
            {
                RegisterOnVideoProgressCallback(this._videoProgressCallback);
            }
            if (this._bufferProgressCallback != null)
            {
                RegisterOnBufferProgressCallback(this._bufferProgressCallback);
            }
            if (this._pauseCallback != null)
            {
                RegisterOnPauseCallback(this._pauseCallback);
            }
            if (this._resumeCallback != null)
            {
                RegisterOnResumeCallback(this._resumeCallback);
            }
            if (this._stopCallback != null)
            {
                RegisterOnStopCallback(this._stopCallback);
            }
            if (this._renderTimingCallback != null)
            {
                RegisterOnRenderTimingCallback(this._renderTimingCallback);
            }
            if (this._videoSizeCallback != null)
            {
                RegisterOnVideoSizeCallback(this._videoSizeCallback);
            }


            //비관리쪽에서만 쓸 녀석들이기 때문에 가비지컬렉터에 의해 정리될 가능성 제거
            GC.KeepAlive(this._imageCallBack);
            GC.KeepAlive(this._videoLengthCallback);
            GC.KeepAlive(this._videoProgressCallback);
            GC.KeepAlive(this._bufferProgressCallback);
            GC.KeepAlive(this._bufferStartPosCallback);

            GC.KeepAlive(this._pauseCallback);
            GC.KeepAlive(this._resumeCallback);
            GC.KeepAlive(this._stopCallback);
            GC.KeepAlive(this._renderTimingCallback);

            var parentWnd = Window.GetWindow(this);
            parentWnd.LocationChanged += OnParentWnd_LocationChanged;
        }

        private void OnParentWnd_LocationChanged(object? sender, EventArgs e)
        {
            this.UpdateDirectXWindowPosition();
        }

        /// <summary>
        /// 파일로부터 영상 스트림을 열고<br/>
        /// 랜더링용 win32창을 만들고<br/>
        /// DirectX11 랜더러를 초기화
        /// </summary>
        private void OpenStreamAndRenderer()
        {
            if (string.IsNullOrWhiteSpace(PlayerStatusService.Instance.FileAddress) == false)
            {
                OpenFileStream(PlayerStatusService.Instance.FileAddress, out this._videoWidth, out this._videoHeight);
                this.CreateWin32Window();
                CreateDx11RenderScreenOnPlayer(this._directXWindowHandle, this._videoWidth, this._videoHeight);
            }
            else if (string.IsNullOrWhiteSpace(PlayerStatusService.Instance.RtspAddress) == false)
            {
                OpenFileStream(PlayerStatusService.Instance.RtspAddress, out this._videoWidth, out this._videoHeight);
                this.CreateWin32Window();
                CreateDx11RenderScreenOnPlayer(this._directXWindowHandle, this._videoWidth, this._videoHeight);
            }
        }

        /// <summary>
        /// directx 11 비디오 랜더링을 위한 win32 창 생성<br/>
        /// </summary>
        private void CreateWin32Window()
        {
            if (this._directXWindowHandle != IntPtr.Zero)
            {
                return;
            }

            var parentWnd = Window.GetWindow(this);
            double multiplier = 1.0;
            PresentationSource source = PresentationSource.FromVisual(parentWnd);
            if (source != null && source.CompositionTarget != null)
            {
                multiplier = source.CompositionTarget.TransformToDevice.M11;//디스플레이 설정에서 배율 설정값 가져오기
            }

            var videoRatio = (double)this._videoWidth / this._videoHeight;
            var gridRatio = this.win32WindowArea.ActualWidth / this.win32WindowArea.ActualHeight;
            if (this._videoWidth == 0 && this._videoHeight == 0)
            {
                videoRatio = 1;
            }

            double innerWidth = 0;
            double innerHeight = 0;
            if (videoRatio >= gridRatio)
            {
                innerWidth = this.win32WindowArea.ActualWidth;
                innerHeight = this.win32WindowArea.ActualWidth / videoRatio;
            }
            else //if (videoRatio < gridRatio)
            {
                innerHeight = this.win32WindowArea.ActualHeight;
                innerWidth = this.win32WindowArea.ActualHeight * videoRatio;
            }

            int targetWidth = (int)(innerWidth * multiplier);
            int targetHeight = (int)(innerHeight * multiplier);

            this._directXWindowHandle = CreateDirectXWindow(Process.GetCurrentProcess().Handle, (int)targetWidth, (int)targetHeight, new WindowInteropHelper(parentWnd).Handle);
            this.UpdateDirectXWindowPosition();
        }

        /// <summary>
        /// Win32 창의 위치와 크기를 WPF 창에 맞춰 업데이트
        /// </summary>
        private void UpdateDirectXWindowPosition()
        {
            Application.Current?.Dispatcher.Invoke(() =>
            {
                if (this._directXWindowHandle == IntPtr.Zero)
                {
                    return;
                }

                var parentWnd = Window.GetWindow(this);

                Point relativeLocation = win32WindowArea.TransformToAncestor(parentWnd)
                                                  .Transform(new Point(0, 0));

                double multiplier = 1.0;
                PresentationSource source = PresentationSource.FromVisual(this);
                if (source != null && source.CompositionTarget != null)
                {
                    multiplier = source.CompositionTarget.TransformToDevice.M11;//디스플레이 설정에서 배율 설정값 가져오기
                }





                var videoRatio = (double)this._videoWidth / this._videoHeight;
                var gridRatio = this.win32WindowArea.ActualWidth / this.win32WindowArea.ActualHeight;
                if (this._videoWidth == 0 && this._videoHeight == 0)
                {
                    videoRatio = 1;
                }

                double innerWidth = 0;
                double innerHeight = 0;
                double innerLeftOffset = 0;
                double innerTopOffset = 0;
                if (videoRatio >= gridRatio)
                {
                    innerWidth = this.win32WindowArea.ActualWidth;
                    innerHeight = this.win32WindowArea.ActualWidth / videoRatio;
                    innerTopOffset = (this.win32WindowArea.ActualHeight - innerHeight) / 2;
                }
                else //if (videoRatio < gridRatio)
                {
                    innerHeight = this.win32WindowArea.ActualHeight;
                    innerWidth = this.win32WindowArea.ActualHeight * videoRatio;
                    innerLeftOffset = (this.win32WindowArea.ActualWidth - innerWidth) / 2;
                }





                int videoLeft = (int)((relativeLocation.X + innerLeftOffset) * multiplier);
                int videoTop = (int)((relativeLocation.Y + innerTopOffset) * multiplier);
                int targetLeft = (int)(parentWnd.Left * multiplier) + videoLeft;
                int targetTop = (int)(parentWnd.Top * multiplier) + videoTop;
                int targetWidth = (int)(innerWidth * multiplier);
                int targetHeight = (int)(innerHeight * multiplier);

                SetWindowPos(this._directXWindowHandle, IntPtr.Zero, targetLeft, targetTop, targetWidth, targetHeight, SWP_NOZORDER);
            });
        }

        private void OnFilePlayer_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            this.UpdateDirectXWindowPosition();
        }

        private void OnImageCallback(IntPtr data, int size, int width, int height)
        {
            Task.Run(()=>
            {
                Application.Current.Dispatcher.Invoke(() =>
                {
                    var bitmap = new WriteableBitmap(width, height, 96, 96, PixelFormats.Rgb24, null);
                    bitmap.Lock();
                    int bytesPerPixel = 3;
                    int stride = width * bytesPerPixel;
                    bitmap.WritePixels(new Int32Rect(0, 0, width, height), data, width * height * bytesPerPixel, stride);
                    bitmap.Unlock();
                    this.img_videoImage.Source = bitmap;
                });
            });
        }

        private void OnVideoLengthCallback(double length)
        {
            length = 100;
            Application.Current.Dispatcher.Invoke(() =>
            {
                this.mediaProgressBar.Minimum = 0;
                this.mediaProgressBar.Maximum = length;
            });
            Console.WriteLine("길이:" + length);
        }

        private void OnVideoProgressCallback(double progress)
        {
            Application.Current.Dispatcher.BeginInvoke(() =>
            {
                this.mediaProgressBar.BufferStartValue = progress;
                this.mediaProgressBar.Value = progress;
            });
        }

        private void OnBufferProgressCallback(double progress)
        {
            Application.Current.Dispatcher.BeginInvoke(() =>
            {
                this.mediaProgressBar.BufferEndValue = progress;
                //Console.WriteLine("버퍼:" + progress);
            });
        }

        private void OnBufferStartPosCallback(double progress)
        {
            Application.Current.Dispatcher.Invoke(() =>
            {
                //this.slider_buffer.Value = progress;
            });
            
        }

        private void OnPlayButtonClick(object sender, RoutedEventArgs e)
        {
            if (PlayerStatusService.Instance.PlayerMode == PlayerStatusService.Mode.File)
            {
                if (this._isPaused == true)
                {
                    Play();
                    this.btn_pause.Visibility = Visibility.Visible;
                    this.btn_play.Visibility = Visibility.Hidden;
                }
                else
                {
                    Stop();
                    this.OpenStreamAndRenderer();

                    Play();
                    this.btn_pause.Visibility = Visibility.Visible;
                    this.btn_play.Visibility = Visibility.Hidden;
                }
            }
            else if (PlayerStatusService.Instance.PlayerMode == PlayerStatusService.Mode.Rtsp)
            {
                PlayerStatusService.Instance.PlayRtsp(this._directXWindowHandle);
            }
        }






        private ImageSource? ByteArrayToImageSource(byte[] imageData)
        {
            try
            {
                using (var ms = new MemoryStream(imageData))
                {
                    var image = new BitmapImage();
                    image.BeginInit();
                    image.CacheOption = BitmapCacheOption.OnLoad;
                    image.StreamSource = ms;
                    image.EndInit();
                    image.Freeze(); // WPF의 UI 스레드 외부에서 사용 가능하게 만듦
                    return image;
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine(ex.Message);
                return null;
            }
        }

        private void OnPlayWithSDLButtonClick(object sender, RoutedEventArgs e)
        {
            Task.Run(() =>
            {
                RunDecodeAndSDLPlayExample();
            });
        }

        private void OnPauseButtonClick(object sender, RoutedEventArgs e)
        {
            Pause();
            this.btn_play.Visibility = Visibility.Visible;
            this.btn_pause.Visibility = Visibility.Hidden;
        }

        private object jumpLock = new object();

        private void OnProgressBar_MouseValueChanged(object sender, Common.CustomControl.MouseValueChangedEventArgs e)
        {
            Task.Run(()=>
            {
                lock (this.jumpLock)
                {
                    JumpPlayTime(e.Value);
                }
            });
        }

        private void OnStopButtonClick(object sender, RoutedEventArgs e)
        {
            Stop();
        }

        private void Onbtn_fileOpenClick(object sender, RoutedEventArgs e)
        {
            OpenFileDialog openFileDialog = new OpenFileDialog();
            openFileDialog.Filter = "Video Files|*.mp4;*.avi;*.mkv;*.mov;*.wmv|All Files|*.*";
            if (openFileDialog.ShowDialog() == true)
            {
                PlayerStatusService.Instance.RtspAddress = null;
                PlayerStatusService.Instance.FileAddress = openFileDialog.FileName;
                if (string.IsNullOrWhiteSpace(PlayerStatusService.Instance.FileAddress) == false)
                {
                    Stop();
                }
                this.OpenStreamAndRenderer();
            }
        }

        private void Onbtn_testClick(object sender, RoutedEventArgs e)
        {
            var dxTestWnd = new DirectXTestWindow();
            dxTestWnd.Show();
        }

        private void Onbtn_streamOpenClick(object sender, RoutedEventArgs e)
        {
        }
    }
}
