using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Data;

namespace Common.Mvvm.Converter
{
    public class EnumToVisibilityConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            if (value == null || parameter == null)
            {
                return Visibility.Collapsed;
            }

            if (Enum.IsDefined(value.GetType(), value) == false)
            {
                return Visibility.Collapsed;
            }

            if (Enum.IsDefined(parameter.GetType(), parameter) == false)
            {
                return Visibility.Collapsed;
            }

            var enumValue = (Enum)value;
            var enumParam = (Enum)parameter;
            var targetEnumValue = Enum.Parse(value.GetType(), enumParam.ToString());

            return enumValue.Equals(targetEnumValue) ? Visibility.Visible : Visibility.Collapsed;
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
}
