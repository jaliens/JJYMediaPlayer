// dllmain.cpp : DLL 애플리케이션의 진입점을 정의합니다.
#include "pch.h"

extern "C" {
    #include <libavformat/avformat.h>
}


BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

extern "C" __declspec(dllexport) int Add(int a, int b) {
    //av_regi
    unsigned version = avformat_version();
    printf("libavformat version: %u.%u.%u\n", version >> 16, (version >> 8) & 0xFF, version & 0xFF);
    return a + b;
}