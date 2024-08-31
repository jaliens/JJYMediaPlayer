using Common.Mvvm;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;

namespace MediaPlayer.Mvvm
{
    public class DialogService : IDialogService
    {
        public void ShowDialog(string viewModelName)
        {
            Window? window = null;
            switch (viewModelName)
            {
                case "MainWindowViewModel":
                    window = new MainWindow();
                    break;
                case "StreamOpenWindowViewModel":
                    window = new StreamOpenWindow();
                    break;
            }

            window?.ShowDialog();
        }

        public bool ShowDialog(string viewModelName, string message)
        {
            return true;
        }
    }
}
