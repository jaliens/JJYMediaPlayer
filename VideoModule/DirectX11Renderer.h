#pragma once

#include <dxgi.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include <vector>

extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libavutil/frame.h>
#include <libswscale/swscale.h>

#include <SDL.h>
}

class DirectX11Renderer {
public:
    DirectX11Renderer(HWND hwnd);
    ~DirectX11Renderer();
    bool Init(int videoWidth, int videoHeight);
    void Render();
    void Render(AVFrame* frame);

private:
    bool InitDeviceAndSwapChain();
    bool InitRenderTargetView();
    bool InitPipeline();
    bool InitTexture(int width, int height, AVPixelFormat format);

    HWND hwnd_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D11Device> device_ = nullptr;//dx 디바이스 객체
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_ = nullptr;//실제 랜더링 명령을 처리하는 디바이스 컨텍스트
    Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain_ = nullptr;//프레임 버퍼 관리 스왑체인
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> renderTargetView_ = nullptr;//랜더링 결과가 저장될 백 버퍼를 가리킴
    Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader_ = nullptr;//버텍스 셰이더 프로그램
    Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader_ = nullptr;//픽셀 셰이더 프로그램
    Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout_ = nullptr;//정점 데이터의 구조 정의
    Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer_ = nullptr;//정점 데이터를 GPU로 전달하는 버퍼
    Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler_ = nullptr;//텍스쳐 샘플러 상태
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> textureView_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> texture_ = nullptr;
    SwsContext* swsContext_ = nullptr;
    int videoWidth_ = 0;
    int videoHeight_ = 0;
};