using OpenCvSharp.Internal;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Interop;

namespace MediaPlayer
{
    /// <summary>
    /// HwndHost는 WPF에서 네이티브 윈도우를 포함하기 위한 클래스<br/>
    /// 부모 윈도우의 AllowTransparency가 True일 경우 DX를 통한 랜더링 결과가 나타나지 않는 문제가 있음<br/>
    /// AllowTransparency를 True로 하면 레이어드 윈도우로 동작하게 되고 이는 하드웨어 가속을 지원하지 않기 때문
    /// </summary>
    public class DirectXHost : HwndHost 
    {
        private const int WS_CHILD = 0x40000000; //자식 윈도우를 나타냄
        private const int WS_VISIBLE = 0x10000000; //윈도우가 보이는 상태임을 나타냄

        /// <summary>
        /// 네이티브 윈도우 생성
        /// </summary>
        /// <param name="dwExStyle">확장 스타일</param>
        /// <param name="lpszClassName">윈도우 클래스 이름</param>
        /// <param name="lpszWindowName">윈도우 이름</param>
        /// <param name="style">윈도우 스타일</param>
        /// <param name="x"></param>
        /// <param name="y"></param>
        /// <param name="width"></param>
        /// <param name="height"></param>
        /// <param name="hwndParent">부모 윈도우 핸들</param>
        /// <param name="hMenu">메뉴 핸들</param>
        /// <param name="hInst">인스턴스 핸들</param>
        /// <param name="pvParam">추가 매개변수</param>
        /// <returns></returns>
        [DllImport("user32.dll", SetLastError = true, CharSet = CharSet.Auto)]
        private static extern IntPtr CreateWindowEx(int dwExStyle, string lpszClassName, string lpszWindowName,
            int style, int x, int y, int width, int height, IntPtr hwndParent, IntPtr hMenu, IntPtr hInst, [MarshalAs(UnmanagedType.AsAny)] object pvParam);

        /// <summary>
        /// 네이티브 윈도우를 생성
        /// </summary>
        /// <param name="hwndParent">부모 WPF 창의 핸들</param>
        /// <returns></returns>
        protected override HandleRef BuildWindowCore(HandleRef hwndParent)
        {
            IntPtr hwnd = CreateWindowEx(0, "Static", "", WS_CHILD | WS_VISIBLE/*자식윈도우 보이기*/, 0, 0, 800, 450, hwndParent.Handle, IntPtr.Zero, IntPtr.Zero, 0);

            //네이티브 리소스에 대한 핸들을 래핑하여 가비지 수집기에서 보호
            return new HandleRef(this, hwnd);
        }

        protected override void DestroyWindowCore(HandleRef hwnd)
        {
            //FilePlayer.Cleanup();
        }
        
    }
}
