using Common.Util;
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using WebCamModule;

namespace MediaPlayer
{
    /// <summary>
    /// WebCamPlayer.xaml에 대한 상호 작용 논리
    /// </summary>
    public partial class WebCamPlayer : UserControl
    {
        WebCamReader _webCamReader = new WebCamReader();

        public WebCamPlayer()
        {
            InitializeComponent();
            this.Loaded += OnWebCamPlayer_Loaded;
        }

        public void HandleVideoCapture(BitmapSource bitmapSource) 
        {
            bitmapSource.Freeze();
            Application.Current.Dispatcher.Invoke(() =>
            {
                this.img_videoImage.Source = bitmapSource;
            });
        }

        private void OnWebCamPlayer_Loaded(object sender, RoutedEventArgs e)
        {
            this._webCamReader.OnVideoCapture = this.HandleVideoCapture;
        }

        private void OnOpenCamButtonClick(object sender, RoutedEventArgs e)
        {
            this._webCamReader.StartCamera();
            this.btn_openCam.Visibility = Visibility.Collapsed;
            this.btn_closeCam.Visibility = Visibility.Visible;
        }

        private void OnCloseCamButtonClick(object sender, RoutedEventArgs e)
        {
            this._webCamReader.StopCamera();
            this.btn_openCam.Visibility = Visibility.Visible;
            this.btn_closeCam.Visibility = Visibility.Collapsed;
        }
    }
}
