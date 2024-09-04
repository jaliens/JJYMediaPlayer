#pragma once

#include "pch.h"
#include "DirectX11Renderer.h"
#include <stdexcept>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

using namespace DirectX;

struct Vertex {
    XMFLOAT3 position;
    XMFLOAT2 texcoord;
};


DirectX11Renderer::DirectX11Renderer(HWND hwnd)
    : hwnd_(hwnd) {}

DirectX11Renderer::~DirectX11Renderer() {
    this->Cleanup();
}




/// <summary>
/// DX �ʱ�ȭ ���� ����
/// </summary>
/// <returns></returns>
bool DirectX11Renderer::Init(int videoWidth, int videoHeight) {
    this->videoWidth_ = videoWidth;
    this->videoHeight_ = videoHeight;
    if (!InitDeviceAndSwapChain()) return false;
    if (!InitRenderTargetView()) return false;
    if (!InitPipeline()) return false;
    return true;
}


/// <summary>
/// ���� ü��, DirectX 11 ����̽�, ����̽� ���ؽ�Ʈ�� �ʱ�ȭ.
/// </summary>
/// <returns></returns>
bool DirectX11Renderer::InitDeviceAndSwapChain() {
    DXGI_SWAP_CHAIN_DESC scd = {};//����ü���� �Ӽ��� �����ϴ� ����ü.
    scd.BufferCount = 1; //�� ������ ����(1: ������۸� 2: Ʈ���ù��۸�)
    scd.BufferDesc.Width = this->videoWidth_; //������ ũ�� ����(WPF�� ��ġ ���� �����ʹ� �ٸ��� �ػ� ������ ���Ǵ� ����)
    scd.BufferDesc.Height = this->videoHeight_; //������ ũ�� ����(WPF�� ��ġ ���� �����ʹ� �ٸ��� �ػ� ������ ���Ǵ� ����)
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // �� �ʼ��� ����(�� ������ ������ �� 8��Ʈ�� ���� R,G,B,A)
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // �� ���۰� ���� Ÿ������ ������ ��Ÿ��
    scd.OutputWindow = hwnd_; // ������ ����� ǥ���� �������� �ڵ�(�� â�� ũ�⿡ ���� ���� ũ�Ⱑ �ڵ� ������)
    scd.SampleDesc.Count = 1; //��Ƽ���ø� (1: ��� �� ��)
    scd.SampleDesc.Quality = 0; //��Ƽ���ø� (1: ��� �� ��)
    scd.Windowed = TRUE; // â��� ����


    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_0
    };
    D3D_FEATURE_LEVEL featureLevel;



    //Direct3D 11 ����̽��� ���� ü���� �����ϴ� �Լ�
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, //����� ����(null : �� �׷��� ī��)
        D3D_DRIVER_TYPE_HARDWARE, //�ϵ���� ����
        nullptr, //����Ʈ���� ������ ����̹� ����(null: �ϵ���� ���)
        0,
        featureLevels, //dx11 ��� ���� ����(null: ��� ��� ���� ����)
        2, //��� ���� ���� ����
        D3D11_SDK_VERSION, //sdk ���� ����
        &scd, //����ü�� �Ӽ� ����ü�� ������
        &this->swapChain_,
        &this->device_,
        nullptr,
        &this->context_ //������ ����̽� ���ؽ�Ʈ�� �����Ͱ� ����� ������
    );

    return SUCCEEDED(hr);
}

/// <summary>
/// ���� ü���� �� ���۸� ������ ���� Ÿ�� �並 ����.
/// </summary>
/// <returns></returns>
bool DirectX11Renderer::InitRenderTargetView() {
    Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer; // ID3D11Texture2D �������̽� : 2d �ؽ����̸� ����ü���� ����۸� ����ų ��
    HRESULT hr = this->swapChain_->GetBuffer(0, __uuidof(ID3D11Texture2D), &backBuffer); // ����ü�ο��� ����� ��������(ù ����: 0��° ����)
    if (FAILED(hr)) return false;

    hr = this->device_->CreateRenderTargetView(backBuffer.Get(), nullptr, &this->renderTargetView_); // ������ �ؽ��ĸ� ���� Ÿ�� ��� ����(backBuffer.Get(): �� ���� �ؽ����� ������ ��ȯ)
    if (FAILED(hr)) return false;

    this->context_->OMSetRenderTargets(1, this->renderTargetView_.GetAddressOf(), nullptr); // ���� Ÿ�� ����(1: ���� Ÿ�� ����, renderTargetView_.GetAddressOf(): ���� Ÿ�� ���� ������, nullptr: ����-���Ľ� �並 ������� �ʵ��� ����)

    // ����Ʈ : ȭ�鿡 ǥ�õǴ� 2D �簢�� �����̸� ���� �� ����
    // ����Ʈ ���� �߰�
    D3D11_VIEWPORT viewport = {};
    viewport.Width = this->videoWidth_;  // ����Ʈ �ʺ�
    viewport.Height = this->videoHeight_; // ����Ʈ ����

    // ����Ʈ �������� ���� �� ����(z�� �������� ��ü���� �󸶳� �ְ� ������� �ִ� 0 ~ 1.0 ���̷� ���� ����)
    // 2d ȭ�������� ���������� Ȱ���Ͽ� ��ü�� ��ĥ �� ������ �տ� �������� ����
    viewport.MinDepth = 0.0f; // �ּ� ����(ī�޶󿡼� ���� ����� ��ġ)
    viewport.MaxDepth = 1.0f; // �ִ� ����(ī�޶󿡼� ���� �� ��ġ)

    // ����Ʈ�� ���� ��ǥ ����
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;

    this->context_->RSSetViewports(1, &viewport); // ������ ������������ �����Ͷ����� �ܰ迡�� ����� ����Ʈ�� ����

    return true;
}

/// <summary>
/// Vertex Shader, Pixel Shader ����, ���̴� ������ �� DirectX ����̽��� ����
/// </summary>
/// <returns></returns>
bool DirectX11Renderer::InitPipeline() {

    // ���ؽ� ���̴� ���α׷�
    const char* vsSource = R"(
    struct VS_INPUT {
        float3 position : POSITION;
        float2 texcoord : TEXCOORD;
    };

    struct PS_INPUT {
        float4 position : SV_POSITION;
        float2 texcoord : TEXCOORD;
    };

    PS_INPUT VSMain(VS_INPUT input) {
        PS_INPUT output;
        output.position = float4(input.position, 1.0f);
        output.texcoord = input.texcoord;
        return output;
    }
    )";

    // �ȼ� ���̴� ���α׷�
    const char* psSource = R"(
    Texture2D tex : register(t0);
    SamplerState samp : register(s0);

    struct PS_INPUT {
        float4 position : SV_POSITION; // ȭ�� ���������� ��ġ (������ ��)
        float2 texcoord : TEXCOORD;    // �ؽ�ó ��ǥ (������ ��)
    };

    float4 PSMain(PS_INPUT input) : SV_TARGET {
        return tex.Sample(samp, input.texcoord);
    }
    )";

    Microsoft::WRL::ComPtr<ID3DBlob> vsBlob;// ���ؽ� ���̴� ������ ��� ����
    Microsoft::WRL::ComPtr<ID3DBlob> psBlob;// �ȼ� ���̴� ������ ��� ����
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;// ������ ��� ���� ����

    // ���ؽ� ���̴� HLSL �ڵ� ������
    HRESULT hr = D3DCompile(vsSource, strlen(vsSource), nullptr, nullptr, nullptr, "VSMain", "vs_5_0", 0, 0, &vsBlob, &errorBlob);
    if (FAILED(hr)) return false;

    // �ȼ� ���̴� HLSL �ڵ� ������
    hr = D3DCompile(psSource, strlen(psSource), nullptr, nullptr, nullptr, "PSMain", "ps_5_0", 0, 0, &psBlob, &errorBlob);
    if (FAILED(hr)) return false;

    // ���ؽ� ���̴� ����
    hr = this->device_->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &this->vertexShader_);
    if (FAILED(hr)) return false;

    // �ȼ� ���̴� ����
    hr = this->device_->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &this->pixelShader_);
    if (FAILED(hr)) return false;

    this->context_->VSSetShader(this->vertexShader_.Get(), nullptr, 0);// ���ؽ� ���̴� ����
    this->context_->PSSetShader(this->pixelShader_.Get(), nullptr, 0);// �ȼ� ���̴� ����

    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    // ������ ������ �Է� ���̾ƿ��� GPU�� �ε��Ͽ� ��� �����ϵ��� ����.
    hr = this->device_->CreateInputLayout(layout, ARRAYSIZE(layout), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &this->inputLayout_);
    if (FAILED(hr)) return false;

    // ������� �Է� ���̾ƿ��� ����.
    this->context_->IASetInputLayout(this->inputLayout_.Get());

    std::vector<Vertex> vertices = {
        { XMFLOAT3(-1.0f,  1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
        { XMFLOAT3(1.0f,  1.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) },
        { XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) },
        { XMFLOAT3(1.0f, -1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) }
    };

    D3D11_BUFFER_DESC bd = {};// GPU�� ������ ���ؽ� ������ �Ӽ�
    bd.Usage = D3D11_USAGE_DEFAULT; // ���۸� �⺻ �뵵�� ����(CPU�� GPU�� ��� ���� ����)
    bd.ByteWidth = sizeof(Vertex) * vertices.size(); // ���� ũ�� ����(����Ʈ ����)
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER; // ���ؽ� ���۷� ����ϵ��� ����
    bd.CPUAccessFlags = /*D3D11_CPU_ACCESS_WRITE*/0;//0; // 0: GPU�� ���� �����ϵ��� ����

    D3D11_SUBRESOURCE_DATA initData = {};// �ʱ� �����ͷ� ä�� ����
    initData.pSysMem = vertices.data();// �ʱ� �����Ͱ� ����� �޸��� ������ ����

    hr = this->device_->CreateBuffer(&bd, &initData, &this->vertexBuffer_);// ���ؽ� ���� ����(���� �����͸� GPU�� ������ �� ����)
    if (FAILED(hr)) return false;

    UINT stride = sizeof(Vertex);// �� ������ ũ��
    UINT offset = 0;// ���� ������ �б� ������(0: ó�� ���� �д´ٴ� �ǹ�)
    this->context_->IASetVertexBuffers(0, 1/*���� ����*/, this->vertexBuffer_.GetAddressOf(), &stride, &offset);// ���ؽ� ���� ����
    this->context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);// GPU�� ������ �ؼ��ϴ� ��� ����
    
    // �ؽ��� ���ø� ����
    D3D11_SAMPLER_DESC sampDesc = {};
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;// �ؽ�ó ��ǥ�� ��Ȯ�� ���ε��� �ʴ� ���, �ֺ� �ȼ����� ������ ���������� ����
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;// �ؽ��� ��ǥ�� ���� ����� ���� �ݺ�
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;// �ؽ��� ��ǥ�� ���� ����� ���� �ݺ�
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;// �ؽ��� ��ǥ�� ���� ����� ���� �ݺ�
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;// ���ø��� �ؽ��� ���� ���۷��� ���� �� �� ��
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
    hr = this->device_->CreateSamplerState(&sampDesc, &this->sampler_);
    if (FAILED(hr)) return false;
    
    return true;
}

bool DirectX11Renderer::InitTexture(int srcWidth, int srcHeight, AVPixelFormat srcFormat) {
    if (this->texture_) {
        this->texture_.Reset();
        this->shaderResourceView_texture.Reset();
    }

    this->videoWidth_ = srcWidth;
    this->videoHeight_ = srcHeight;

    // �ؽ��� ����
    DXGI_FORMAT dxgiFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

    // FFmpeg ������ DXGI �������� ��ȯ
    if (srcFormat == AV_PIX_FMT_RGB24 || srcFormat == AV_PIX_FMT_BGR24) 
    {
        dxgiFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    }
    else if (srcFormat == AV_PIX_FMT_YUV420P) 
    {
        // YUV420P�� RGB�� ��ȯ�ϱ� ���� �⺻ RGB ���� ���
        dxgiFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    }
    else {
        // �⺻������ �������� �ʴ� ���˿� ���� ��ȯ�� ���� SwsContext ����
        dxgiFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
        if (!this->swsContext_) 
        {
            this->swsContext_ = sws_getContext(srcWidth, srcHeight, srcFormat, srcWidth, srcHeight, AV_PIX_FMT_RGBA, SWS_BILINEAR, nullptr, nullptr, nullptr);
            if (!this->swsContext_)
            {
                return false;
            }
        }
    }

    //2D �ؽ��� �Ӽ� ����
    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = srcWidth;
    texDesc.Height = srcHeight;
    texDesc.MipLevels = 1; // 1:mipmap ��� ����
    texDesc.ArraySize = 1; // 1:�迭 �ؽ��İ� �ƴ� ���� �ؽ��ĸ� �ǹ�
    texDesc.Format = dxgiFormat;
    texDesc.SampleDesc.Count = 1; // 1:��Ƽ���ø� �Ⱦ�
    texDesc.Usage = D3D11_USAGE_DYNAMIC; // CPU�� GPU ��� ���� �����ϵ���
    texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE; // �� �ؽ��İ� ���̴����� ���� ����
    texDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE; // CPU���� ���� ���� ��

    // GPU�� 2D�ؽ��� ����
    HRESULT hr = this->device_->CreateTexture2D(&texDesc, nullptr, &this->texture_);
    if (FAILED(hr)) return false;

    // ���̴� ���ҽ��� : ���̴��� �ؽ���, ���� � ������ �� �ֵ����ϴ� �������̽�
    // ���̴� ���ҽ��� �Ӽ� ����
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = texDesc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;

    // ���̴� ���ҽ��� ����(�ȼ� ���̴����� �ؽ��ĸ� ���ø��ϴµ� ����)
    hr = this->device_->CreateShaderResourceView(this->texture_.Get(), &srvDesc, &this->shaderResourceView_texture);
    if (FAILED(hr)) return false;

    return true;
}

void DirectX11Renderer::RenderEmptyRect() {
    const float clearColor[4] = { 0.0f, 0.2f, 0.4f, 1.0f };//ȭ�� ����鼭 ä�� ����
    this->context_->ClearRenderTargetView(this->renderTargetView_.Get(), clearColor);//ȭ�� �����
    this->context_->Draw(3, 0);//����ۿ� ������ ����
    HRESULT hr = this->swapChain_->Present(1, 0); //�����, ����Ʈ ���� ��ü�Ͽ� ���÷���(����Ʈ ���۴� ȭ�鿡 ǥ�õǴ� ���̰� ����۴� ������ �������Ǵ� ���̴�)
    if (FAILED(hr))
    {
        printf("����\n");
    }
}

/// <summary>
/// AVFrame�� DirectX11�� ȭ�鿡 ������
/// </summary>
/// <param name="frame"></param>
void DirectX11Renderer::Render(AVFrame* frame) {
    if (!frame) return;
    AVPixelFormat format = static_cast<AVPixelFormat>(frame->format);
    if (frame->width != this->videoWidth_ || 
        frame->height != this->videoHeight_ || 
        this->texture_ == nullptr) 
    {
        InitTexture(frame->width, frame->height, format);
    }

    D3D11_MAPPED_SUBRESOURCE mappedResource;
    // Map�Լ� : GPU �޸��� �ؽ��ĸ� CPU�� ������ �� �ְ� ����
    this->context_->Map(this->texture_.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);

    uint8_t* dest = static_cast<uint8_t*>(mappedResource.pData);// �ؽ��� ������ ������
    int destPitch = mappedResource.RowPitch;// �ؽ����� �� ��(��)�� �����ϴ� ����Ʈ ��

    // ���� �������� �ؽ��Ŀ� ����
    if (format == AV_PIX_FMT_RGB24 || format == AV_PIX_FMT_BGR24) {
        // RGB �Ǵ� BGR ������ RGBA�� ��ȯ
        for (int y = 0; y < frame->height; y++) 
        {
            uint8_t* src = frame->data[0];
            for (int x = 0; x < frame->width; x++) 
            {
                if (format == AV_PIX_FMT_RGB24)
                {
                    dest[y * destPitch + x * 4 + 0] = src[y * frame->linesize[0] + x * 3 + 0];
                    dest[y * destPitch + x * 4 + 1] = src[y * frame->linesize[0] + x * 3 + 1];
                    dest[y * destPitch + x * 4 + 2] = src[y * frame->linesize[0] + x * 3 + 2];
                    dest[y * destPitch + x * 4 + 3] = 255;
                }
                else if(format == AV_PIX_FMT_RGB24)
                {
                    dest[y * destPitch + x * 4 + 0] = src[y * frame->linesize[0] + x * 3 + 2];
                    dest[y * destPitch + x * 4 + 1] = src[y * frame->linesize[0] + x * 3 + 1];
                    dest[y * destPitch + x * 4 + 2] = src[y * frame->linesize[0] + x * 3 + 0];
                    dest[y * destPitch + x * 4 + 3] = 255;
                }
            }
        }
    }
    else if (format == AV_PIX_FMT_RGBA || format == AV_PIX_FMT_BGRA) 
    {
        for (int y = 0; y < frame->height; y++) 
        {
            // �̹� RGBA�̸� �״�� ����
            if (format == AV_PIX_FMT_RGBA)
            {
                memcpy(dest + y * destPitch, frame->data[0] + y * frame->linesize[0], frame->width * 4);
            }
            // �̹� BGRA�̸� RGBA�� ���� ��ȯ
            else if (format == AV_PIX_FMT_BGRA)
            {
                uint8_t* src = frame->data[0];
                for (int x = 0; x < frame->width; x++)
                {
                    dest[y * destPitch + x * 4 + 0] = src[y * destPitch + x * 4 + 2];
                    dest[y * destPitch + x * 4 + 1] = src[y * destPitch + x * 4 + 1];
                    dest[y * destPitch + x * 4 + 2] = src[y * destPitch + x * 4 + 0];
                    dest[y * destPitch + x * 4 + 3] = src[y * destPitch + x * 4 + 3];
                }
            }
        }
    }
    else if (format == AV_PIX_FMT_YUV420P) {
        // YUV420P�� RGBA�� ��ȯ
        uint8_t* srcY = frame->data[0];
        uint8_t* srcU = frame->data[1];
        uint8_t* srcV = frame->data[2];
        for (int y = 0; y < frame->height; y++) 
        {
            for (int x = 0; x < frame->width; x++) 
            {
                int Y = srcY[y * frame->linesize[0] + x]; //Y�� �ȼ� 4�� �� 4��
                int U = srcU[(y / 2) * frame->linesize[1] + (x / 2)]; //U�� �ȼ� 4��(2x2) �� 1��
                int V = srcV[(y / 2) * frame->linesize[2] + (x / 2)]; //V�� �ȼ� 4��(2x2) �� 1��

                int R = Y + 1.402 * (V - 128);
                int G = Y - 0.344 * (U - 128) - 0.714 * (V - 128);
                int B = Y + 1.772 * (U - 128);

                R = min(max(R, 0), 255);
                G = min(max(G, 0), 255);
                B = min(max(B, 0), 255);

                dest[y * destPitch + x * 4 + 0] = R;
                dest[y * destPitch + x * 4 + 1] = G;
                dest[y * destPitch + x * 4 + 2] = B;
                dest[y * destPitch + x * 4 + 3] = 255;
            }
        }
    }
    else {
        // SwsContext�� ����Ͽ� �ٸ� ������ RGBA�� ��ȯ
        uint8_t* destData[4] = { dest, nullptr, nullptr, nullptr };
        int destLinesize[4] = { destPitch, 0, 0, 0 };
        sws_scale(this->swsContext_, frame->data, frame->linesize, 0, frame->height, destData, destLinesize);
    }

    // Unmap : �ٽ� GPU���� �ؽ��� ���� �ѱ�
    this->context_->Unmap(this->texture_.Get(), 0);

    // ȭ��(�����) �����(�ʱ�ȭ)
    const float clearColor[4] = { 0.0f, 0.2f, 0.4f, 1.0f };
    this->context_->ClearRenderTargetView(this->renderTargetView_.Get(), clearColor);

    // �ؽ��ĸ� �ȼ����̴��� ���ε�
    this->context_->PSSetShaderResources(0, 1, this->shaderResourceView_texture.GetAddressOf());

    // �ؽ��� ���ø��� ����� ���÷� ���� ����
    this->context_->PSSetSamplers(0, 1, this->sampler_.GetAddressOf());

    this->context_->Draw(4, 0);// ����ۿ� ������ ����
    this->swapChain_->Present(1, 0);// �����, ����Ʈ ���� ��ü�Ͽ� ���÷���(����Ʈ ���۴� ȭ�鿡 ǥ�õǴ� ���̰� ����۴� ������ �������Ǵ� ���̴�)
}

/// <summary>
/// ȭ�鿡 ���� �ؽ��ĸ� ǥ��
/// </summary>
void DirectX11Renderer::ClearScreen() {

    if (this->videoWidth_ == 0 ||
        this->videoHeight_ == 0 ||
        this->texture_ == nullptr)
        return;

    bool res = this->InitTexture(this->videoWidth_, this->videoHeight_, AV_PIX_FMT_RGB24);
    if (res == false) return;

    D3D11_MAPPED_SUBRESOURCE mappedResource;
    // Map�Լ� : GPU �޸��� �ؽ��ĸ� CPU�� ������ �� �ְ� ����
    this->context_->Map(this->texture_.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);

    uint8_t* dest = static_cast<uint8_t*>(mappedResource.pData);// �ؽ��� ������ ������
    int destPitch = mappedResource.RowPitch;// �ؽ����� �� ��(��)�� �����ϴ� ����Ʈ ��

    // �ؽ��ĸ� ����������
    for (int y = 0; y < this->videoHeight_; y++)
    {
        for (int x = 0; x < this->videoWidth_; x++)
        {
            dest[y * destPitch + x * 4 + 0] = 0;
            dest[y * destPitch + x * 4 + 1] = 0;
            dest[y * destPitch + x * 4 + 2] = 0;
            dest[y * destPitch + x * 4 + 3] = 255;
        }
    }

    // Unmap : �ٽ� GPU���� �ؽ��� ���� �ѱ�
    this->context_->Unmap(this->texture_.Get(), 0);

    // ȭ��(�����) �����(�ʱ�ȭ)
    const float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    this->context_->ClearRenderTargetView(this->renderTargetView_.Get(), clearColor);

    // �ؽ��ĸ� �ȼ����̴��� ���ε�
    this->context_->PSSetShaderResources(0, 1, this->shaderResourceView_texture.GetAddressOf());

    // �ؽ��� ���ø��� ����� ���÷� ���� ����
    this->context_->PSSetSamplers(0, 1, this->sampler_.GetAddressOf());

    this->context_->Draw(4, 0);//����ۿ� ������ ����
    this->swapChain_->Present(1, 0);//�����, ����Ʈ ���� ��ü�Ͽ� ���÷���(����Ʈ ���۴� ȭ�鿡 ǥ�õǴ� ���̰� ����۴� ������ �������Ǵ� ���̴�)
}

/// <summary>
/// �ڿ� ����
/// </summary>
void DirectX11Renderer::Cleanup() {
    // ��� ���̴� ���ҽ��� ���������� ���¸� ����
    if (this->context_) {
        this->context_->ClearState(); 
    }

    if (this->swsContext_) {
        sws_freeContext(this->swsContext_);
        this->swsContext_ = nullptr;
    }

    // ����Ʈ �����͸� ����� �͵��� ����� ���� ���ʿ�
}


LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

extern "C" __declspec(dllexport) HWND createDirectXWindow(HINSTANCE hInstance, int width, int height, HWND parentHwnd) {
    // ������ Ŭ���� ���
    const wchar_t CLASS_NAME[] = L"Window Class";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    // ������ ����
    HWND hwnd = CreateWindowEx(
        0,                          // Ȯ�� ��Ÿ��
        CLASS_NAME,                 // â Ŭ���� �̸�
        L"",                        // â ����
        WS_POPUP,                   // â ��Ÿ��
        0, 0,                       // â�� �ʱ� ��ġ
        width, height,              // â�� �ʱ� ũ��
        parentHwnd,                 // �θ� â
        nullptr,                    // �޴�
        hInstance,                  // �ν��Ͻ� �ڵ�
        nullptr                     // �߰����� ���ø����̼� ������
    );

    if (hwnd == nullptr) {
        return 0;
    }

    // â ǥ�� �� ������Ʈ
    ShowWindow(hwnd, 5);
    UpdateWindow(hwnd);

    return hwnd;
}