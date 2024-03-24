using System;
using System.Collections.Generic;
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

namespace MediaPlayer
{
    /// <summary>
    /// FilePlayer.xaml에 대한 상호 작용 논리
    /// </summary>
    public partial class FilePlayer : UserControl
    {
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        private delegate void ImageCallback(IntPtr data, int size, int width, int height);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        private delegate void OnVideoLengthCallbackFunction(double length);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        private delegate void OnVideoProgressCallbackFunction(double progress);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        private delegate void OnStartCallbackFunction();
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        private delegate void OnPauseCallbackFunction();
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        private delegate void OnResumeCallbackFunction();

        ImageCallback? _imageCallBack = null;
        OnVideoLengthCallbackFunction? _videoLengthCallback = null;
        OnVideoProgressCallbackFunction? _videoProgressCallback = null;
        OnStartCallbackFunction? _startCallback = null;
        OnPauseCallbackFunction? _pauseCallback = null;
        OnResumeCallbackFunction? _resumeCallback = null;






        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void RegisterOnImgDecodeCallback(ImageCallback callback);

        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void RegisterOnVideoLengthCallback(OnVideoLengthCallbackFunction callback);

        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void RegisterOnVideoProgressCallback(OnVideoProgressCallbackFunction callback);

        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void RegisterOnStartCallback(OnStartCallbackFunction callback);

        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void RegisterOnPauseCallback(OnPauseCallbackFunction callback);

        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void RegisterOnResumeCallback(OnResumeCallbackFunction callback);





        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void CreatePlayer();

        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void OpenFileStream();

        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void Play();

        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void Pause();

        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void Stop();

        






        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void RunDecodeExample1();

        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void RunDecodeAndSDLPlayExample();





















        public FilePlayer()
        {
            this.InitializeComponent();
            this.Loaded += this.OnFilePlayer_Loaded;

            this._imageCallBack = this.OnImageCallback;
            this._videoLengthCallback = this.OnVideoLengthCallback;
            this._videoProgressCallback = this.OnVideoProgressCallback;
        }

        

        private void OnFilePlayer_Loaded(object sender, RoutedEventArgs e)
        {
            CreatePlayer();
            RegisterOnImgDecodeCallback(this._imageCallBack);
            RegisterOnVideoLengthCallback(this._videoLengthCallback);
            RegisterOnVideoProgressCallback(this._videoProgressCallback);
            OpenFileStream();
            GC.KeepAlive(this._imageCallBack);
        }

        private void OnImageCallback(IntPtr data, int size, int width, int height)
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
        }

        private void OnVideoLengthCallback(double length)
        {
            this.videoSlider.Maximum = length;
        }

        private void OnVideoProgressCallback(double progress)
        {
            Application.Current.Dispatcher.Invoke(() =>
            {
                this.videoSlider.Value = progress;
            });
            
        }

        private void OnPlayButtonClick(object sender, RoutedEventArgs e)
        {
            Task.Run(()=>
            {
                Play();
            });
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
        }
    }
}
