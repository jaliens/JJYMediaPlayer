using System;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Interop;
using MediaPlayer.Service;
using CommunityToolkit.Mvvm.Messaging;
using MediaPlayer.Message;

namespace MediaPlayer
{
    /// <summary>
    /// FilePlayer.xaml에 대한 상호 작용 논리
    /// </summary>
    public partial class FilePlayer : UserControl
    {
        const uint SWP_NOZORDER = 0x0004;

        private int _videoWidth = 0;
        private int _videoHeight = 0;
        private IntPtr _win32WindowHandle = IntPtr.Zero;

        [DllImport("user32.dll", SetLastError = true)]
        private static extern bool SetWindowPos(IntPtr hWnd, IntPtr hWndInsertAfter, int X, int Y, int cx, int cy, uint uFlags);


        public FilePlayer()
        {
            this.InitializeComponent();
            this.Loaded += this.OnFilePlayer_Loaded;
            this.SizeChanged += OnFilePlayer_SizeChanged;

            this.RegisterToMessage();
        }

        private void OnFilePlayer_Loaded(object sender, RoutedEventArgs e)
        {
            this.CreateWin32Window();
            PlayerStatusService.Instance.RegisterWindowHandle(this._win32WindowHandle);
            PlayerStatusService.Instance.RegisterOnVideoSizeCallback(this.OnVideoSizeCallback);

            var parentWnd = Window.GetWindow(this);
            parentWnd.LocationChanged += OnParentWnd_LocationChanged;
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
                    }
                    break;
                default:
                    break;
            }
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

        private void OnParentWnd_LocationChanged(object? sender, EventArgs e)
        {
            this.UpdateDirectXWindowPosition();
        }

        private void OnFilePlayer_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            this.UpdateDirectXWindowPosition();
        }

        /// <summary>
        /// directx 11 비디오 랜더링을 위한 win32 창 생성<br/>
        /// </summary>
        private void CreateWin32Window()
        {
            if (this._win32WindowHandle != IntPtr.Zero)
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

            this._win32WindowHandle = PlayerStatusService.Instance.CreateDirectXWindow(Process.GetCurrentProcess().Handle, (int)targetWidth, (int)targetHeight, new WindowInteropHelper(parentWnd).Handle);
            this.UpdateDirectXWindowPosition();
        }

        /// <summary>
        /// Win32 창의 위치와 크기를 WPF 창에 맞춰 업데이트
        /// </summary>
        private void UpdateDirectXWindowPosition()
        {
            Application.Current?.Dispatcher.Invoke(() =>
            {
                if (this._win32WindowHandle == IntPtr.Zero)
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

                SetWindowPos(this._win32WindowHandle, IntPtr.Zero, targetLeft, targetTop, targetWidth, targetHeight, SWP_NOZORDER);
            });
        }

        private void RegisterToMessage()
        {
            WeakReferenceMessenger.Default.Register<VideoSourceSetMessage>(this, this.OnVideoSourceSetMessage);
            WeakReferenceMessenger.Default.Register<PlayserStatusMessage>(this, this.OnPlayserStatusMessage);
        }
    }
}
