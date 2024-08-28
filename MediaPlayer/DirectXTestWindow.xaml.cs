using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using System.Windows;
using System.Runtime.InteropServices;
using System.Windows.Interop;
using System.Threading;
using Common.CustomControl;
using System.Windows.Forms.Integration;
using System.Windows.Forms;
using OpenCvSharp.Internal;
using System.Diagnostics;

namespace MediaPlayer
{
    /// <summary>
    /// DirectXTestWindow.xaml에 대한 상호 작용 논리
    /// </summary>
    public partial class DirectXTestWindow : WindowBasic
    {
        const uint SWP_NOZORDER = 0x0004;
        private IntPtr directXWindowHandle = IntPtr.Zero;
        private IntPtr win32ChildHandle = IntPtr.Zero;
        private IntPtr rendererPtr = IntPtr.Zero;
        internal IntPtr handle = IntPtr.Zero;
        private D3DImage? d3dImage = null;

        public DirectXTestWindow()
        {
            InitializeComponent();

            this.Loaded += this.MainWindow_Loaded;
            this.Closed += this.MainWindow_Closed;
            this.LocationChanged += this.MainWindow_LocationChanged;
            this.SizeChanged += this.MainWindow_SizeChanged;
        }

        //[DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        //private static extern IntPtr CreateRenderer(IntPtr hwnd);

        //[DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        //private static extern void DestroyRenderer(IntPtr renderer);

        //[DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        //private static extern void DoRenderFrame(IntPtr renderer);





        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr CreateRenderer(IntPtr hwnd);

        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void DestroyRenderer(IntPtr renderer);

        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void RenderRectangle(IntPtr renderer);

        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void RenderAVFrame(IntPtr renderer, IntPtr frame);

        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr GetSurface(IntPtr renderer);




        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr CreateRenderer_DX11(IntPtr hwnd);

        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr CreateStandAloneRenderer_DX11();

        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void DestroyRenderer_DX11(IntPtr renderer);

        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void RenderFrame_DX11(IntPtr renderer);

        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void RenderRectangle_DX11(IntPtr renderer);

        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr CreateDirectXWindow(IntPtr hInstance, int width, int height, IntPtr parentHwnd);




        [DllImport("user32.dll", SetLastError = true)]
        private static extern bool SetWindowPos(IntPtr hWnd, IntPtr hWndInsertAfter, int X, int Y, int cx, int cy, uint uFlags);

        [DllImport("user32.dll", SetLastError = true, CharSet = CharSet.Auto)]
        private static extern IntPtr CreateWindowEx(
            int dwExStyle,
            string lpClassName,
            string lpWindowName,
            int dwStyle,
            int x,
            int y,
            int nWidth,
            int nHeight,
            IntPtr hWndParent,
            IntPtr hMenu,
            IntPtr hInstance,
            IntPtr lpParam);

        [DllImport("user32.dll", SetLastError = true)]
        private static extern bool DestroyWindow(IntPtr hWnd);


        private void UpdateDirectXWindowPosition()
        {
            // Win32 창의 위치와 크기를 WPF 창에 맞춰 업데이트
            Point relativeLocation = topGrid.TransformToAncestor(this)
                                              .Transform(new Point(0, 0));

            double multiplier = 1.0;
            PresentationSource source = PresentationSource.FromVisual(this);
            if (source != null && source.CompositionTarget != null)
            {
                multiplier = source.CompositionTarget.TransformToDevice.M11;//디스플레이 설정에서 배율 설정값 가져오기
            }
            int videoLeft = (int)(relativeLocation.X * multiplier);
            int videoTop = (int)(relativeLocation.Y * multiplier);
            int targetLeft = (int)(this.Left * multiplier) + videoLeft;
            int targetTop = (int)(this.Top * multiplier) + videoTop;
            int targetWidth = (int)(this.topGrid.ActualWidth * multiplier);
            int targetHeight = (int)(this.topGrid.ActualHeight * multiplier);
            SetWindowPos(this.directXWindowHandle, IntPtr.Zero, targetLeft, targetTop, targetWidth, targetHeight, SWP_NOZORDER);
        }


        private void MainWindow_Loaded(object sender, RoutedEventArgs e)
        {
            // win32 이용 방식
            double multiplier = 1.0;
            PresentationSource source = PresentationSource.FromVisual(this);
            if (source != null && source.CompositionTarget != null)
            {
                multiplier = source.CompositionTarget.TransformToDevice.M11;//디스플레이 설정에서 배율 설정값 가져오기
            }
            int targetWidth = (int)(this.topGrid.ActualWidth * multiplier);
            int targetHeight = (int)(this.topGrid.ActualHeight * multiplier);

            this.directXWindowHandle = CreateDirectXWindow(Process.GetCurrentProcess().Handle, (int)targetWidth, (int)targetHeight, new WindowInteropHelper(this).Handle);

            // 초기 위치와 크기 설정
            this.UpdateDirectXWindowPosition();

            this.rendererPtr = CreateRenderer_DX11(this.directXWindowHandle);
            Task.Run(() =>
            {
                Thread.Sleep(2000);
                RenderRectangle(this.rendererPtr);
            });
        }


        private void MainWindow_Closed(object? sender, EventArgs e)
        {
            DestroyRenderer_DX11(rendererPtr);
        }

        private void MainWindow_LocationChanged(object? sender, EventArgs e)
        {
            UpdateDirectXWindowPosition();
        }

        private void MainWindow_SizeChanged(object? sender, EventArgs e)
        {
            UpdateDirectXWindowPosition();
        }

    }
}
