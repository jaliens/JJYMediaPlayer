using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;
using System.Windows.Media;

namespace Common.CustomControl
{
    public class MediaProgressBar : RangeBase
    {
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
                new FrameworkPropertyMetadata(4d, FrameworkPropertyMetadataOptions.AffectsRender));
        public double ThumbRadius
        {
            get { return (double)GetValue(ThumbRadiusProperty); }
            set { SetValue(ThumbRadiusProperty, value); }
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
            double progressBarMax = this.Maximum;
            double bufferStart = this.BufferStartValue;
            double bufferEnd = this.BufferEndValue;
            double value = this.Value;

            //프로그래스바 배경 그리기
            Rect barBackgroundRect = new Rect()
            {
                X = 0,
                Y = 0,
                Width = RenderSize.Width,
                Height = RenderSize.Height,
            };
            drawingContext.DrawRectangle(this.Background, null, barBackgroundRect);

            //버퍼 그리기
            double bufferBarWidth = (bufferEnd - bufferStart) / (progressBarMax - progressBarMin) * this.RenderSize.Width;
            double bufferStartX = bufferStart / (progressBarMax - progressBarMin) * this.RenderSize.Width;
            Rect bufferRect = new Rect()
            {
                X = bufferStartX,
                Y = 0,
                Width = bufferBarWidth,
                Height = RenderSize.Height,
            };
            drawingContext.DrawRectangle(this.BufferBrush, null, bufferRect);

            //현재 값 표시기 그리기
            double centerX = (value / (progressBarMax - progressBarMin)) * this.RenderSize.Width;
            double centerY = this.RenderSize.Height / 2d;
            double thumbRadius = this.ThumbRadius;
            Point center = new Point()
            {
                X = centerX,
                Y = centerY,
            };
            drawingContext.DrawEllipse(this.ThumbBrush, null, center, thumbRadius, thumbRadius);
        }
    }
}
