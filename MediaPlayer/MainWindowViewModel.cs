using Common.Mvvm;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace MediaPlayer
{
    public class MainWindowViewModel : ViewModelBase
    {
        private bool _isShowWebcamPlayer = false;
        public bool IsShowWebcamPlayer
        {
            get
            {
                return this._isShowWebcamPlayer;
            }
            set
            {
                this._isShowWebcamPlayer = value;
                SetProperty(nameof(this.IsShowWebcamPlayer));
            }
        }


        private bool _isShowFilePlayer = true;
        public bool IsShowFilePlayer
        {
            get
            {
                return this._isShowFilePlayer;
            }
            set
            {
                this._isShowFilePlayer = value;
                SetProperty(nameof(this.IsShowFilePlayer));
            }
        }

        public MainWindowViewModel()
        {
            this.InitCommands();
        }

        public void InitCommands()
        {
        }
    }
}
