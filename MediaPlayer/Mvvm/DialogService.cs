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
        private Dictionary<string,Window> _viewModelNameWindowPair = new Dictionary<string,Window>();

        public void ShowDialog(string viewModelName, out bool dialogResult)
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

            if (window != null)
            {
                this._viewModelNameWindowPair.TryAdd(viewModelName, window);
            }

            bool? result = window?.ShowDialog();
            dialogResult = result != null ? (bool)result : false;
        }

        public void CloseDialog(string viewModelName, bool dialogResult)
        {
            if (string.IsNullOrWhiteSpace(viewModelName))
            {
                return;
            }

            if (this._viewModelNameWindowPair.TryGetValue(viewModelName, out Window? window) == false)
            {
                return;
            }
            this._viewModelNameWindowPair.Remove(viewModelName);

            window.DialogResult = dialogResult;
        }
    }
}
