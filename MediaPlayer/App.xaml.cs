using MediaPlayer.Mvvm;
using System;
using System.Collections.Generic;
using System.Configuration;
using System.Data;
using System.Linq;
using System.Threading.Tasks;
using System.Windows;

namespace MediaPlayer
{
    /// <summary>
    /// Interaction logic for App.xaml
    /// </summary>
    public partial class App : Application
    {
        public static DialogService DialogService { get; } = new DialogService();

        public App()
        {
            this.Startup += OnStartup;
            
        }

        private void OnStartup(object sender, StartupEventArgs e)
        {
            DialogService.ShowDialog("MainWindowViewModel");
        }
    }
}
