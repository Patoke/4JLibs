/*
MIT License

Copyright (c) 2026 Patoke

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "stdafx.h"
#include "Renderer.h"
#include "CompiledShaders.h"

Renderer InternalRenderManager;

DWORD Renderer::tlsIdx = TlsAlloc();
_RTL_CRITICAL_SECTION Renderer::totalAllocCS = {};

DWORD Renderer::s_auiWidths[]  = { 1920, 512, 256, 128, 64, 0 };
DWORD Renderer::s_auiHeights[] = { 1080, 512, 256, 128, 64 };
int Renderer::totalAlloc = 0;

D3D11_INPUT_ELEMENT_DESC g_vertex_PTN_Elements_PF3_TF2_CB4_NB4_XW1[] = {
    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM,  0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"NORMAL",   0, DXGI_FORMAT_R8G8B8A8_SNORM,  0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"TEXCOORD", 1, DXGI_FORMAT_R16G16_SINT,     0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0},
};

D3D11_INPUT_ELEMENT_DESC g_vertex_PTN_Elements_Compressed[] = {
    {"POSITION", 0, DXGI_FORMAT_R16G16B16A16_SINT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"TEXCOORD", 0, DXGI_FORMAT_R16G16B16A16_SINT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0},
};

D3D11_PRIMITIVE_TOPOLOGY Renderer::g_topologies[C4JRender::PRIMITIVE_TYPE_COUNT] = {
    D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
    D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, D3D11_PRIMITIVE_TOPOLOGY_LINELIST,      D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP,
};

static const unsigned int kVertexBufferSize = 0x100000;
static const unsigned int kScreenGrabWidth = 1920;
static const unsigned int kScreenGrabHeight = 1080;
static const unsigned int kThumbnailSize = 64;

static const unsigned int g_vertexStrides[C4JRender::VERTEX_TYPE_COUNT] = { 32, 16, 32, 32 };

Renderer::Context::Context(ID3D11Device* device, ID3D11DeviceContext* deviceContext)
    : m_pDeviceContext(deviceContext)
    , userAnnotation(NULL)
    , annotateDepth(0)
    , stackType(0)
    , textureIdx(0)
    , faceCullEnabled(true)
    , depthTestEnabled(true)
    , depthWriteEnabled(true)
    , alphaTestEnabled(false)
    , alphaReference(1.0f)
    , fogEnabled(false)
    , fogNearDistance(0.0f)
    , fogFarDistance(0.0f)
    , fogDensity(0.0f)
    , fogColourRed(0.0f)
    , fogColourGreen(0.0f)
    , fogColourBlue(0.0f)
    , fogMode(0)
    , lightingEnabled(false)
    , lightingDirty(false)
    , forcedLOD(-1)
    , m_modelViewMatrix(NULL)
    , m_localTransformMatrix(NULL)
    , m_projectionMatrix(NULL)
    , m_textureMatrix(NULL)
    , m_vertexTexcoordBuffer(NULL)
    , m_fogParamsBuffer(NULL)
    , m_lightingStateBuffer(NULL)
    , m_texGenMatricesBuffer(NULL)
    , m_compressedTranslationBuffer(NULL)
    , m_thumbnailBoundsBuffer(NULL)
    , m_tintColorBuffer(NULL)
    , m_fogColourBuffer(NULL)
    , m_unkColorBuffer(NULL)
    , m_alphaTestBuffer(NULL)
    , m_clearColorBuffer(NULL)
    , m_forcedLODBuffer(NULL)
    , dynamicVertexBase(0)
    , dynamicVertexOffset(0)
    , dynamicVertexBuffer(NULL)
    , commandBuffer(NULL)
    , recordingBufferIndex(0)
    , recordingVertexType(0)
    , recordingPrimitiveType(0)
    , deferredModeEnabled(false)
    , deferredBuffers()
{
    deviceContext->QueryInterface(IID_PPV_ARGS(&userAnnotation));
    memset(matrixStacks, 0, sizeof(matrixStacks));
    memset(matrixDirty, 0, sizeof(matrixDirty));
    memset(stackPos, 0, sizeof(stackPos));
    memset(lightEnabled, 0, sizeof(lightEnabled));
    memset(lightDirection, 0, sizeof(lightDirection));
    memset(lightColour, 0, sizeof(lightColour));
    memset(&lightAmbientColour, 0, sizeof(lightAmbientColour));
    memset(texGenMatrices, 0, sizeof(texGenMatrices));
    memset(&blendDesc, 0, sizeof(blendDesc));
    memset(&depthStencilDesc, 0, sizeof(depthStencilDesc));
    memset(&rasterizerDesc, 0, sizeof(rasterizerDesc));
    blendFactor[0] = 0.0f;
    blendFactor[1] = 0.0f;
    blendFactor[2] = 0.0f;
    blendFactor[3] = 0.0f;

    const DirectX::XMMATRIX identity = DirectX::XMMatrixIdentity();
    for (UINT i = 0; i < MATRIX_MODE_MODELVIEW_MAX; ++i)
    {
        matrixStacks[i][0] = identity;
        stackPos[i] = 0;
    }

    blendDesc.AlphaToCoverageEnable = false;
    blendDesc.IndependentBlendEnable = false;
    blendDesc.RenderTarget[0].BlendEnable = false;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    depthStencilDesc.DepthEnable = true;
    depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
    depthStencilDesc.StencilEnable = false;
    depthStencilDesc.StencilReadMask = 0xFF;
    depthStencilDesc.StencilWriteMask = 0xFF;
    depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
    depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

    rasterizerDesc.FillMode = D3D11_FILL_SOLID;
    rasterizerDesc.CullMode = D3D11_CULL_BACK;
    rasterizerDesc.FrontCounterClockwise = true;
    rasterizerDesc.DepthBias = 0;
    rasterizerDesc.DepthBiasClamp = 0.0f;
    rasterizerDesc.SlopeScaledDepthBias = 0.0f;
    rasterizerDesc.DepthClipEnable = true;
    rasterizerDesc.ScissorEnable = false;
    rasterizerDesc.MultisampleEnable = true;
    rasterizerDesc.AntialiasedLineEnable = false;

    memset(lightDirection, 0, sizeof(lightDirection));
    memset(lightColour, 0, sizeof(lightColour));
    memset(&lightAmbientColour, 0, sizeof(lightAmbientColour));
    memset(texGenMatrices, 0, sizeof(texGenMatrices));

    const float zero4[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    const float one4[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    const float alpha4[4] = {0.0f, 0.0f, 0.0f, 1.0f};

    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    D3D11_SUBRESOURCE_DATA cbData = {};
    cbDesc.ByteWidth = sizeof(DirectX::XMMATRIX);
    cbData.pSysMem = &identity;
    device->CreateBuffer(&cbDesc, &cbData, &m_modelViewMatrix);
    device->CreateBuffer(&cbDesc, &cbData, &m_localTransformMatrix);
    device->CreateBuffer(&cbDesc, &cbData, &m_projectionMatrix);
    device->CreateBuffer(&cbDesc, &cbData, &m_textureMatrix);

    cbDesc.ByteWidth = sizeof(zero4);
    cbData.pSysMem = zero4;
    device->CreateBuffer(&cbDesc, &cbData, &m_vertexTexcoordBuffer);
    device->CreateBuffer(&cbDesc, &cbData, &m_fogParamsBuffer);

    const UINT lightingBytes = sizeof(lightDirection) + sizeof(lightColour) + sizeof(lightAmbientColour);
    cbDesc.ByteWidth = lightingBytes;
    cbData.pSysMem = lightDirection;
    device->CreateBuffer(&cbDesc, &cbData, &m_lightingStateBuffer);

    cbDesc.ByteWidth = sizeof(texGenMatrices);
    cbData.pSysMem = texGenMatrices;
    device->CreateBuffer(&cbDesc, &cbData, &m_texGenMatricesBuffer);

    cbDesc.ByteWidth = sizeof(zero4);
    cbData.pSysMem = zero4;
    device->CreateBuffer(&cbDesc, &cbData, &m_compressedTranslationBuffer);
    device->CreateBuffer(&cbDesc, &cbData, &m_thumbnailBoundsBuffer);

    cbDesc.ByteWidth = sizeof(one4);
    cbData.pSysMem = one4;
    device->CreateBuffer(&cbDesc, &cbData, &m_tintColorBuffer);
    device->CreateBuffer(&cbDesc, &cbData, &m_fogColourBuffer);
    device->CreateBuffer(&cbDesc, &cbData, &m_unkColorBuffer);

    cbDesc.ByteWidth = sizeof(alpha4);
    cbData.pSysMem = alpha4;
    device->CreateBuffer(&cbDesc, &cbData, &m_alphaTestBuffer);

    cbDesc.ByteWidth = sizeof(zero4);
    cbData.pSysMem = zero4;
    device->CreateBuffer(&cbDesc, &cbData, &m_clearColorBuffer);
    device->CreateBuffer(&cbDesc, &cbData, &m_forcedLODBuffer);

    deviceContext->VSSetConstantBuffers(0, 10, &m_modelViewMatrix);
    deviceContext->PSSetConstantBuffers(0, 6, &m_tintColorBuffer);

    {
        void *dynamicVertexPtr = operator new[](kVertexBufferSize);
        dynamicVertexBase = reinterpret_cast<std::uint64_t>(dynamicVertexPtr);
    }
    dynamicVertexOffset = 0;

    D3D11_BUFFER_DESC vbDesc = {};
    vbDesc.ByteWidth = kVertexBufferSize;
    vbDesc.Usage = D3D11_USAGE_DYNAMIC;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    device->CreateBuffer(&vbDesc, NULL, &dynamicVertexBuffer);
}

void Renderer::BeginConditionalRendering(int) {}

void Renderer::BeginConditionalSurvey(int) {}

void Renderer::CaptureScreen(ImageFileBuffer *, XSOCIAL_PREVIEWIMAGE *) {}

void Renderer::Clear(int flags, D3D11_RECT *)
{
    PROFILER_SCOPE("Renderer::Clear", "Clear", MP_MAGENTA)

    Renderer::Context &c = getContext();

    ID3D11BlendState *blendState = NULL;
    ID3D11DepthStencilState *depthState = NULL;
    ID3D11RasterizerState *rasterizerState = NULL;

    PROFILER_SCOPE("Renderer::Clear", "Blend", MP_MAGENTA)
    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.AlphaToCoverageEnable = false;
    blendDesc.IndependentBlendEnable = false;
    blendDesc.RenderTarget[0].BlendEnable = false;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = (flags & CLEAR_COLOUR_FLAG) ? D3D11_COLOR_WRITE_ENABLE_ALL : 0;
    m_pDevice->CreateBlendState(&blendDesc, &blendState);

    PROFILER_SCOPE("Renderer::Clear", "Depth", MP_MAGENTA)
    D3D11_DEPTH_STENCIL_DESC depthDesc = {};
    depthDesc.DepthEnable = (flags & CLEAR_DEPTH_FLAG) ? true : false;
    depthDesc.DepthWriteMask = depthDesc.DepthEnable ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
    depthDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
    depthDesc.StencilEnable = false;
    depthDesc.StencilReadMask = 0xFF;
    depthDesc.StencilWriteMask = 0xFF;
    depthDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    depthDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    depthDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    depthDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
    depthDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    depthDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    depthDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    depthDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
    m_pDevice->CreateDepthStencilState(&depthDesc, &depthState);

    PROFILER_SCOPE("Renderer::Clear", "Rasterizer", MP_MAGENTA)
    D3D11_RASTERIZER_DESC rasterDesc = {};
    rasterDesc.FillMode = D3D11_FILL_SOLID;
    rasterDesc.CullMode = D3D11_CULL_NONE;
    rasterDesc.DepthClipEnable = true;
    rasterDesc.MultisampleEnable = true;
    m_pDevice->CreateRasterizerState(&rasterDesc, &rasterizerState);

    PROFILER_SCOPE("Renderer::Clear", "DrawClearQuad", MP_MAGENTA)
    c.m_pDeviceContext->VSSetShader(screenClearVertexShader, NULL, 0);
    c.m_pDeviceContext->IASetInputLayout(NULL);
    c.m_pDeviceContext->PSSetShader(screenClearPixelShader, NULL, 0);
    c.m_pDeviceContext->OMSetRenderTargets(1, &renderTargetView, depthStencilView);
    c.m_pDeviceContext->OMSetBlendState(blendState, NULL, 0xFFFFFFFF);
    c.m_pDeviceContext->OMSetDepthStencilState(depthState, 0);
    c.m_pDeviceContext->RSSetState(rasterizerState);
    c.m_pDeviceContext->PSSetShaderResources(0, 0, NULL);
    c.m_pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    c.m_pDeviceContext->Draw(4, 0);

    if (blendState)
    {
        blendState->Release();
        blendState = NULL;
    }
    if (depthState)
    {
        depthState->Release();
        depthState = NULL;
    }
    if (rasterizerState)
    {
        rasterizerState->Release();
        rasterizerState = NULL;
    }

    c.m_pDeviceContext->OMSetBlendState(GetManagedBlendState(), c.blendFactor, 0xFFFFFFFF);
    c.m_pDeviceContext->OMSetDepthStencilState(GetManagedDepthStencilState(), 0);
    c.m_pDeviceContext->RSSetState(GetManagedRasterizerState());
    c.m_pDeviceContext->OMSetRenderTargets(1, &renderTargetView, depthStencilView);
   
    activeVertexType = -1;
    activePixelType = -1;
}

void Renderer::ConvertLinearToPng(ImageFileBuffer *pngOut, unsigned char *linearData, unsigned int width, unsigned int height)
{
    const size_t dataSize = static_cast<size_t>(width) * static_cast<size_t>(height) * 4;
    const size_t outputCapacity = (dataSize * 24) / 20 + 256;

    void *outputBuffer = malloc(outputCapacity);
    int outputLength = 0;

    SaveTextureDataToMemory(
        outputBuffer, 
        static_cast<int>(outputCapacity), 
        &outputLength, 
        static_cast<int>(width), 
        static_cast<int>(height),
        reinterpret_cast<int *>(linearData)
    );
   
    pngOut->m_type = ImageFileBuffer::e_typePNG;
    pngOut->m_pBuffer = outputBuffer;
    pngOut->m_bufferSize = outputLength;
}

void Renderer::DoScreenGrabOnNextPresent() 
{ 
    m_bShouldScreenGrabNextFrame = true; 
}

void Renderer::EndConditionalRendering() {}
void Renderer::EndConditionalSurvey() {}

void Renderer::BeginEvent(LPCWSTR eventName)
{
    Renderer::Context &c = Renderer::getContext();
    if (c.m_pDeviceContext->GetType() != D3D11_DEVICE_CONTEXT_DEFERRED && c.userAnnotation)
    {
        c.userAnnotation->BeginEvent(eventName);
        ++c.annotateDepth;
    }
}

void Renderer::EndEvent()
{
    Renderer::Context &c = Renderer::getContext();
    if (c.m_pDeviceContext->GetType() != D3D11_DEVICE_CONTEXT_DEFERRED && c.userAnnotation)
    {
        c.userAnnotation->EndEvent();
        --c.annotateDepth;
        assert(c.annotateDepth >= 0);
    }
}

void Renderer::Initialise(ID3D11Device *pDevice, IDXGISwapChain *pSwapChain)
{
    m_pDevice = pDevice;
    m_pDeviceContext = InitialiseContext(true);
    m_pSwapChain = pSwapChain;

    #ifdef ENABLE_PROFILING
    MicroProfileOnThreadCreate("MainRenderThread");
    MicroProfileSetEnableAllGroups(true);
    #endif

    m_commandHandleToIndex = new int16_t[NUM_COMMAND_HANDLES];
    m_commandBuffers = new CommandBuffer *[MAX_COMMAND_BUFFERS];
    m_commandMatrices = new DirectX::XMMATRIX[MAX_COMMAND_BUFFERS];
    m_commandIndexToHandle = new int[MAX_COMMAND_BUFFERS];
    m_commandVertexTypes = new uint8_t[MAX_COMMAND_BUFFERS];
    m_commandPrimitiveTypes = new uint8_t[MAX_COMMAND_BUFFERS];

    memset(m_commandHandleToIndex, 0xFF, NUM_COMMAND_HANDLES * sizeof(int16_t));
    memset(m_commandBuffers, 0, MAX_COMMAND_BUFFERS * sizeof(CommandBuffer*));
    memset(m_commandIndexToHandle, 0, MAX_COMMAND_BUFFERS * sizeof(int));
    memset(m_commandVertexTypes, 0, MAX_COMMAND_BUFFERS * sizeof(uint8_t));
    memset(m_commandPrimitiveTypes, 0, MAX_COMMAND_BUFFERS * sizeof(uint8_t));

    reservedRendererDword3 = 0;
    m_bShouldScreenGrabNextFrame = false;
    m_bSuspended = false;

    SetupShaders();
    const float clearColour[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    SetClearColour(clearColour);

    UINT backBufferSampleCount = 1;
    UINT backBufferSampleQuality = 0;

    ID3D11Texture2D *backBuffer = NULL;
    pSwapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    if (backBuffer)
    {
        D3D11_TEXTURE2D_DESC backDesc = {};
        backBuffer->GetDesc(&backDesc);
        backBufferWidth = backDesc.Width;
        backBufferHeight = backDesc.Height;
        backBufferSampleCount = backDesc.SampleDesc.Count;
        backBufferSampleQuality = backDesc.SampleDesc.Quality;

        m_pDevice->CreateRenderTargetView(backBuffer, NULL, &renderTargetView);

        D3D11_TEXTURE2D_DESC srvDesc = backDesc;
        srvDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        srvDesc.MiscFlags = 0;

        ID3D11Texture2D *srvTexture = NULL;
        m_pDevice->CreateTexture2D(&srvDesc, NULL, &srvTexture);
        m_pDevice->CreateShaderResourceView(srvTexture, NULL, &renderTargetShaderResourceView);

        srvTexture->Release();
        backBuffer->Release();
    }

    ID3D11RenderTargetView *boundRTV = NULL;
    m_pDeviceContext->OMGetRenderTargets(1, &boundRTV, &depthStencilView);
    if (boundRTV)
        boundRTV->Release();

    if (!depthStencilView && backBufferWidth != 0 && backBufferHeight != 0)
    {
        D3D11_TEXTURE2D_DESC depthDesc = {};
        depthDesc.Width = backBufferWidth;
        depthDesc.Height = backBufferHeight;
        depthDesc.MipLevels = 1;
        depthDesc.ArraySize = 1;
        depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        depthDesc.SampleDesc.Count = backBufferSampleCount;
        depthDesc.SampleDesc.Quality = backBufferSampleQuality;
        depthDesc.Usage = D3D11_USAGE_DEFAULT;
        depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

        ID3D11Texture2D *depthTexture = NULL;
        if (SUCCEEDED(m_pDevice->CreateTexture2D(&depthDesc, NULL, &depthTexture)))
        {
            m_pDevice->CreateDepthStencilView(depthTexture, NULL, &depthStencilView);
            depthTexture->Release();
        }
    }

    D3D11_TEXTURE2D_DESC desc = {};
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;
    for (UINT i = 0; i < MAX_MIP_LEVELS - 1; ++i)
    {
        desc.Width = s_auiWidths[i + 1];
        desc.Height = s_auiHeights[i + 1];

        HRESULT hr = m_pDevice->CreateTexture2D(&desc, NULL, &renderTargetTextures[i]);
        assert(hr == S_OK);

        hr = m_pDevice->CreateRenderTargetView(renderTargetTextures[i], NULL, &renderTargetViews[i]);
        assert(hr == S_OK);

        hr = m_pDevice->CreateShaderResourceView(renderTargetTextures[i], NULL, &renderTargetShaderResourceViews[i]);
        assert(hr == S_OK);
    }

    memset(m_textures, 0, sizeof(m_textures));
    defaultTextureIndex = TextureCreate();
    TextureBind(defaultTextureIndex);

    unsigned char *defaultTextureData = new unsigned char[0x400];
    memset(defaultTextureData, 0xFF, 0x400);
    TextureData(16, 16, defaultTextureData, 0, C4JRender::TEXTURE_FORMAT_RxGyBzAw);
    delete[] defaultTextureData;

    presentCount = 0;
    rendererFlag0 = 0;
    reservedRendererWord0 = 10922;
    StateSetViewport(C4JRender::VIEWPORT_TYPE_FULLSCREEN);
    StateSetVertexTextureUV(0.0f, 0.0f);
    TextureBindVertex(-1);

    InitializeCriticalSection(&m_commandBufferCS);

    reservedRendererDword1 = 0;
    activeVertexType = -1;
    activePixelType = -1;
    reservedRendererByte1 = 1;
    reservedRendererByte0 = 0;

    unsigned short *quadIndices = new unsigned short[0x18000];
    for (UINT i = 0; i < 0x4000; ++i)
    {
        unsigned short base = static_cast<unsigned short>(i * 4);
        unsigned int offset = i * 6;
        quadIndices[offset + 0] = base;
        quadIndices[offset + 1] = base + 1;
        quadIndices[offset + 2] = base + 3;
        quadIndices[offset + 3] = base + 1;
        quadIndices[offset + 4] = base + 2;
        quadIndices[offset + 5] = base + 3;
    }

    D3D11_BUFFER_DESC quadIndexDesc = {};
    quadIndexDesc.ByteWidth = 0x30000;
    quadIndexDesc.Usage = D3D11_USAGE_IMMUTABLE;
    quadIndexDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    quadIndexDesc.CPUAccessFlags = 0;
    quadIndexDesc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA quadIndexData = {};
    quadIndexData.pSysMem = quadIndices;
    HRESULT hr = m_pDevice->CreateBuffer(&quadIndexDesc, &quadIndexData, &quadIndexBuffer);
    assert(hr >= 0);
    delete[] quadIndices;

    unsigned short *fanIndices = new unsigned short[0x2FFFA];
    for (UINT i = 0; i < 65534; ++i)
    {
        unsigned int offset = i * 3;
        fanIndices[offset + 0] = 0;
        fanIndices[offset + 1] = static_cast<unsigned short>(i + 1);
        fanIndices[offset + 2] = static_cast<unsigned short>(i + 2);
    }

    D3D11_BUFFER_DESC fanIndexDesc = {};
    fanIndexDesc.ByteWidth = 0x5FFF4;
    fanIndexDesc.Usage = D3D11_USAGE_IMMUTABLE;
    fanIndexDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA fanIndexData = {};
    fanIndexData.pSysMem = fanIndices;
    m_pDevice->CreateBuffer(&fanIndexDesc, &fanIndexData, &fanIndexBuffer);
    delete[] fanIndices;

    InitializeCriticalSection(&Renderer::totalAllocCS);
}

ID3D11DeviceContext *Renderer::InitialiseContext(bool fromPresent)
{
    ID3D11DeviceContext *deviceContext = NULL;

    if (fromPresent)
        m_pDevice->GetImmediateContext(&deviceContext);
    else
        m_pDevice->CreateDeferredContext(0, &deviceContext);

    Renderer::Context *c = new (std::nothrow) Renderer::Context(m_pDevice, deviceContext);
    TlsSetValue(Renderer::tlsIdx, c);

    return deviceContext;
}

bool Renderer::IsHiDef()
{
    return true;
}

bool Renderer::IsWidescreen()
{
    return true;
}

void Renderer::Present()
{
    PROFILER_SCOPE("Renderer::Present", "Present", MP_MAGENTA)

    if (m_bShouldScreenGrabNextFrame)
    {
        PROFILER_SCOPE("Renderer::Present", "ScreenGrab", MP_MAGENTA)

        unsigned char *linearData = new unsigned char[kScreenGrabWidth * kScreenGrabHeight * 4];
        
        ID3D11Texture2D *backBuffer = NULL;
        ID3D11Texture2D *stagingTexture = NULL;

        m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
        if (backBuffer)
        {
            D3D11_TEXTURE2D_DESC desc = {};
            backBuffer->GetDesc(&desc);
            desc.Usage = D3D11_USAGE_STAGING;
            desc.BindFlags = 0;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
            desc.MiscFlags = 0;
            m_pDevice->CreateTexture2D(&desc, NULL, &stagingTexture);
        }

        if (stagingTexture && backBuffer)
        {
            PROFILER_SCOPE("Renderer::Present", "CopyResource", MP_MAGENTA)
            m_pDeviceContext->CopyResource(stagingTexture, backBuffer);

            D3D11_MAPPED_SUBRESOURCE mapped = {};
            if (SUCCEEDED(m_pDeviceContext->Map(stagingTexture, 0, D3D11_MAP_READ, 0, &mapped)))
            {
                const unsigned char *src = reinterpret_cast<const unsigned char *>(mapped.pData);

                for (UINT y = 0; y < kScreenGrabHeight; ++y)
                {
                    unsigned char *dstRow = linearData + y * kScreenGrabWidth * 4;
                    const unsigned char *srcRow = src + y * mapped.RowPitch;
                    memcpy(dstRow, srcRow, kScreenGrabWidth * 4);

                    for (UINT x = 0; x < kScreenGrabWidth; ++x)
                        dstRow[x * 4 + 3] = 0xFF;
                }

                m_pDeviceContext->Unmap(stagingTexture, 0);
            }
        }

        static int count = 0;
        char fileName[304];
        sprintf_s(fileName, "d:\\screen%d.png", count++);

        D3DXIMAGE_INFO info;
        info.Width = kScreenGrabWidth;
        info.Height = kScreenGrabHeight;
        SaveTextureData(fileName, &info, reinterpret_cast<int *>(linearData));

        delete[] linearData;

        if (stagingTexture)
        {
            stagingTexture->Release();
            stagingTexture = NULL;
        }
        if (backBuffer)
        {
            backBuffer->Release();
            backBuffer = NULL;
        }

        m_bShouldScreenGrabNextFrame = false;
    }

    m_pSwapChain->Present(1, 0);
    ++presentCount;
}

void Renderer::Resume()
{
    m_bSuspended = false;
}

void Renderer::SetClearColour(const float colourRGBA[4])
{
    for (int i = 0; i < 4; ++i)
        m_fClearColor[i] = colourRGBA[i];

    Renderer::Context &c = getContext();
    if (&c)
    {
        D3D11_MAPPED_SUBRESOURCE mapped = {};
        if (SUCCEEDED(c.m_pDeviceContext->Map(c.m_clearColorBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            *(DirectX::XMVECTOR*)mapped.pData = DirectX::XMVectorSet(colourRGBA[0], colourRGBA[1], colourRGBA[2], colourRGBA[3]);
            c.m_pDeviceContext->Unmap(c.m_clearColorBuffer, 0);
        }
    }
}

void Renderer::SetupShaders()
{
    vertexShaderTable = new ID3D11VertexShader *[C4JRender::VERTEX_TYPE_COUNT];
    pixelShaderTable = new ID3D11PixelShader *[C4JRender::PIXEL_SHADER_COUNT];
    vertexStrideTable = new unsigned int[C4JRender::VERTEX_TYPE_COUNT];
    inputLayoutTable = new ID3D11InputLayout *[C4JRender::VERTEX_TYPE_COUNT];

    for (UINT i = 0; i < C4JRender::VERTEX_TYPE_COUNT; ++i)
    {
        vertexShaderTable[i] = NULL;
        inputLayoutTable[i] = NULL;
        vertexStrideTable[i] = g_vertexStrides[i];
    }

    for (UINT i = 0; i < C4JRender::PIXEL_SHADER_COUNT; ++i)
    {
        pixelShaderTable[i] = NULL;
    }

    screenSpaceVertexShader = NULL;
    screenClearVertexShader = NULL;
    screenSpacePixelShader = NULL;
    screenClearPixelShader = NULL;

    m_pDevice->CreateVertexShader(g_main_VS_PF3_TF2_CB4_NB4_XW1, sizeof(g_main_VS_PF3_TF2_CB4_NB4_XW1), NULL, &vertexShaderTable[C4JRender::VERTEX_TYPE_PF3_TF2_CB4_NB4_XW1]);
    m_pDevice->CreateVertexShader(g_main_VS_Compressed, sizeof(g_main_VS_Compressed), NULL, &vertexShaderTable[C4JRender::VERTEX_TYPE_COMPRESSED]);
    m_pDevice->CreateVertexShader(g_main_VS_PF3_TF2_CB4_NB4_XW1_LIGHTING, sizeof(g_main_VS_PF3_TF2_CB4_NB4_XW1_LIGHTING), NULL, &vertexShaderTable[C4JRender::VERTEX_TYPE_PF3_TF2_CB4_NB4_XW1_LIT]);
    m_pDevice->CreateVertexShader(g_main_VS_PF3_TF2_CB4_NB4_XW1_TEXGEN, sizeof(g_main_VS_PF3_TF2_CB4_NB4_XW1_TEXGEN), NULL, &vertexShaderTable[C4JRender::VERTEX_TYPE_PF3_TF2_CB4_NB4_XW1_TEXGEN]);
    m_pDevice->CreateVertexShader(g_main_VS_ScreenSpace, sizeof(g_main_VS_ScreenSpace), NULL, &screenSpaceVertexShader);
    m_pDevice->CreateVertexShader(g_main_VS_ScreenClear, sizeof(g_main_VS_ScreenClear), NULL, &screenClearVertexShader);

    m_pDevice->CreatePixelShader(g_main_PS_Standard, sizeof(g_main_PS_Standard), NULL, &pixelShaderTable[C4JRender::PIXEL_SHADER_TYPE_STANDARD]);
    m_pDevice->CreatePixelShader(g_main_PS_TextureProjection, sizeof(g_main_PS_TextureProjection), NULL, &pixelShaderTable[C4JRender::PIXEL_SHADER_TYPE_PROJECTION]);
    m_pDevice->CreatePixelShader(g_main_PS_ForceLOD, sizeof(g_main_PS_ForceLOD), NULL, &pixelShaderTable[C4JRender::PIXEL_SHADER_TYPE_FORCELOD]);
    m_pDevice->CreatePixelShader(g_main_PS_ScreenSpace, sizeof(g_main_PS_ScreenSpace), NULL, &screenSpacePixelShader);
    m_pDevice->CreatePixelShader(g_main_PS_ScreenClear, sizeof(g_main_PS_ScreenClear), NULL, &screenClearPixelShader);

    m_pDevice->CreateInputLayout(g_vertex_PTN_Elements_PF3_TF2_CB4_NB4_XW1, 5, g_main_VS_PF3_TF2_CB4_NB4_XW1, sizeof(g_main_VS_PF3_TF2_CB4_NB4_XW1), &inputLayoutTable[C4JRender::VERTEX_TYPE_PF3_TF2_CB4_NB4_XW1]);
    m_pDevice->CreateInputLayout(g_vertex_PTN_Elements_Compressed, 2, g_main_VS_Compressed, sizeof(g_main_VS_Compressed), &inputLayoutTable[C4JRender::VERTEX_TYPE_COMPRESSED]);
    m_pDevice->CreateInputLayout(g_vertex_PTN_Elements_PF3_TF2_CB4_NB4_XW1, 5, g_main_VS_PF3_TF2_CB4_NB4_XW1_LIGHTING, sizeof(g_main_VS_PF3_TF2_CB4_NB4_XW1_LIGHTING), &inputLayoutTable[C4JRender::VERTEX_TYPE_PF3_TF2_CB4_NB4_XW1_LIT]);
    m_pDevice->CreateInputLayout(g_vertex_PTN_Elements_PF3_TF2_CB4_NB4_XW1, 5, g_main_VS_PF3_TF2_CB4_NB4_XW1_TEXGEN, sizeof(g_main_VS_PF3_TF2_CB4_NB4_XW1_TEXGEN), &inputLayoutTable[C4JRender::VERTEX_TYPE_PF3_TF2_CB4_NB4_XW1_TEXGEN]);
}

void Renderer::StartFrame()
{
    PROFILER_SCOPE("Renderer::StartFrame", "StartFrame", MP_MAGENTA)

    Renderer::Context &c = getContext();

    activeVertexType = -1;
    activePixelType = -1;

    TextureBindVertex(-1);
    TextureBind(-1);

    PROFILER_SCOPE("Renderer::StartFrame", "State", MP_MAGENTA)

    StateSetColour(1.0f, 1.0f, 1.0f, 1.0f);
    StateSetDepthMask(true);
    StateSetBlendEnable(true);
    StateSetBlendFunc(D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA);
    StateSetBlendFactor(0xFFFFFFFF);
    StateSetAlphaFunc(D3D11_COMPARISON_GREATER, 0.1f);
    StateSetDepthFunc(D3D11_COMPARISON_LESS_EQUAL);
    StateSetFaceCull(true);
    StateSetLineWidth(1.0f);
    StateSetWriteEnable(true, true, true, true);
    StateSetDepthTestEnable(false);
    StateSetAlphaTestEnable(true);

    c.m_pDeviceContext->VSSetConstantBuffers(0, 10, &c.m_modelViewMatrix);
    c.m_pDeviceContext->PSSetConstantBuffers(0, 6, &c.m_tintColorBuffer);

    D3D11_VIEWPORT viewport = {};
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    viewport.Width = (float)backBufferWidth;
    viewport.Height = (float)backBufferHeight;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    c.m_pDeviceContext->RSSetViewports(1, &viewport);
    c.m_pDeviceContext->OMSetRenderTargets(1, &renderTargetView, depthStencilView);

    #ifdef ENABLE_PROFILING
    MicroProfileFlip(nullptr);
    #endif
}

void Renderer::Suspend()
{
    m_bSuspended = true;
}

bool Renderer::Suspended()
{
    return m_bSuspended;
}

void Renderer::UpdateGamma(unsigned short) {}

Renderer::Context &Renderer::getContext()
{
    return *reinterpret_cast<Renderer::Context *>(TlsGetValue(Renderer::tlsIdx));
}

void Renderer::CaptureThumbnail(ImageFileBuffer *pngOut)
{
    Renderer::Context &c = getContext();

    float left = 0.0f;
    float bottom = 0.0f;
    float right = 1.0f;
    float top = 1.0f;
    
    switch (m_ViewportType)
    {
    case C4JRender::VIEWPORT_TYPE_SPLIT_TOP:
        bottom = 0.5f;
        break;
    case C4JRender::VIEWPORT_TYPE_SPLIT_BOTTOM:
        top = 0.5f;
        break;
    case C4JRender::VIEWPORT_TYPE_SPLIT_LEFT:
        right = 0.5f;
        break;
    case C4JRender::VIEWPORT_TYPE_SPLIT_RIGHT:
        left = 0.5f;
        break;
    case C4JRender::VIEWPORT_TYPE_QUADRANT_TOP_LEFT:
        right = 0.5f;
        bottom = 0.5f;
        break;
    case C4JRender::VIEWPORT_TYPE_QUADRANT_TOP_RIGHT:
        left = 0.5f;
        bottom = 0.5f;
        break;
    case C4JRender::VIEWPORT_TYPE_QUADRANT_BOTTOM_LEFT:
        right = 0.5f;
        top = 0.5f;
        break;
    case C4JRender::VIEWPORT_TYPE_QUADRANT_BOTTOM_RIGHT:
        left = 0.5f;
        top = 0.5f;
        break;
    default:
        break;
    }

    float aspectRatio = IsWidescreen() ? (16.0f / 9.0f) : (4.0f / 3.0f);

    right *= aspectRatio;
    left  *= aspectRatio;

    float width  = right - left;
    float height = top - bottom;

    if (height > width)
    {
        float diff = (height - width) * 0.5f;
        bottom += diff;
        top    -= diff;
    }
    else
    {
        float diff = (width - height) * 0.5f;
        left  += diff;
        right -= diff;
    }

    left  /= aspectRatio;
    right /= aspectRatio;

    ID3D11BlendState *blendState = NULL;
    ID3D11DepthStencilState *depthState = NULL;
    ID3D11RasterizerState *rasterizerState = NULL;
    ID3D11SamplerState *samplerState = NULL;
    ID3D11Texture2D *stagingTexture = NULL;

    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = false;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    m_pDevice->CreateBlendState(&blendDesc, &blendState);

    D3D11_DEPTH_STENCIL_DESC depthDesc = {};
    depthDesc.DepthEnable = false;
    depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    depthDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
    depthDesc.StencilEnable = false;
    depthDesc.StencilReadMask = 0xFF;
    depthDesc.StencilWriteMask = 0xFF;
    depthDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    depthDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    depthDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    depthDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
    depthDesc.BackFace = depthDesc.FrontFace;
    m_pDevice->CreateDepthStencilState(&depthDesc, &depthState);

    D3D11_RASTERIZER_DESC rasterDesc = {};
    rasterDesc.FillMode = D3D11_FILL_SOLID;
    rasterDesc.CullMode = D3D11_CULL_NONE;
    rasterDesc.DepthClipEnable = true;
    rasterDesc.MultisampleEnable = true;
    m_pDevice->CreateRasterizerState(&rasterDesc, &rasterizerState);

    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.MaxAnisotropy = 1;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    samplerDesc.MinLOD = 0.0f;
    samplerDesc.MaxLOD = (std::numeric_limits<float>::max)();
    m_pDevice->CreateSamplerState(&samplerDesc, &samplerState);

    c.m_pDeviceContext->VSSetShader(screenSpaceVertexShader, NULL, 0);
    c.m_pDeviceContext->IASetInputLayout(NULL);
    c.m_pDeviceContext->PSSetShader(screenSpacePixelShader, NULL, 0);
    c.m_pDeviceContext->OMSetBlendState(blendState, NULL, -1);
    c.m_pDeviceContext->OMSetDepthStencilState(depthState, 0);
    c.m_pDeviceContext->RSSetState(rasterizerState);

    for (UINT i = 0; i < MAX_MIP_LEVELS - 1; ++i)
    {
        D3D11_VIEWPORT viewport = {};
        viewport.TopLeftX = 0.0f;
        viewport.TopLeftY = 0.0f;
        viewport.Width = (float)s_auiWidths[i + 1];
        viewport.Height = (float)s_auiHeights[i + 1];
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;

        c.m_pDeviceContext->OMSetRenderTargets(1, &renderTargetViews[i], NULL);
        c.m_pDeviceContext->RSSetViewports(1, &viewport);

        ID3D11ShaderResourceView *inputTexture = (i == 0) ? renderTargetShaderResourceView : renderTargetShaderResourceViews[i - 1];
        c.m_pDeviceContext->PSSetShaderResources(0, 1, &inputTexture);
        c.m_pDeviceContext->PSSetSamplers(0, 1, &samplerState);
        c.m_pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

        D3D11_MAPPED_SUBRESOURCE mapped = {};
        c.m_pDeviceContext->Map(c.m_thumbnailBoundsBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);

        float *constants = (float *)mapped.pData;
        if (i == 0)
        {
            constants[0] = left;
            constants[1] = bottom;
            constants[2] = right - left;
            constants[3] = top - bottom;
        }
        else
        {
            constants[0] = 0.0f;
            constants[1] = 0.0f;
            constants[2] = 1.0f;
            constants[3] = 1.0f;
        }

        c.m_pDeviceContext->Unmap(c.m_thumbnailBoundsBuffer, 0);
        c.m_pDeviceContext->Draw(4, 0);
    }

    D3D11_TEXTURE2D_DESC texDesc = {};
    renderTargetTextures[MAX_MIP_LEVELS - 2]->GetDesc(&texDesc);
    texDesc.Usage = D3D11_USAGE_STAGING;
    texDesc.BindFlags = 0;
    texDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    texDesc.MiscFlags = 0;
    m_pDevice->CreateTexture2D(&texDesc, NULL, &stagingTexture);

    const unsigned int stride = kThumbnailSize * 4;
    unsigned char *linearData = new unsigned char[kThumbnailSize * stride];

    if (stagingTexture)
    {
        c.m_pDeviceContext->CopyResource(stagingTexture, renderTargetTextures[MAX_MIP_LEVELS - 2]);

        D3D11_MAPPED_SUBRESOURCE mapped = {};
        if (SUCCEEDED(c.m_pDeviceContext->Map(stagingTexture, 0, D3D11_MAP_READ, 0, &mapped)))
        {
            const unsigned char *src = static_cast<const unsigned char *>(mapped.pData);
            unsigned char *dst = linearData;

            for (UINT y = 0; y < kThumbnailSize; ++y)
            {
                memcpy(dst, src, stride);

                unsigned char *alpha = dst + 3;
                for (UINT x = 0; x < kThumbnailSize; ++x)
                {
                    *alpha = 0xFF;
                    alpha += 4;
                }

                src += mapped.RowPitch;
                dst += stride;
            }

            c.m_pDeviceContext->Unmap(stagingTexture, 0);
        }
    }

    ConvertLinearToPng(pngOut, linearData, kThumbnailSize, kThumbnailSize);
    delete[] linearData;

    if (stagingTexture)
    {
        stagingTexture->Release();
        stagingTexture = NULL;
    }
    if (samplerState)
    {
        samplerState->Release();
        samplerState = NULL;
    }
    if (rasterizerState)
    {
        rasterizerState->Release();
        rasterizerState = NULL;
    }
    if (depthState)
    {
        depthState->Release();
        depthState = NULL;
    }
    if (blendState)
    {
        blendState->Release();
        blendState = NULL;
    }

    c.m_pDeviceContext->OMSetBlendState(GetManagedBlendState(), c.blendFactor, -1);
    c.m_pDeviceContext->OMSetDepthStencilState(GetManagedDepthStencilState(), 0);
    c.m_pDeviceContext->RSSetState(GetManagedRasterizerState());

    D3D11_VIEWPORT viewport = {};
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    viewport.Width = (float)backBufferWidth;
    viewport.Height = (float)backBufferHeight;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    c.m_pDeviceContext->RSSetViewports(1, &viewport);
    c.m_pDeviceContext->OMSetRenderTargets(1, &renderTargetView, depthStencilView);

    activeVertexType = -1;
    activePixelType = -1;
}
