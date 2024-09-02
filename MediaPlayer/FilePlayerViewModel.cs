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
    public class FilePlayerViewModel : ViewModelBase
    {

        public ICommand? ShowStreamOpenWindowCommand { get; set; } = null;

        public FilePlayerViewModel()
        {
            this.InitCommands();
        }

        public void InitCommands()
        {
            this.ShowStreamOpenWindowCommand ??= new RelayCommand(this.ProcShowStreamOpenWindowCommand);
        }

        private void ProcShowStreamOpenWindowCommand(object? param)
        {
            App.DialogService.ShowDialog("StreamOpenWindowViewModel", out bool dialogResult);
        }
    }
}
