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
    Microsoft::WRL::ComPtr<ID3D11Device> device_ = nullptr;//dx ����̽� ��ü
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_ = nullptr;//���� ������ ����� ó���ϴ� ����̽� ���ؽ�Ʈ
    Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain_ = nullptr;//������ ���� ���� ����ü��
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> renderTargetView_ = nullptr;//������ ����� ����� �� ���۸� ����Ŵ
    Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader_ = nullptr;//���ؽ� ���̴� ���α׷�
    Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader_ = nullptr;//�ȼ� ���̴� ���α׷�
    Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout_ = nullptr;//���� �������� ���� ����
    Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer_ = nullptr;//���� �����͸� GPU�� �����ϴ� ����
    Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler_ = nullptr;//�ؽ��� ���÷� ����
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> textureView_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> texture_ = nullptr;
    SwsContext* swsContext_ = nullptr;
    int videoWidth_ = 0;
    int videoHeight_ = 0;
};