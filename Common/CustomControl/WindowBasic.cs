using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Shapes;

namespace Common.CustomControl
{
    public class WindowBasic : Window
    {
        private Grid? _titleBarGrid = null;
        private Button? _btn_minimize = null;
        private Button? _btn_maximize = null;
        private Button? _btn_close = null;
        private Thumb? _resizeBar_left = null;
        private Thumb? _resizeBar_right = null;
        private Thumb? _resizeBar_top = null;
        private Thumb? _resizeBar_bottom = null;
        private Thumb? _resizeBar_rightTop = null;
        private Thumb? _resizeBar_rightBottom = null;
        private Thumb? _resizeBar_leftBottom = null;
        private Thumb? _resizeBar_leftTop = null;

        public static readonly DependencyProperty ResizeBarBrushProperty =
            DependencyProperty.Register(
                "ResizeBarBrush",
                typeof(Brush),
                typeof(WindowBasic),
                new FrameworkPropertyMetadata(Brushes.DarkOrange, FrameworkPropertyMetadataOptions.AffectsRender));
        public Brush ResizeBarBrush
        {
            get { return (Brush)GetValue(ResizeBarBrushProperty); }
            set { SetValue(ResizeBarBrushProperty, value); }
        }

        public static readonly DependencyProperty ResizeBarOpacityProperty =
            DependencyProperty.Register(
                "ResizeBarOpacity",
                typeof(double),
                typeof(WindowBasic),
                new FrameworkPropertyMetadata(0.5, FrameworkPropertyMetadataOptions.AffectsRender));
        public double ResizeBarOpacity
        {
            get { return (double)GetValue(ResizeBarOpacityProperty); }
            set { SetValue(ResizeBarOpacityProperty, value); }
        }

        public override void OnApplyTemplate()
        {
            base.OnApplyTemplate();

            this._titleBarGrid = this.GetTemplateChild("titleBar") as Grid;
            this._btn_minimize = this.GetTemplateChild("btn_minimize") as Button;
            this._btn_maximize = this.GetTemplateChild("btn_maximize") as Button;
            this._btn_close = this.GetTemplateChild("btn_close") as Button;

            this._resizeBar_left = this.GetTemplateChild("resizeBar_left") as Thumb;
            this._resizeBar_right = this.GetTemplateChild("resizeBar_right") as Thumb;
            this._resizeBar_top = this.GetTemplateChild("resizeBar_top") as Thumb;
            this._resizeBar_bottom = this.GetTemplateChild("resizeBar_bottom") as Thumb;

            this._resizeBar_rightTop = this.GetTemplateChild("resizeBar_rightTop") as Thumb;
            this._resizeBar_rightBottom = this.GetTemplateChild("resizeBar_rightBottom") as Thumb;
            this._resizeBar_leftBottom = this.GetTemplateChild("resizeBar_leftBottom") as Thumb;
            this._resizeBar_leftTop = this.GetTemplateChild("resizeBar_leftTop") as Thumb;

            if (this._resizeBar_rightTop != null)
            {
                this._resizeBar_rightTop.DragDelta += this.On_resizeBar_rightTop_DragDelta;
            }
            if (this._resizeBar_rightBottom != null)
            {
                this._resizeBar_rightBottom.DragDelta += this.On_resizeBar_rightBottom_DragDelta;
            }
            if (this._resizeBar_leftBottom != null)
            {
                this._resizeBar_leftBottom.DragDelta += this.On_resizeBar_leftBottom_DragDelta;
            }
            if (this._resizeBar_leftTop != null)
            {
                this._resizeBar_leftTop.DragDelta += this.On_resizeBar_leftTop_DragDelta;
            }

            if (this._titleBarGrid != null)
            {
                this._titleBarGrid.MouseLeftButtonDown += this.On_titleBarGrid_MouseLeftButtonDown;
            }
            if (this._btn_minimize != null)
            {
                this._btn_minimize.Click += this.On_btn_minimize_Click;
            }
            if (this._btn_maximize != null)
            {
                this._btn_maximize.Click += this.On_btn_maximize_Click;
            }
            if (this._btn_close != null)
            {
                this._btn_close.Click += this.On_btn_close_Click;
            }

            if (this._resizeBar_left != null)
            {
                this._resizeBar_left.DragDelta += this.On_resizeBar_left_DragDelta;
            }
            if (this._resizeBar_right != null)
            {
                this._resizeBar_right.DragDelta += this.On_resizeBar_right_DragDelta;
            }
            if (this._resizeBar_top != null)
            {
                this._resizeBar_top.DragDelta += this.On_resizeBar_top_DragDelta;
            }
            if (this._resizeBar_bottom != null)
            {
                this._resizeBar_bottom.DragDelta += this.On_resizeBar_bottom_DragDelta;
            }


        }

        private void On_resizeBar_rightTop_DragDelta(object sender, DragDeltaEventArgs e)
        {
            if (this.WindowState == WindowState.Maximized)
            {
                return;
            }

            double newWidth = Math.Max(this.MinWidth, this.Width + e.HorizontalChange);
            if (newWidth > 13)
            {
                this.Width = newWidth;
            }
            double newHeight = this.Height - e.VerticalChange;
            if (newHeight > this.MinHeight && newHeight > 13)
            {
                this.Height = newHeight;
                this.Top += e.VerticalChange;
            }
        }

        private void On_resizeBar_rightBottom_DragDelta(object sender, DragDeltaEventArgs e)
        {
            if (this.WindowState == WindowState.Maximized)
            {
                return;
            }

            this.Width = Math.Max(this.MinWidth, this.Width + e.HorizontalChange);
            this.Height = Math.Max(this.MinHeight, this.Height + e.VerticalChange);
        }

        private void On_resizeBar_leftBottom_DragDelta(object sender, DragDeltaEventArgs e)
        {
            if (this.WindowState == WindowState.Maximized)
            {
                return;
            }

            double newWidth = this.Width - e.HorizontalChange;
            if (newWidth > this.MinWidth && newWidth > 13)
            {
                this.Width = newWidth;
                this.Left += e.HorizontalChange;
            }
            this.Height = Math.Max(this.MinHeight, this.Height + e.VerticalChange);
        }

        private void On_resizeBar_leftTop_DragDelta(object sender, DragDeltaEventArgs e)
        {
            if (this.WindowState == WindowState.Maximized)
            {
                return;
            }

            double newWidth = this.Width - e.HorizontalChange;
            if (newWidth > this.MinWidth && newWidth > 13)
            {
                this.Width = newWidth;
                this.Left += e.HorizontalChange;
            }
            double newHeight = this.Height - e.VerticalChange;
            if (newHeight > this.MinHeight && newHeight > 13)
            {
                this.Height = newHeight;
                this.Top += e.VerticalChange;
            }
        }

        private void On_resizeBar_left_DragDelta(object sender, DragDeltaEventArgs e)
        {
            if (this.WindowState == WindowState.Maximized)
            {
                return;
            }

            double newWidth = this.Width - e.HorizontalChange;
            if (newWidth > this.MinWidth && newWidth > 13)
            {
                this.Width = newWidth;
                this.Left += e.HorizontalChange;
            }
        }

        private void On_resizeBar_right_DragDelta(object sender, DragDeltaEventArgs e)
        {
            if (this.WindowState == WindowState.Maximized)
            {
                return;
            }

            double newWidth = Math.Max(this.MinWidth, this.Width + e.HorizontalChange);
            if (newWidth > 13)
            {
                this.Width = newWidth;
            }
        }

        private void On_resizeBar_top_DragDelta(object sender, DragDeltaEventArgs e)
        {
            if (this.WindowState == WindowState.Maximized)
            {
                return;
            }

            double newHeight = this.Height - e.VerticalChange;
            if (newHeight > this.MinHeight && newHeight > 13)
            {
                this.Height = newHeight;
                this.Top += e.VerticalChange;
            }
        }

        private void On_resizeBar_bottom_DragDelta(object sender, DragDeltaEventArgs e)
        {
            if (this.WindowState == WindowState.Maximized)
            {
                return;
            }

            double newHeight = Math.Max(this.MinHeight, this.Height + e.VerticalChange);
            if (newHeight > 13)
            {
                this.Height = newHeight;
            }
        }

        private void On_btn_minimize_Click(object sender, RoutedEventArgs e)
        {
            this.WindowState = WindowState.Minimized;
        }

        private void On_btn_maximize_Click(object sender, RoutedEventArgs e)
        {
            if (this.WindowState == WindowState.Maximized)
            {
                this.WindowState = WindowState.Normal;
            }
            else
            {
                this.WindowState = WindowState.Maximized;
            }
        }

        private void On_btn_close_Click(object sender, RoutedEventArgs e)
        {
            this.Close();
        }

        /// <summary>
        /// 타이틀바 마우스 다운
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void On_titleBarGrid_MouseLeftButtonDown(object sender, System.Windows.Input.MouseButtonEventArgs e)
        {
            //마우스 드래그 창 이동 처리
            if (e.ClickCount < 2)
            {
              this.DragMove();
            }

            //더블클릭 시 최대화
            if (e.ClickCount == 2)
            {
                if (this.WindowState == WindowState.Maximized)
                {
                    this.WindowState = WindowState.Normal;
                }
                else
                {
                    this.WindowState = WindowState.Maximized;
                }
            }
        }

    }
}
