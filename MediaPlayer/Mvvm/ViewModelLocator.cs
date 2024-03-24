using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace MediaPlayer.Mvvm
{
    public class ViewModelLocator
    {
        public MainWindowViewModel MainWindowViewModel => new MainWindowViewModel();
    }
}
