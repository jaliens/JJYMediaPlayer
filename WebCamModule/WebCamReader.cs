using OpenCvSharp.WpfExtensions;
using OpenCvSharp;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Media.Imaging;
using System.Windows;
using System.Drawing;
using System.IO;
using System.Drawing.Imaging;
using Common.Util;

namespace WebCamModule
{
    public class WebCamReader
    {
        private VideoCapture? _capture;
        private Mat _frame;
        private Timer _camReadTimer;
        private bool _isCameraRunning = false;

        public delegate void OnVideoCaptureDelegate(BitmapSource bitmapSource);
        public OnVideoCaptureDelegate OnVideoCapture { get; set; }

        public WebCamReader()
        {
        }

        private void OnTimerThick(object? state)
        {
            if (this._capture is null)
            {
                return;
            }

            
            this._capture.Read(this._frame);
            if (this._frame.Empty() == false)
            {
                BitmapSource bitmapSrc = BitmapSourceConverter.ToBitmapSource(this._frame);
                //Bitmap bitmap = Util.BitmapSourceToBitmap(bitmapSrc);
                this.OnVideoCapture(bitmapSrc);
            }
        }

        public void StartCamera()
        {
            if (this._isCameraRunning)
                return;

            this._capture = new VideoCapture(0);
            this._capture.Open(0);
            if (this._capture.IsOpened() == false)
            {
                MessageBox.Show("카메라 열기 실패");
                return;
            }

            this._frame = new Mat();
            this._isCameraRunning = true;
            TimerCallback timerCallback = new TimerCallback(this.OnTimerThick);
            this._camReadTimer = new Timer(timerCallback, null, 0, 33);
        }

        public void StopCamera()
        {
            this._camReadTimer?.Dispose();
            this._capture?.Release();
            this._isCameraRunning = false;
        }



        
    }
}
