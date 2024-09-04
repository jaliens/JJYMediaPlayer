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
    public class MediaProgressBar : RangeBase
    {
        private Ellipse? _thumb = null;
        private Canvas? _canvas = null;
        private bool _isMouseCaptured = false;
        private double? _draggingValue = 0;

        static MediaProgressBar()
        {
            //RangeBase의 기본 스타일이 아닌 MediaProgressBar의 스타일을 사용하기 위해 호출
            DefaultStyleKeyProperty.OverrideMetadata(typeof(MediaProgressBar), new FrameworkPropertyMetadata(typeof(MediaProgressBar)));
        }


        #region property
        public static readonly DependencyProperty BufferStartValueProperty =
            DependencyProperty.Register(
                "BufferStartValue",
                typeof(double),
                typeof(MediaProgressBar),
                new FrameworkPropertyMetadata(0d, FrameworkPropertyMetadataOptions.AffectsRender));
        public double BufferStartValue
        {
            get { return (double)GetValue(BufferStartValueProperty); }
            set { SetValue(BufferStartValueProperty, value); }
        }

        public static readonly DependencyProperty BufferEndValueProperty =
            DependencyProperty.Register(
                "BufferEndValue",
                typeof(double),
                typeof(MediaProgressBar),
                new FrameworkPropertyMetadata(0d, FrameworkPropertyMetadataOptions.AffectsRender));
        public double BufferEndValue
        {
            get { return (double)GetValue(BufferEndValueProperty); }
            set { SetValue(BufferEndValueProperty, value); }
        }

        public static readonly DependencyProperty BufferBrushProperty =
            DependencyProperty.Register(
                "BufferBrush",
                typeof(Brush),
                typeof(MediaProgressBar),
                new FrameworkPropertyMetadata(Brushes.LightGray, FrameworkPropertyMetadataOptions.AffectsRender));
        public Brush BufferBrush
        {
            get { return (Brush)GetValue(BufferBrushProperty); }
            set { SetValue(BufferBrushProperty, value); }
        }

        public static readonly DependencyProperty ThumbBrushProperty =
            DependencyProperty.Register(
                "ThumbBrush",
                typeof(Brush),
                typeof(MediaProgressBar),
                new FrameworkPropertyMetadata(Brushes.White, FrameworkPropertyMetadataOptions.AffectsRender));
        public Brush ThumbBrush
        {
            get { return (Brush)GetValue(ThumbBrushProperty); }
            set { SetValue(ThumbBrushProperty, value); }
        }

        public static readonly DependencyProperty ThumbRadiusProperty =
            DependencyProperty.Register(
                "ThumbRadius",
                typeof(double),
                typeof(MediaProgressBar),
                new FrameworkPropertyMetadata(5d, FrameworkPropertyMetadataOptions.AffectsRender));
        public double ThumbRadius
        {
            get { return (double)GetValue(ThumbRadiusProperty); }
            set { SetValue(ThumbRadiusProperty, value); }
        }

        public static readonly DependencyProperty BarThicknessProperty =
            DependencyProperty.Register(
                "BarThickness",
                typeof(double),
                typeof(MediaProgressBar),
                new FrameworkPropertyMetadata(5d, FrameworkPropertyMetadataOptions.AffectsRender));
        public double BarThickness
        {
            get { return (double)GetValue(BarThicknessProperty); }
            set { SetValue(BarThicknessProperty, value); }
        }
        #endregion property


        #region event
        public static readonly RoutedEvent MouseValueChangedEvent = EventManager.RegisterRoutedEvent(
            "MouseValueChanged", RoutingStrategy.Bubble, typeof(EventHandler<MouseValueChangedEventArgs>), typeof(MediaProgressBar));

        public event EventHandler<MouseValueChangedEventArgs> MouseValueChanged
        {
            add { AddHandler(MouseValueChangedEvent, value); }
            remove { RemoveHandler(MouseValueChangedEvent, value); }
        }
        #endregion event



        public override void OnApplyTemplate()
        {
            base.OnApplyTemplate();

            this._thumb = this.GetTemplateChild("thumbEllipse") as Ellipse;
            this._canvas = this.GetTemplateChild("canvas") as Canvas;

            if (this._thumb != null)
            {
                this._thumb.Fill = this.ThumbBrush;
                this._thumb.Width = this.ThumbRadius*2;
                this._thumb.Height = this.ThumbRadius*2;
            }

            if (this._canvas != null)
            {
                this._canvas.PreviewMouseDown += this.On_canvas_PreviewMouseDown;
                this._canvas.PreviewMouseMove += this.On_canvas_PreviewMouseMove;
                this._canvas.PreviewMouseUp += this.On_canvas_PreviewMouseUp;
            }
        }

        protected override Size MeasureOverride(Size constraint)
        {
            return base.MeasureOverride(constraint);
        }

        protected override Size ArrangeOverride(Size arrangeBounds)
        {
            return base.ArrangeOverride(arrangeBounds);
        }

        protected override void OnRender(DrawingContext drawingContext)
        {
            base.OnRender(drawingContext);

            double progressBarMin = this.Minimum;
            double progressBarMax = Math.Max(this.Maximum, this.Minimum);
            double bufferStart = this.BufferStartValue;
            double bufferEnd = this.BufferEndValue;
            double value = this.Value;
            double barY = this.RenderSize.Height / 2d - this.BarThickness / 2d;

            //배경 막대 그리기
            Rect barBackgroundRect = new Rect()
            {
                X = 0,
                Y = barY,
                Width = this.RenderSize.Width,
                Height = this.BarThickness,
            };
            drawingContext.DrawRectangle(this.Background, null, barBackgroundRect);

            //버퍼 상태 막대 그리기
            if (bufferEnd > bufferStart)
            {
                double bufferBarWidth = (bufferEnd - bufferStart) / (progressBarMax - progressBarMin) * this.RenderSize.Width;
                double bufferStartX = bufferStart / (progressBarMax - progressBarMin) * this.RenderSize.Width;
                if (bufferStartX < 0)
                {
                    bufferStartX = 0;
                }
                if (bufferStartX + bufferBarWidth > this.RenderSize.Width)
                {
                    bufferBarWidth -= bufferStartX + bufferBarWidth - this.RenderSize.Width;
                }
                Rect bufferRect = new Rect()
                {
                    X = bufferStartX,
                    Y = barY,
                    Width = bufferBarWidth == 0 ? 0 : bufferBarWidth,
                    Height = this.BarThickness,
                };
                drawingContext.DrawRectangle(this.BufferBrush, null, bufferRect);
            }

            //현재 값 표시기 위치 결정
            if (this._isMouseCaptured == true)
            {

            }
            else if (progressBarMax > progressBarMin &&
                value <= progressBarMax &&
                value >= progressBarMin)
            {
                double centerX = (value / (progressBarMax - progressBarMin)) * this.RenderSize.Width;
                double centerY = this.RenderSize.Height / 2d;
                double thumbX = centerX - this.ThumbRadius;
                double thumbY = centerY - this.ThumbRadius;
                this._thumb?.SetValue(Canvas.LeftProperty, thumbX);
                this._thumb?.SetValue(Canvas.TopProperty, thumbY);
            }
            else
            {
                this._thumb?.SetValue(Canvas.LeftProperty, - this.ThumbRadius);
                this._thumb?.SetValue(Canvas.TopProperty, - this.ThumbRadius);
            }

            if (this._thumb != null)
            {
                this._thumb.Fill = this.ThumbBrush;
                this._thumb.Width = this.ThumbRadius * 2;
                this._thumb.Height = this.ThumbRadius * 2;
            }
        }

        private void On_canvas_PreviewMouseDown(object sender, System.Windows.Input.MouseButtonEventArgs e)
        {
            if (this._canvas == null)
            {
                return;
            }

            var point = e.GetPosition(this._canvas);
            double pointX = point.X;
            double progressBarMin = this.Minimum;
            double progressBarMax = Math.Max(this.Maximum, this.Minimum);

            //현재 값 표시기 위치 결정
            if (progressBarMax > progressBarMin)
            {
                double centerX = pointX;
                double centerY = this.RenderSize.Height / 2d;
                double thumbX = centerX - this.ThumbRadius;
                double thumbY = centerY - this.ThumbRadius;
                this._thumb?.SetValue(Canvas.LeftProperty, thumbX);
                this._thumb?.SetValue(Canvas.TopProperty, thumbY);
            }
            else
            {
                this._thumb?.SetValue(Canvas.LeftProperty, -this.ThumbRadius);
                this._thumb?.SetValue(Canvas.TopProperty, -this.ThumbRadius);
            }

            if (this._thumb != null)
            {
                this._thumb.Fill = this.ThumbBrush;
                this._thumb.Width = this.ThumbRadius * 2;
                this._thumb.Height = this.ThumbRadius * 2;
            }

            double valueRange = progressBarMax - progressBarMin;
            this._draggingValue = progressBarMin + valueRange * (pointX / this._canvas.ActualWidth);

            this._canvas.CaptureMouse();
            this._isMouseCaptured = true;
        }

        private void On_canvas_PreviewMouseMove(object sender, MouseEventArgs e)
        {
            if (this._canvas == null ||
                this._isMouseCaptured == false)
            {
                return;
            }

            var point = e.GetPosition(this._canvas);
            double pointX = point.X;
            double progressBarMin = this.Minimum;
            double progressBarMax = Math.Max(this.Maximum, this.Minimum);
            double valueRange = progressBarMax - progressBarMin;

            //현재 값 표시기 위치 결정
            if (progressBarMax > progressBarMin)
            {
                if (pointX > this._canvas.ActualWidth)
                {
                    pointX = this._canvas.ActualWidth;
                }
                else if (pointX < 0)
                {
                    pointX = 0;
                }

                double centerX = pointX;
                double thumbX = centerX - this.ThumbRadius;
                this._thumb?.SetValue(Canvas.LeftProperty, thumbX);

                this._draggingValue = progressBarMin + valueRange * (pointX / this._canvas.ActualWidth);
            }
            else
            {
                this._thumb?.SetValue(Canvas.LeftProperty, -this.ThumbRadius);
            }
        }

        private void On_canvas_PreviewMouseUp(object sender, MouseButtonEventArgs e)
        {
            if (this._canvas == null)
            {
                return;
            }

            this._canvas.ReleaseMouseCapture();
            this._isMouseCaptured = false;

            if (this._draggingValue != null)
            {
                this.Value = (double)this._draggingValue;
                RoutedEventArgs newEventArgs = new MouseValueChangedEventArgs(MouseValueChangedEvent,this.Value);
                RaiseEvent(newEventArgs);
            }
            this._draggingValue = null;
        }
    }



    public class MouseValueChangedEventArgs : RoutedEventArgs
    {
        public double Value { get; set; }

        public MouseValueChangedEventArgs(RoutedEvent routedEvent, double value) : base(routedEvent)
        {
            this.Value = value;
        }
    }
}
