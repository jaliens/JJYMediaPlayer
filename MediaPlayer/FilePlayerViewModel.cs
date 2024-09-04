using Common.CustomControl;
using Common.Mvvm;
using MediaPlayer.Service;
using Microsoft.Win32;
using System;
using System.Windows.Input;

namespace MediaPlayer
{
    public class FilePlayerViewModel : ViewModelBase
    {
        private double _ProgressBarMax = 100d;
        public double ProgressBarMax
        {
            get
            {
                return this._ProgressBarMax;
            }
            set
            {
                this._ProgressBarMax = value;
                SetProperty(nameof(this.ProgressBarMax));
            }
        }

        private double _ProgressBarMin = 0d;
        public double ProgressBarMin
        {
            get
            {
                return this._ProgressBarMin;
            }
            set
            {
                this._ProgressBarMin = value;
                SetProperty(nameof(this.ProgressBarMin));
            }
        }

        private double _ProgressBarValue = 0d;
        public double ProgressBarValue
        {
            get
            {
                return this._ProgressBarValue;
            }
            set
            {
                this._ProgressBarValue = value;
                SetProperty(nameof(this.ProgressBarValue));
            }
        }

        private double _ProgressBarBufferLeftBoundary = 0d;
        public double ProgressBarBufferLeftBoundary
        {
            get
            {
                return this._ProgressBarBufferLeftBoundary;
            }
            set
            {
                this._ProgressBarBufferLeftBoundary = value;
                SetProperty(nameof(this.ProgressBarBufferLeftBoundary));
            }
        }

        private double _ProgressBarBufferRightBoundary = 0d;
        public double ProgressBarBufferRightBoundary
        {
            get
            {
                return this._ProgressBarBufferRightBoundary;
            }
            set
            {
                this._ProgressBarBufferRightBoundary = value;
                SetProperty(nameof(this.ProgressBarBufferRightBoundary));
            }
        }


        public ICommand? LoadedCommand { get; set; } = null;
        public ICommand? ShowFileOpenWindowCommand { get; set; } = null;
        public ICommand? ShowStreamOpenWindowCommand { get; set; } = null;
        public ICommand? PlayCommand { get; set; } = null;
        public ICommand? PauseCommand { get; set; } = null;
        public ICommand? StopCommand { get; set; } = null;
        public ICommand? SeekCommand { get; set; } = null;
        
        public FilePlayerViewModel()
        {
            this.InitCommands();
        }

        public void InitCommands()
        {
            this.LoadedCommand ??= new RelayCommand(this.ProcLoadedCommand);
            this.ShowFileOpenWindowCommand ??= new RelayCommand(this.ProcShowFileOpenWindowCommand);
            this.ShowStreamOpenWindowCommand ??= new RelayCommand(this.ProcShowStreamOpenWindowCommand);
            this.PlayCommand ??= new RelayCommand(this.ProcPlayCommand);
            this.PauseCommand ??= new RelayCommand(this.ProcPauseCommand);
            this.StopCommand ??= new RelayCommand(this.ProcStopCommand);
            this.SeekCommand ??= new RelayCommand(this.ProcSeekCommand);
        }

        private void ProcSeekCommand(object? obj)
        {
            var arg = obj as MouseValueChangedEventArgs;
            if (arg == null)
            {
                return;
            }

            PlayerStatusService.Instance.JumpPlayTime(arg.Value);
        }


        private void ProcLoadedCommand(object? obj)
        {
            PlayerStatusService.Instance.CreatePlayer();
            PlayerStatusService.Instance.RegisterOnVideoLengthCallback(this.OnVideoLengthCallback);
            PlayerStatusService.Instance.RegisterOnVideoProgressCallback(this.OnVideoProgressCallback);
            PlayerStatusService.Instance.RegisterOnBufferProgressCallback(this.OnBufferProgressCallback);
            PlayerStatusService.Instance.RegisterOnPauseCallback(this.OnPauseCallback);
            PlayerStatusService.Instance.RegisterOnResumeCallback(this.OnResumeCallback);
            PlayerStatusService.Instance.RegisterOnStopCallback(this.OnStopCallback);
            PlayerStatusService.Instance.RegisterOnRenderTimingCallback(this.OnRenderTimingCallback);
        }

        private void OnVideoLengthCallback(double length)
        {
            this.ProgressBarMax = 100;
            this.ProgressBarMin = 0;
        }

        private void OnVideoProgressCallback(double progress)
        {
            progress = Math.Max(0, progress);
            progress = Math.Min(progress,100);
            this.ProgressBarValue = progress;
            this.ProgressBarBufferLeftBoundary = progress;
        }

        private void OnBufferProgressCallback(double progress)
        {
            progress = Math.Max(this.ProgressBarValue, progress);
            progress = Math.Min(progress, 100);
            this.ProgressBarBufferRightBoundary = progress;
        }

        private void OnPauseCallback()
        {
        }

        private void OnResumeCallback()
        {
        }

        private void OnStopCallback()
        {
        }

        private void OnRenderTimingCallback()
        {
        }

        private void ProcStopCommand(object obj)
        {
            PlayerStatusService.Instance.Stop();
        }

        private void ProcShowFileOpenWindowCommand(object? obj)
        {
            OpenFileDialog openFileDialog = new OpenFileDialog();
            openFileDialog.Filter = "Video Files|*.mp4;*.avi;*.mkv;*.mov;*.wmv|All Files|*.*";
            if (openFileDialog.ShowDialog() == true)
            {
                PlayerStatusService.Instance.FileAddress = openFileDialog.FileName;
            }
        }

        private void ProcPauseCommand(object? obj)
        {
            PlayerStatusService.Instance.Pause();
        }

        private void ProcPlayCommand(object? obj)
        {
            if (PlayerStatusService.Instance.PlayerMode == PlayerStatusService.Mode.File)
            {
                PlayerStatusService.Instance.Play();
            }
            else if (PlayerStatusService.Instance.PlayerMode == PlayerStatusService.Mode.Rtsp)
            {
                PlayerStatusService.Instance.PlayRtsp();
            }
        }

        private void ProcShowStreamOpenWindowCommand(object? param)
        {
            App.DialogService.ShowDialog("StreamOpenWindowViewModel", out bool dialogResult);
        }
    }
}
