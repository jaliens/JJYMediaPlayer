#pragma once

#include <d3d9.h>
#include <wrl/client.h>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libavutil/frame.h>
#include <libswscale/swscale.h>
}

class DirectX9Renderer {
public:
    DirectX9Renderer(HWND hwnd);
    ~DirectX9Renderer();
    bool Init();
    void RenderRectangle();
    void RenderAVFrame(AVFrame* frame);
    IDirect3DSurface9* GetSurface() const { return renderTargetSurface_.Get(); }

private:
    bool InitD3D9();
    bool InitTexture(int width, int height, AVPixelFormat format);

    HWND hwnd_;
    Microsoft::WRL::ComPtr<IDirect3D9> d3d9_;
    Microsoft::WRL::ComPtr<IDirect3DDevice9> d3d9Device_;
    Microsoft::WRL::ComPtr<IDirect3DTexture9> texture_;
    Microsoft::WRL::ComPtr<IDirect3DSurface9> renderTargetSurface_;
    SwsContext* swsContext_;
    int videoWidth_;
    int videoHeight_;
};

