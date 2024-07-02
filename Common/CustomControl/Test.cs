using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

namespace Common.CustomControl
{
    public class Test : Slider
    {
        static Test()
        {
            DefaultStyleKeyProperty.OverrideMetadata(typeof(Test), new FrameworkPropertyMetadata(typeof(Test)));
        }

        public override void OnApplyTemplate()
        {
            base.OnApplyTemplate();
        }

        private void OnPreviewMouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            //base.OnPreviewMouseLeftButtonDown(e);
            //e.Handled = true;
        }

        private void OnMouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {

        }
    }
}
