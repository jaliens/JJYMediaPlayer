using Common.Mvvm;
using MediaPlayer.Service;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Input;

namespace MediaPlayer
{
    public class StreamOpenWindowViewModel : ViewModelBase
    {


        private string _RTSPAddress = "rtsp://localhost:8554/sss";//string.Empty;
        /// <summary>
        /// RTSP서버 주소(영상 스트리밍의 소스)
        /// </summary>
        public string RTSPAddress
        {
            get
            {
                return this._RTSPAddress;
            }
            set
            {
                this._RTSPAddress = value;
                SetProperty(nameof(this.RTSPAddress));
            }
        }

        public ICommand? ConfirmRTSPAddressCommand { get; set; } = null;

        public StreamOpenWindowViewModel()
        {
            this.InitCommands();
        }

        void InitCommands()
        {
            this.ConfirmRTSPAddressCommand ??= new RelayCommand(this.ProcConfirmRTSPAddressCommand);
        }

        private void ProcConfirmRTSPAddressCommand(object? obj)
        {
            PlayerStatusService.Instance.RtspAddress = this.RTSPAddress;
            App.DialogService.CloseDialog(nameof(StreamOpenWindowViewModel),true);
        }
    }
}
