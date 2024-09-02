using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Common.Mvvm
{
    public interface IDialogService
    {
        /// <summary>
        /// 뷰모델 이름을 가지고 매핑된 창을 띄움
        /// </summary>
        /// <param name="viewModelName"></param>
        void ShowDialog(string viewModelName, out bool dialogResult);

        void CloseDialog(string viewModelName, bool dialogResult);
    }
}
