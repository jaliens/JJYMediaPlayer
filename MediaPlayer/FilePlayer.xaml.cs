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
using System.Windows.Media;
using System.Windows.Media.Imaging;
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

        ImageCallback? _imageCallBack = null;

        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void RegisterImgDecodedCallback(ImageCallback callback);

        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void RunDecodeExample1();

        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void RunDecodeAndSDLPlayExample();

        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void Play();

        [DllImport("VideoModule.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void OpenFileStream();



        public FilePlayer()
        {
            this.InitializeComponent();
            this.Loaded += this.OnFilePlayer_Loaded;
        }

        private void OnFilePlayer_Loaded(object sender, RoutedEventArgs e)
        {
            this._imageCallBack = this.OnImageCallback;
            RegisterImgDecodedCallback(this._imageCallBack);
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
            // Image 컨트롤에 BitmapSource를 할당
            
                this.img_videoImage.Source = bitmap;
            });



            //byte[] managedArray = new byte[size];
            //Marshal.Copy(data, managedArray, 0, size);
            //var imageSource = this.ByteArrayToImageSource(managedArray);
            //if (imageSource == null)
            //{
            //    return;
            //}
            //Application.Current.Dispatcher.Invoke(() => { 
            //    this.img_videoImage.Source = imageSource;
            //});
        }

        private void OnPlayButtonClick(object sender, RoutedEventArgs e)
        {
            Task.Run(()=>
            {
                //RunDecodeExample1();
                OpenFileStream();
                RegisterImgDecodedCallback(this._imageCallBack);
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
    }
}
