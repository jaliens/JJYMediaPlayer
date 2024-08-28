#pragma once

#include "pch.h"
#include "DirectX9Renderer.h"
#include <stdexcept>


DirectX9Renderer::DirectX9Renderer(HWND hwnd)
    : hwnd_(hwnd), swsContext_(nullptr), videoWidth_(0), videoHeight_(0) {}

DirectX9Renderer::~DirectX9Renderer() {
    if (swsContext_) {
        sws_freeContext(swsContext_);
    }
    if (d3d9Device_) {
        d3d9Device_->Release();
        d3d9Device_ = nullptr;
    }
    if (d3d9_) {
        d3d9_->Release();
        d3d9_ = nullptr;
    }
}

bool DirectX9Renderer::Init() {
    return InitD3D9();
}

bool DirectX9Renderer::InitD3D9() {
    d3d9_ = Direct3DCreate9(D3D_SDK_VERSION);
    if (!d3d9_) {
        throw std::runtime_error("Failed to create Direct3D9 object.");
    }

    D3DPRESENT_PARAMETERS d3dpp = {};
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.hDeviceWindow = hwnd_;
    d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;
    d3dpp.BackBufferWidth = 800;
    d3dpp.BackBufferHeight = 600;

    HRESULT hr = d3d9_->CreateDevice(
        D3DADAPTER_DEFAULT,
        D3DDEVTYPE_HAL,
        hwnd_,
        D3DCREATE_HARDWARE_VERTEXPROCESSING,
        &d3dpp,
        &d3d9Device_
    );
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to create Direct3D9 device.");
    }

    hr = d3d9Device_->CreateRenderTarget(
        d3dpp.BackBufferWidth,
        d3dpp.BackBufferHeight,
        D3DFMT_X8R8G8B8,
        D3DMULTISAMPLE_NONE,
        0,
        FALSE,
        &renderTargetSurface_,
        nullptr
    );
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to create render target surface.");
    }

    return true;
}

bool DirectX9Renderer::InitTexture(int width, int height, AVPixelFormat format) {
    if (texture_) {
        texture_.Reset();
    }

    videoWidth_ = width;
    videoHeight_ = height;

    D3DFORMAT d3dFormat = D3DFMT_A8R8G8B8;

    if (format == AV_PIX_FMT_RGB24 || format == AV_PIX_FMT_BGR24 || format == AV_PIX_FMT_YUV420P) {
        d3dFormat = D3DFMT_A8R8G8B8;
        if (format == AV_PIX_FMT_YUV420P) {
            if (!swsContext_) {
                swsContext_ = sws_getContext(width, height, format, width, height, AV_PIX_FMT_RGBA, SWS_BILINEAR, nullptr, nullptr, nullptr);
                if (!swsContext_) return false;
            }
        }
    }
    else {
        if (!swsContext_) {
            swsContext_ = sws_getContext(width, height, format, width, height, AV_PIX_FMT_RGBA, SWS_BILINEAR, nullptr, nullptr, nullptr);
            if (!swsContext_) return false;
        }
    }

    HRESULT hr = d3d9Device_->CreateTexture(
        width,
        height,
        1,
        D3DUSAGE_DYNAMIC,
        d3dFormat,
        D3DPOOL_DEFAULT,
        &texture_,
        nullptr
    );
    if (FAILED(hr)) {
        return false;
    }

    return true;
}

void DirectX9Renderer::RenderAVFrame(AVFrame* frame) {
    if (!frame) return;

    AVPixelFormat format = static_cast<AVPixelFormat>(frame->format);
    if (frame->width != videoWidth_ || frame->height != videoHeight_ || !texture_) {
        InitTexture(frame->width, frame->height, format);
    }

    D3DLOCKED_RECT lockedRect;
    HRESULT hr = texture_->LockRect(0, &lockedRect, nullptr, D3DLOCK_DISCARD);
    if (FAILED(hr)) return;

    uint8_t* dest = static_cast<uint8_t*>(lockedRect.pBits);
    int destPitch = lockedRect.Pitch;

    if (format == AV_PIX_FMT_RGB24 || format == AV_PIX_FMT_BGR24) {
        for (int y = 0; y < frame->height; y++) {
            uint8_t* src = frame->data[0] + y * frame->linesize[0];
            for (int x = 0; x < frame->width; x++) {
                dest[y * destPitch + x * 4 + 0] = src[x * 3 + (format == AV_PIX_FMT_RGB24 ? 0 : 2)]; // R
                dest[y * destPitch + x * 4 + 1] = src[x * 3 + 1]; // G
                dest[y * destPitch + x * 4 + 2] = src[x * 3 + (format == AV_PIX_FMT_RGB24 ? 2 : 0)]; // B
                dest[y * destPitch + x * 4 + 3] = 255; // A
            }
        }
    }
    else if (format == AV_PIX_FMT_RGBA || format == AV_PIX_FMT_BGRA) {
        for (int y = 0; y < frame->height; y++) {
            memcpy(dest + y * destPitch, frame->data[0] + y * frame->linesize[0], frame->width * 4);
        }
    }
    else if (format == AV_PIX_FMT_YUV420P) {
        uint8_t* destData[4] = { dest, nullptr, nullptr, nullptr };
        int destLinesize[4] = { destPitch, 0, 0, 0 };
        sws_scale(swsContext_, frame->data, frame->linesize, 0, frame->height, destData, destLinesize);
    }
    else {
        uint8_t* destData[4] = { dest, nullptr, nullptr, nullptr };
        int destLinesize[4] = { destPitch, 0, 0, 0 };
        sws_scale(swsContext_, frame->data, frame->linesize, 0, frame->height, destData, destLinesize);
    }

    texture_->UnlockRect(0);

    //d3d9Device_->StretchRect(texture_->GetSurfaceLevel(0), nullptr, renderTargetSurface_.Get(), nullptr, D3DTEXF_NONE);
	IDirect3DSurface9* surfaceLevel;
	texture_->GetSurfaceLevel(0, &surfaceLevel);
	d3d9Device_->StretchRect(surfaceLevel, nullptr, renderTargetSurface_.Get(), nullptr, D3DTEXF_NONE);

    d3d9Device_->BeginScene();
    d3d9Device_->EndScene();
    d3d9Device_->Present(nullptr, nullptr, nullptr, nullptr);
}

void DirectX9Renderer::RenderRectangle() {
    if (!d3d9Device_) return;

    struct Vertex {
        float x, y, z, rhw;
        DWORD color;
    };

    Vertex vertices[] = {
        { 100.0f, 100.0f, 0.0f, 1.0f, D3DCOLOR_XRGB(255, 0, 0) }, // 왼쪽 위
        { 200.0f, 100.0f, 0.0f, 1.0f, D3DCOLOR_XRGB(255, 0, 0) }, // 오른쪽 위
        { 100.0f, 200.0f, 0.0f, 1.0f, D3DCOLOR_XRGB(255, 0, 0) }, // 왼쪽 아래
        { 200.0f, 200.0f, 0.0f, 1.0f, D3DCOLOR_XRGB(255, 0, 0) }  // 오른쪽 아래
    };

    d3d9Device_->BeginScene();

    d3d9Device_->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
    d3d9Device_->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices, sizeof(Vertex));

    d3d9Device_->EndScene();
    d3d9Device_->Present(nullptr, nullptr, nullptr, nullptr);
}




//IDirect3DSurface9* surfaceLevel;
//texture_->GetSurfaceLevel(0, &surfaceLevel);
//d3d9Device_->StretchRect(surfaceLevel, nullptr, renderTargetSurface_.Get(), nullptr, D3DTEXF_NONE);