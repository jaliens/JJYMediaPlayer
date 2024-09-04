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
/// DX 초기화 과정 통합
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
/// 스왑 체인, DirectX 11 디바이스, 디바이스 컨텍스트를 초기화.
/// </summary>
/// <returns></returns>
bool DirectX11Renderer::InitDeviceAndSwapChain() {
    DXGI_SWAP_CHAIN_DESC scd = {};//스왑체인의 속성을 정의하는 구조체.
    scd.BufferCount = 1; //백 버퍼의 개수(1: 더블버퍼링 2: 트리플버퍼링)
    scd.BufferDesc.Width = this->videoWidth_; //윈도우 크기 권장(WPF의 장치 독립 단위와는 다르고 해상도 설정에 사용되는 단위)
    scd.BufferDesc.Height = this->videoHeight_; //윈도우 크기 권장(WPF의 장치 독립 단위와는 다르고 해상도 설정에 사용되는 단위)
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // 각 필셀의 형식(이 설정에 따르면 각 8비트는 각각 R,G,B,A)
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // 이 버퍼가 랜더 타겟으로 사용됨을 나타냄
    scd.OutputWindow = hwnd_; // 랜더링 결과를 표시할 윈도우의 핸들(이 창의 크기에 맞춰 버퍼 크기가 자동 조절됨)
    scd.SampleDesc.Count = 1; //멀티샘플링 (1: 사용 안 함)
    scd.SampleDesc.Quality = 0; //멀티샘플링 (1: 사용 안 함)
    scd.Windowed = TRUE; // 창모드 설정


    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_0
    };
    D3D_FEATURE_LEVEL featureLevel;



    //Direct3D 11 디바이스와 스왑 체인을 생성하는 함수
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, //어댑터 지정(null : 주 그래픽 카드)
        D3D_DRIVER_TYPE_HARDWARE, //하드웨어 가속
        nullptr, //소프트웨어 랜더링 드라이버 지정(null: 하드웨어 사용)
        0,
        featureLevels, //dx11 기능 레벨 지정(null: 모든 기능 레벨 지원)
        2, //기능 레벨 개수 지정
        D3D11_SDK_VERSION, //sdk 버전 지정
        &scd, //스왑체인 속성 구조체의 포인터
        &this->swapChain_,
        &this->device_,
        nullptr,
        &this->context_ //생성된 디바이스 컨텍스트의 포인터가 저장될 포인터
    );

    return SUCCEEDED(hr);
}

/// <summary>
/// 스왑 체인의 백 버퍼를 가져와 렌더 타겟 뷰를 생성.
/// </summary>
/// <returns></returns>
bool DirectX11Renderer::InitRenderTargetView() {
    Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer; // ID3D11Texture2D 인터페이스 : 2d 텍스쳐이며 스왑체인의 백버퍼를 가리킬 것
    HRESULT hr = this->swapChain_->GetBuffer(0, __uuidof(ID3D11Texture2D), &backBuffer); // 스왑체인에서 백버퍼 가져오기(첫 인자: 0번째 버퍼)
    if (FAILED(hr)) return false;

    hr = this->device_->CreateRenderTargetView(backBuffer.Get(), nullptr, &this->renderTargetView_); // 지정된 텍스쳐를 랜더 타겟 뷰로 설정(backBuffer.Get(): 백 버퍼 텍스쳐의 포인터 반환)
    if (FAILED(hr)) return false;

    this->context_->OMSetRenderTargets(1, this->renderTargetView_.GetAddressOf(), nullptr); // 랜더 타겟 설정(1: 랜더 타겟 개수, renderTargetView_.GetAddressOf(): 랜더 타겟 뷰의 포인터, nullptr: 깊이-스탠실 뷰를 사용하지 않도록 지정)

    // 뷰포트 : 화면에 표시되는 2D 사각형 영역이며 여러 개 가능
    // 뷰포트 설정 추가
    D3D11_VIEWPORT viewport = {};
    viewport.Width = this->videoWidth_;  // 뷰포트 너비
    viewport.Height = this->videoHeight_; // 뷰포트 높이

    // 뷰포트 내에서의 깊이 값 범위(z축 방향으로 객체들이 얼마나 멀고 가까운지 최대 0 ~ 1.0 사이로 설정 가능)
    // 2d 화면이지만 깊이정보를 활용하여 객체가 겹칠 때 무엇을 앞에 보여줄지 결정
    viewport.MinDepth = 0.0f; // 최소 깊이(카메라에서 가장 가까운 위치)
    viewport.MaxDepth = 1.0f; // 최대 깊이(카메라에서 가장 먼 위치)

    // 뷰포트의 시작 좌표 지정
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;

    this->context_->RSSetViewports(1, &viewport); // 렌더링 파이프라인의 래스터라이저 단계에서 사용할 뷰포트를 설정

    return true;
}

/// <summary>
/// Vertex Shader, Pixel Shader 정의, 셰이더 컴파일 후 DirectX 디바이스에 적용
/// </summary>
/// <returns></returns>
bool DirectX11Renderer::InitPipeline() {

    // 버텍스 셰이더 프로그램
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

    // 픽셀 셰이더 프로그램
    const char* psSource = R"(
    Texture2D tex : register(t0);
    SamplerState samp : register(s0);

    struct PS_INPUT {
        float4 position : SV_POSITION; // 화면 공간에서의 위치 (보간된 값)
        float2 texcoord : TEXCOORD;    // 텍스처 좌표 (보간된 값)
    };

    float4 PSMain(PS_INPUT input) : SV_TARGET {
        return tex.Sample(samp, input.texcoord);
    }
    )";

    Microsoft::WRL::ComPtr<ID3DBlob> vsBlob;// 버텍스 셰이더 컴파일 결과 정보
    Microsoft::WRL::ComPtr<ID3DBlob> psBlob;// 픽셀 셰이더 컴파일 결과 정보
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;// 컴파일 결과 에러 정보

    // 버텍스 셰이더 HLSL 코드 컴파일
    HRESULT hr = D3DCompile(vsSource, strlen(vsSource), nullptr, nullptr, nullptr, "VSMain", "vs_5_0", 0, 0, &vsBlob, &errorBlob);
    if (FAILED(hr)) return false;

    // 픽셀 셰이더 HLSL 코드 컴파일
    hr = D3DCompile(psSource, strlen(psSource), nullptr, nullptr, nullptr, "PSMain", "ps_5_0", 0, 0, &psBlob, &errorBlob);
    if (FAILED(hr)) return false;

    // 버텍스 셰이더 생성
    hr = this->device_->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &this->vertexShader_);
    if (FAILED(hr)) return false;

    // 픽셀 셰이더 생성
    hr = this->device_->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &this->pixelShader_);
    if (FAILED(hr)) return false;

    this->context_->VSSetShader(this->vertexShader_.Get(), nullptr, 0);// 버텍스 셰이더 설정
    this->context_->PSSetShader(this->pixelShader_.Get(), nullptr, 0);// 픽세 셰이더 설정

    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    // 위에서 정의한 입력 레이아웃을 GPU에 로드하여 사용 가능하도록 생성.
    hr = this->device_->CreateInputLayout(layout, ARRAYSIZE(layout), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &this->inputLayout_);
    if (FAILED(hr)) return false;

    // 만들어진 입력 레이아웃을 설정.
    this->context_->IASetInputLayout(this->inputLayout_.Get());

    std::vector<Vertex> vertices = {
        { XMFLOAT3(-1.0f,  1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
        { XMFLOAT3(1.0f,  1.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) },
        { XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) },
        { XMFLOAT3(1.0f, -1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) }
    };

    D3D11_BUFFER_DESC bd = {};// GPU에 생성할 버텍스 버퍼의 속성
    bd.Usage = D3D11_USAGE_DEFAULT; // 버퍼를 기본 용도로 설정(CPU와 GPU가 모두 접근 가능)
    bd.ByteWidth = sizeof(Vertex) * vertices.size(); // 버퍼 크기 지정(바이트 단위)
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER; // 버텍스 버퍼로 사용하도록 지정
    bd.CPUAccessFlags = /*D3D11_CPU_ACCESS_WRITE*/0;//0; // 0: GPU만 접근 가능하도록 설정

    D3D11_SUBRESOURCE_DATA initData = {};// 초기 데이터로 채울 버퍼
    initData.pSysMem = vertices.data();// 초기 데이터가 저장된 메모리의 포인터 전달

    hr = this->device_->CreateBuffer(&bd, &initData, &this->vertexBuffer_);// 버텍스 버퍼 생성(정점 데이터를 GPU에 전달할 때 사용됨)
    if (FAILED(hr)) return false;

    UINT stride = sizeof(Vertex);// 각 정점의 크기
    UINT offset = 0;// 버퍼 내에서 읽기 오프셋(0: 처음 부터 읽는다는 의미)
    this->context_->IASetVertexBuffers(0, 1/*버퍼 개수*/, this->vertexBuffer_.GetAddressOf(), &stride, &offset);// 버텍스 버퍼 설정
    this->context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);// GPU가 정점을 해석하는 방식 설정
    
    // 텍스쳐 샘플링 설정
    D3D11_SAMPLER_DESC sampDesc = {};
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;// 텍스처 좌표가 정확히 매핑되지 않는 경우, 주변 픽셀들의 색상을 선형적으로 보간
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;// 텍스쳐 좌표가 범위 벗어나면 가로 반복
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;// 텍스쳐 좌표가 범위 벗어나면 세로 반복
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;// 텍스쳐 좌표가 범위 벗어나면 깊이 반복
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;// 샘플링된 텍스쳐 값과 레퍼런스 값을 비교 안 함
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

    // 텍스쳐 포멧
    DXGI_FORMAT dxgiFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

    // FFmpeg 포맷을 DXGI 포맷으로 변환
    if (srcFormat == AV_PIX_FMT_RGB24 || srcFormat == AV_PIX_FMT_BGR24) 
    {
        dxgiFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    }
    else if (srcFormat == AV_PIX_FMT_YUV420P) 
    {
        // YUV420P를 RGB로 변환하기 위해 기본 RGB 포맷 사용
        dxgiFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    }
    else {
        // 기본적으로 지원하지 않는 포맷에 대한 변환을 위해 SwsContext 설정
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

    //2D 텍스쳐 속성 정의
    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = srcWidth;
    texDesc.Height = srcHeight;
    texDesc.MipLevels = 1; // 1:mipmap 사용 안함
    texDesc.ArraySize = 1; // 1:배열 텍스쳐가 아닌 단일 텍스쳐를 의미
    texDesc.Format = dxgiFormat;
    texDesc.SampleDesc.Count = 1; // 1:멀티샘플링 안씀
    texDesc.Usage = D3D11_USAGE_DYNAMIC; // CPU와 GPU 모두 접근 가능하도록
    texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE; // 이 텍스쳐가 셰이더에서 사용될 것임
    texDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE; // CPU에게 쓰기 권한 줌

    // GPU에 2D텍스쳐 생성
    HRESULT hr = this->device_->CreateTexture2D(&texDesc, nullptr, &this->texture_);
    if (FAILED(hr)) return false;

    // 셰이더 리소스뷰 : 셰이더가 텍스쳐, 버퍼 등에 접근할 수 있도록하는 인터페이스
    // 셰이더 리소스뷰 속성 정의
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = texDesc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;

    // 셰이더 리소스뷰 생성(픽셀 셰이더에서 텍스쳐를 샘플링하는데 사용됨)
    hr = this->device_->CreateShaderResourceView(this->texture_.Get(), &srvDesc, &this->shaderResourceView_texture);
    if (FAILED(hr)) return false;

    return true;
}

void DirectX11Renderer::RenderEmptyRect() {
    const float clearColor[4] = { 0.0f, 0.2f, 0.4f, 1.0f };//화면 지우면서 채울 색상
    this->context_->ClearRenderTargetView(this->renderTargetView_.Get(), clearColor);//화면 지우기
    this->context_->Draw(3, 0);//백버퍼에 랜더링 수행
    HRESULT hr = this->swapChain_->Present(1, 0); //백버퍼, 프론트 버퍼 교체하여 디스플레이(프론트 버퍼는 화면에 표시되는 것이고 백버퍼는 사전에 렌더링되는 것이다)
    if (FAILED(hr))
    {
        printf("실패\n");
    }
}

/// <summary>
/// AVFrame을 DirectX11로 화면에 랜더링
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
    // Map함수 : GPU 메모리의 텍스쳐를 CPU가 수정할 수 있게 해줌
    this->context_->Map(this->texture_.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);

    uint8_t* dest = static_cast<uint8_t*>(mappedResource.pData);// 텍스쳐 데이터 포인터
    int destPitch = mappedResource.RowPitch;// 텍스쳐의 한 줄(행)이 차지하는 바이트 수

    // 비디오 프레임을 텍스쳐에 복사
    if (format == AV_PIX_FMT_RGB24 || format == AV_PIX_FMT_BGR24) {
        // RGB 또는 BGR 포맷을 RGBA로 변환
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
            // 이미 RGBA이면 그대로 복사
            if (format == AV_PIX_FMT_RGBA)
            {
                memcpy(dest + y * destPitch, frame->data[0] + y * frame->linesize[0], frame->width * 4);
            }
            // 이미 BGRA이면 RGBA로 순서 변환
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
        // YUV420P를 RGBA로 변환
        uint8_t* srcY = frame->data[0];
        uint8_t* srcU = frame->data[1];
        uint8_t* srcV = frame->data[2];
        for (int y = 0; y < frame->height; y++) 
        {
            for (int x = 0; x < frame->width; x++) 
            {
                int Y = srcY[y * frame->linesize[0] + x]; //Y는 픽셀 4개 당 4개
                int U = srcU[(y / 2) * frame->linesize[1] + (x / 2)]; //U는 픽셀 4개(2x2) 당 1개
                int V = srcV[(y / 2) * frame->linesize[2] + (x / 2)]; //V는 픽셀 4개(2x2) 당 1개

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
        // SwsContext를 사용하여 다른 포맷을 RGBA로 변환
        uint8_t* destData[4] = { dest, nullptr, nullptr, nullptr };
        int destLinesize[4] = { destPitch, 0, 0, 0 };
        sws_scale(this->swsContext_, frame->data, frame->linesize, 0, frame->height, destData, destLinesize);
    }

    // Unmap : 다시 GPU에게 텍스쳐 사용권 넘김
    this->context_->Unmap(this->texture_.Get(), 0);

    // 화면(백버퍼) 지우기(초기화)
    const float clearColor[4] = { 0.0f, 0.2f, 0.4f, 1.0f };
    this->context_->ClearRenderTargetView(this->renderTargetView_.Get(), clearColor);

    // 텍스쳐를 픽셀셰이더에 바인딩
    this->context_->PSSetShaderResources(0, 1, this->shaderResourceView_texture.GetAddressOf());

    // 텍스쳐 샘플링에 사용할 샘플러 상태 설정
    this->context_->PSSetSamplers(0, 1, this->sampler_.GetAddressOf());

    this->context_->Draw(4, 0);// 백버퍼에 랜더링 수행
    this->swapChain_->Present(1, 0);// 백버퍼, 프론트 버퍼 교체하여 디스플레이(프론트 버퍼는 화면에 표시되는 것이고 백버퍼는 사전에 렌더링되는 것이다)
}

/// <summary>
/// 화면에 검은 텍스쳐를 표시
/// </summary>
void DirectX11Renderer::ClearScreen() {

    if (this->videoWidth_ == 0 ||
        this->videoHeight_ == 0 ||
        this->texture_ == nullptr)
        return;

    bool res = this->InitTexture(this->videoWidth_, this->videoHeight_, AV_PIX_FMT_RGB24);
    if (res == false) return;

    D3D11_MAPPED_SUBRESOURCE mappedResource;
    // Map함수 : GPU 메모리의 텍스쳐를 CPU가 수정할 수 있게 해줌
    this->context_->Map(this->texture_.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);

    uint8_t* dest = static_cast<uint8_t*>(mappedResource.pData);// 텍스쳐 데이터 포인터
    int destPitch = mappedResource.RowPitch;// 텍스쳐의 한 줄(행)이 차지하는 바이트 수

    // 텍스쳐를 검은색으로
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

    // Unmap : 다시 GPU에게 텍스쳐 사용권 넘김
    this->context_->Unmap(this->texture_.Get(), 0);

    // 화면(백버퍼) 지우기(초기화)
    const float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    this->context_->ClearRenderTargetView(this->renderTargetView_.Get(), clearColor);

    // 텍스쳐를 픽셀셰이더에 바인딩
    this->context_->PSSetShaderResources(0, 1, this->shaderResourceView_texture.GetAddressOf());

    // 텍스쳐 샘플링에 사용할 샘플러 상태 설정
    this->context_->PSSetSamplers(0, 1, this->sampler_.GetAddressOf());

    this->context_->Draw(4, 0);//백버퍼에 랜더링 수행
    this->swapChain_->Present(1, 0);//백버퍼, 프론트 버퍼 교체하여 디스플레이(프론트 버퍼는 화면에 표시되는 것이고 백버퍼는 사전에 렌더링되는 것이다)
}

/// <summary>
/// 자원 해제
/// </summary>
void DirectX11Renderer::Cleanup() {
    // 모든 셰이더 리소스와 파이프라인 상태를 해제
    if (this->context_) {
        this->context_->ClearState(); 
    }

    if (this->swsContext_) {
        sws_freeContext(this->swsContext_);
        this->swsContext_ = nullptr;
    }

    // 스마트 포인터를 사용한 것들은 명시적 해제 불필요
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
    // 윈도우 클래스 등록
    const wchar_t CLASS_NAME[] = L"Window Class";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    // 윈도우 생성
    HWND hwnd = CreateWindowEx(
        0,                          // 확장 스타일
        CLASS_NAME,                 // 창 클래스 이름
        L"",                        // 창 제목
        WS_POPUP,                   // 창 스타일
        0, 0,                       // 창의 초기 위치
        width, height,              // 창의 초기 크기
        parentHwnd,                 // 부모 창
        nullptr,                    // 메뉴
        hInstance,                  // 인스턴스 핸들
        nullptr                     // 추가적인 애플리케이션 데이터
    );

    if (hwnd == nullptr) {
        return 0;
    }

    // 창 표시 및 업데이트
    ShowWindow(hwnd, 5);
    UpdateWindow(hwnd);

    return hwnd;
}