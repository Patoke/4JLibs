#pragma once
#include "Renderer.h"

#include <cstddef>
#include <cstring>
#include <limits>

ID3D11BlendState *Renderer::GetManagedBlendState()
{
    Context &c = this->getContext();
    const D3D11_RENDER_TARGET_BLEND_DESC &rtBlend = c.blendDesc.RenderTarget[0];

    const int key = (rtBlend.BlendEnable ? 1 : 0) | ((static_cast<int>(rtBlend.SrcBlend) & 0x1F) << 1) |
                    ((static_cast<int>(rtBlend.DestBlend) & 0x1F) << 6) | ((static_cast<int>(rtBlend.RenderTargetWriteMask) & 0x0F) << 11);

    auto it = managedBlendStates.find(key);
    if (it != managedBlendStates.end())
    {
        return it->second;
    }

    ID3D11BlendState *state = nullptr;
    m_pDevice->CreateBlendState(&c.blendDesc, &state);
    managedBlendStates.emplace(key, state);
    return state;
}

ID3D11DepthStencilState *Renderer::GetManagedDepthStencilState()
{
    Context &c = this->getContext();

    const int key = (c.depthStencilDesc.DepthEnable ? 2 : 0) | ((static_cast<int>(c.depthStencilDesc.DepthFunc) & 0x0F) << 2) |
                    (c.depthStencilDesc.DepthWriteMask == D3D11_DEPTH_WRITE_MASK_ALL ? 1 : 0);

    auto it = managedDepthStencilStates.find(key);
    if (it != managedDepthStencilStates.end())
    {
        return it->second;
    }

    ID3D11DepthStencilState *state = nullptr;
    m_pDevice->CreateDepthStencilState(&c.depthStencilDesc, &state);
    managedDepthStencilStates.emplace(key, state);
    return state;
}

ID3D11RasterizerState *Renderer::GetManagedRasterizerState()
{
    Context &c = this->getContext();

    const int key = (static_cast<std::uint8_t>(c.rasterizerDesc.DepthBias)) |
                    (static_cast<std::uint8_t>(static_cast<int>(c.rasterizerDesc.SlopeScaledDepthBias)) << 8) |
                    ((static_cast<int>(c.rasterizerDesc.CullMode) & 0x03) << 16);

    auto it = managedRasterizerStates.find(key);
    if (it != managedRasterizerStates.end())
    {
        return it->second;
    }

    ID3D11RasterizerState *state = nullptr;
    m_pDevice->CreateRasterizerState(&c.rasterizerDesc, &state);
    managedRasterizerStates.emplace(key, state);
    return state;
}

ID3D11SamplerState *Renderer::GetManagedSamplerState()
{
    Context &c = this->getContext();
    const int key = m_textures[c.textureIdx].samplerParams;

    auto it = managedSamplerStates.find(key);
    if (it != managedSamplerStates.end())
    {
        return it->second;
    }

    const bool clampU = (key & 0x01) != 0;
    const bool clampV = (key & 0x02) != 0;
    const bool linearFilter = (key & 0x04) != 0;
    const bool mipLinear = (key & 0x08) != 0;
    const int filterBits = (mipLinear ? 0x08 : 0x00) | (linearFilter ? 0x22 : 0x02);

    D3D11_SAMPLER_DESC desc = {};
    desc.Filter = static_cast<D3D11_FILTER>(filterBits >> 1);
    desc.AddressU = clampU ? D3D11_TEXTURE_ADDRESS_CLAMP : D3D11_TEXTURE_ADDRESS_WRAP;
    desc.AddressV = clampV ? D3D11_TEXTURE_ADDRESS_CLAMP : D3D11_TEXTURE_ADDRESS_WRAP;
    desc.AddressW = static_cast<D3D11_TEXTURE_ADDRESS_MODE>(3);
    desc.MipLODBias = 0.0f;
    desc.MaxAnisotropy = 16;
    desc.ComparisonFunc = static_cast<D3D11_COMPARISON_FUNC>(1);
    desc.BorderColor[0] = 0.0f;
    desc.BorderColor[1] = 0.0f;
    desc.BorderColor[2] = 0.0f;
    desc.BorderColor[3] = 0.0f;
    desc.MinLOD = -(std::numeric_limits<float>::max)();
    desc.MaxLOD = (std::numeric_limits<float>::max)();

    ID3D11SamplerState *state = nullptr;
    m_pDevice->CreateSamplerState(&desc, &state);
    managedSamplerStates.emplace(key, state);
    return state;
}

void Renderer::StateSetFogEnable(bool enable)
{
    Context &c = this->getContext();
    c.fogEnabled = enable ? 1 : 0;
}

void Renderer::StateSetFogMode(int mode)
{
    Context &c = this->getContext();
    c.fogMode = mode;
}

void Renderer::StateSetFogNearDistance(float dist)
{
    Context &c = this->getContext();
    c.fogNearDistance = dist;
}

void Renderer::StateSetFogFarDistance(float dist)
{
    Context &c = this->getContext();
    c.fogFarDistance = dist;
}

void Renderer::StateSetFogDensity(float density)
{
    Context &c = this->getContext();
    c.fogDensity = density;
}

void Renderer::StateSetFogColour(float red, float green, float blue)
{
    Context &c = this->getContext();
    c.fogColourRed = red;
    c.fogColourBlue = blue;
    c.fogColourGreen = green;
}

void Renderer::UpdateViewportState() {}

void Renderer::StateSetLightingEnable(bool enable)
{
    Context &c = this->getContext();
    if (c.commandBuffer != nullptr && c.commandBuffer->isActive != 0)
    {
        c.commandBuffer->SetLightingEnable(enable);
        return;
    }

    c.lightingEnabled = enable ? 1 : 0;
}

void Renderer::StateSetLightColour(int light, float red, float green, float blue)
{
    if (light >= 2)
    {
        return;
    }

    Context &c = this->getContext();
    if (c.commandBuffer != nullptr && c.commandBuffer->isActive != 0)
    {
        c.commandBuffer->SetLightColour(light, red, green, blue);
        return;
    }

    c.lightColour[light].x = red;
    c.lightColour[light].y = green;
    c.lightColour[light].z = blue;
    c.lightColour[light].w = 1.0f;
    c.lightingDirty = 1;
}

void Renderer::StateSetLightAmbientColour(float red, float green, float blue)
{
    Context &c = this->getContext();
    if (c.commandBuffer != nullptr && c.commandBuffer->isActive != 0)
    {
        c.commandBuffer->SetLightAmbientColour(red, green, blue);
        return;
    }

    c.lightAmbientColour.x = red;
    c.lightAmbientColour.y = green;
    c.lightAmbientColour.z = blue;
    c.lightAmbientColour.w = 1.0f;
    c.lightingDirty = 1;
}

void Renderer::StateSetLightEnable(int light, bool enable)
{
    if (light >= 2)
    {
        return;
    }

    Context &c = this->getContext();
    if (c.commandBuffer != nullptr && c.commandBuffer->isActive != 0)
    {
        c.commandBuffer->SetLightEnable(light, enable);
        return;
    }

    c.lightEnabled[light] = enable ? 1 : 0;
    c.lightingDirty = 1;
}

void Renderer::StateSetColour(float r, float g, float b, float a)
{
    Context &c = this->getContext();
    if (c.commandBuffer != nullptr && c.commandBuffer->isActive != 0)
    {
        c.commandBuffer->SetColor(r, g, b, a);
        return;
    }

    ID3D11DeviceContext *d3d11 = c.m_pDeviceContext;
    const float colour[4] = {r, g, b, a};

    D3D11_MAPPED_SUBRESOURCE mapped = {};
    d3d11->Map(c.m_tintColorBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    std::memcpy(mapped.pData, colour, sizeof(colour));
    d3d11->Unmap(c.m_tintColorBuffer, 0);
}

void Renderer::StateSetDepthMask(bool enable)
{
    Context &c = this->getContext();
    if (c.commandBuffer != nullptr && c.commandBuffer->isActive != 0)
    {
        c.commandBuffer->SetDepthMask(enable);
        return;
    }

    c.depthStencilDesc.DepthWriteMask = enable ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
    c.m_pDeviceContext->OMSetDepthStencilState(this->GetManagedDepthStencilState(), 0);
    c.depthWriteEnabled = enable ? 1 : 0;
}

void Renderer::StateSetBlendEnable(bool enable)
{
    Context &c = this->getContext();
    if (c.commandBuffer != nullptr && c.commandBuffer->isActive != 0)
    {
        c.commandBuffer->SetBlendEnable(enable);
        return;
    }

    c.blendDesc.RenderTarget[0].BlendEnable = enable ? TRUE : FALSE;
    c.m_pDeviceContext->OMSetBlendState(this->GetManagedBlendState(), c.blendFactor, 0xFFFFFFFFu);
}

void Renderer::StateSetBlendFunc(int src, int dst)
{
    Context &c = this->getContext();
    if (c.commandBuffer != nullptr && c.commandBuffer->isActive != 0)
    {
        c.commandBuffer->SetBlendFunc(src, dst);
        return;
    }

    c.blendDesc.RenderTarget[0].SrcBlend = static_cast<D3D11_BLEND>(src);
    c.blendDesc.RenderTarget[0].DestBlend = static_cast<D3D11_BLEND>(dst);
    c.m_pDeviceContext->OMSetBlendState(this->GetManagedBlendState(), c.blendFactor, 0xFFFFFFFFu);
}

void Renderer::StateSetBlendFactor(unsigned int colour)
{
    Context &c = this->getContext();
    if (c.commandBuffer != nullptr && c.commandBuffer->isActive != 0)
    {
        c.commandBuffer->SetBlendFactor(colour);
        return;
    }

    const float scale = 255.0f;
    c.blendFactor[0] = static_cast<float>((colour >> 0) & 0xFFu) / scale;
    c.blendFactor[1] = static_cast<float>((colour >> 8) & 0xFFu) / scale;
    c.blendFactor[2] = static_cast<float>((colour >> 16) & 0xFFu) / scale;
    c.blendFactor[3] = static_cast<float>((colour >> 24) & 0xFFu) / scale;
    c.m_pDeviceContext->OMSetBlendState(this->GetManagedBlendState(), c.blendFactor, 0xFFFFFFFFu);
}

void Renderer::StateSetAlphaFunc(int, float param)
{
    Context &c = this->getContext();
    c.alphaReference = param;

    const float alpha[4] = {0.0f, 0.0f, 0.0f, c.alphaTestEnabled ? c.alphaReference : 0.0f};
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    c.m_pDeviceContext->Map(c.m_alphaTestBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    std::memcpy(mapped.pData, alpha, sizeof(alpha));
    c.m_pDeviceContext->Unmap(c.m_alphaTestBuffer, 0);
}

void Renderer::StateSetDepthFunc(int func)
{
    Context &c = this->getContext();
    if (c.commandBuffer != nullptr && c.commandBuffer->isActive != 0)
    {
        c.commandBuffer->SetDepthFunc(func);
        return;
    }

    c.depthStencilDesc.DepthFunc = static_cast<D3D11_COMPARISON_FUNC>(func);
    c.m_pDeviceContext->OMSetDepthStencilState(this->GetManagedDepthStencilState(), 0);
}

void Renderer::StateSetFaceCull(bool enable)
{
    Context &c = this->getContext();
    if (c.commandBuffer != nullptr && c.commandBuffer->isActive != 0)
    {
        c.commandBuffer->SetFaceCull(enable);
        return;
    }

    c.rasterizerDesc.CullMode = enable ? D3D11_CULL_BACK : D3D11_CULL_NONE;
    c.m_pDeviceContext->RSSetState(this->GetManagedRasterizerState());
    c.faceCullEnabled = enable ? 1 : 0;
}

void Renderer::StateSetFaceCullCW(bool enable)
{
    Context &c = this->getContext();
    if (c.faceCullEnabled)
    {
        c.rasterizerDesc.CullMode = enable ? D3D11_CULL_BACK : D3D11_CULL_FRONT;
    }
    else
    {
        c.rasterizerDesc.CullMode = D3D11_CULL_NONE;
    }

    c.m_pDeviceContext->RSSetState(this->GetManagedRasterizerState());
}

void Renderer::StateSetLineWidth(float) {}

void Renderer::StateSetWriteEnable(bool red, bool green, bool blue, bool alpha)
{
    Context &c = this->getContext();

    std::uint8_t mask = 0;
    mask |= red ? 0x1 : 0;
    mask |= green ? 0x2 : 0;
    mask |= blue ? 0x4 : 0;
    mask |= alpha ? 0x8 : 0;

    c.blendDesc.RenderTarget[0].RenderTargetWriteMask = mask;
    c.m_pDeviceContext->OMSetBlendState(this->GetManagedBlendState(), c.blendFactor, 0xFFFFFFFFu);
}

void Renderer::StateSetDepthTestEnable(bool enable)
{
    Context &c = this->getContext();
    if (c.commandBuffer != nullptr && c.commandBuffer->isActive != 0)
    {
        c.commandBuffer->SetDepthTestEnable(enable);
        return;
    }

    c.depthStencilDesc.DepthEnable = enable ? TRUE : FALSE;
    c.m_pDeviceContext->OMSetDepthStencilState(this->GetManagedDepthStencilState(), 0);
    c.depthTestEnabled = enable ? 1 : 0;
}

void Renderer::StateSetAlphaTestEnable(bool enable)
{
    Context &c = this->getContext();
    c.alphaTestEnabled = enable ? 1 : 0;

    const float alpha[4] = {0.0f, 0.0f, 0.0f, enable ? c.alphaReference : 0.0f};
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    c.m_pDeviceContext->Map(c.m_alphaTestBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    std::memcpy(mapped.pData, alpha, sizeof(alpha));
    c.m_pDeviceContext->Unmap(c.m_alphaTestBuffer, 0);
}

void Renderer::StateSetDepthSlopeAndBias(float slope, float bias)
{
    Context &c = this->getContext();

    const float scale = 65536.0f;
    c.rasterizerDesc.DepthBias = static_cast<int>(bias * scale);
    c.rasterizerDesc.SlopeScaledDepthBias = slope * scale;
    c.m_pDeviceContext->RSSetState(this->GetManagedRasterizerState());
}

void Renderer::UpdateFogState()
{
    Context &c = this->getContext();
    ID3D11DeviceContext *d3d11 = c.m_pDeviceContext;

    float fogParams[4] = {};
    if (c.fogEnabled)
    {
        if (c.fogMode == 1)
        {
            fogParams[0] = c.fogFarDistance;
            fogParams[1] = 1.0f / (c.fogFarDistance - c.fogNearDistance);
            fogParams[2] = 1.0f;
        }
        else
        {
            fogParams[0] = c.fogDensity;
            fogParams[2] = 2.0f;
        }
    }

    const float fogColour[4] = {c.fogColourRed, c.fogColourGreen, c.fogColourBlue, 1.0f};

    D3D11_MAPPED_SUBRESOURCE mapped = {};
    d3d11->Map(c.m_fogParamsBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    std::memcpy(mapped.pData, fogParams, sizeof(fogParams));
    d3d11->Unmap(c.m_fogParamsBuffer, 0);

    d3d11->Map(c.m_fogColourBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    std::memcpy(mapped.pData, fogColour, sizeof(fogColour));
    d3d11->Unmap(c.m_fogColourBuffer, 0);
}

void Renderer::StateSetVertexTextureUV(float u, float v)
{
    Context &c = this->getContext();
    const float texgen[4] = {u - 1.0f, v - 1.0f, 0.0f, 0.0f};

    D3D11_MAPPED_SUBRESOURCE mapped = {};
    c.m_pDeviceContext->Map(c.m_vertexTexcoordBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    std::memcpy(mapped.pData, texgen, sizeof(texgen));
    c.m_pDeviceContext->Unmap(c.m_vertexTexcoordBuffer, 0);
}

void Renderer::UpdateTexGenState()
{
    Context &c = this->getContext();

    D3D11_MAPPED_SUBRESOURCE mapped = {};
    c.m_pDeviceContext->Map(c.m_texGenMatricesBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    std::memcpy(mapped.pData, c.texGenMatrices, sizeof(c.texGenMatrices));
    c.m_pDeviceContext->Unmap(c.m_texGenMatricesBuffer, 0);
}

void Renderer::UpdateLightingState()
{
    Context &c = this->getContext();
    if (!c.lightingDirty || !c.lightingEnabled)
    {
        return;
    }

    if (!c.lightEnabled[0])
    {
        std::memset(&c.lightDirection[0], 0, sizeof(c.lightDirection[0]));
        std::memset(&c.lightColour[0], 0, sizeof(c.lightColour[0]));
    }

    if (!c.lightEnabled[1])
    {
        std::memset(&c.lightDirection[1], 0, sizeof(c.lightDirection[1]));
        std::memset(&c.lightColour[1], 0, sizeof(c.lightColour[1]));
    }

    const std::size_t lightingBytes = sizeof(c.lightDirection) + sizeof(c.lightColour) + sizeof(c.lightAmbientColour);
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    c.m_pDeviceContext->Map(c.m_lightingStateBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    std::memcpy(mapped.pData, c.lightDirection, lightingBytes);
    c.m_pDeviceContext->Unmap(c.m_lightingStateBuffer, 0);

    c.lightingDirty = 0;
}

void Renderer::StateSetLightDirection(int light, float x, float y, float z)
{
    if (light >= 2)
    {
        return;
    }

    Context &c = this->getContext();
    if (c.commandBuffer != nullptr && c.commandBuffer->isActive != 0)
    {
        c.commandBuffer->SetLightDirection(light, x, y, z);
        return;
    }

    const std::uint32_t stackIndex = c.stackPos[MATRIX_MODE_MODELVIEW];
    const DirectX::XMMATRIX &modelView = c.matrixStacks[MATRIX_MODE_MODELVIEW][stackIndex];
    const DirectX::XMVECTOR direction = DirectX::XMVectorSet(x, y, z, 0.0f);
    const DirectX::XMVECTOR transformed = DirectX::XMVector3TransformNormal(direction, modelView);
    const DirectX::XMVECTOR normalized = DirectX::XMVector3Normalize(transformed);

    DirectX::XMStoreFloat4(&c.lightDirection[light], normalized);
    c.lightingDirty = 1;
}

void Renderer::StateSetViewport(C4JRender::eViewportType viewportType)
{
    this->getContext();
    m_ViewportType = viewportType;

    const float fullWidth = static_cast<float>(backBufferWidth);
    const float fullHeight = static_cast<float>(backBufferHeight);

    float x = 0.0f;
    float y = 0.0f;
    float width = fullWidth;
    float height = fullHeight;

    switch (viewportType)
    {
    case C4JRender::VIEWPORT_TYPE_FULLSCREEN:
        break;
    case C4JRender::VIEWPORT_TYPE_SPLIT_TOP:
        y = fullHeight * 0.5f;
        height = fullHeight * 0.5f;
        break;
    case C4JRender::VIEWPORT_TYPE_SPLIT_BOTTOM:
        height = fullHeight * 0.5f;
        break;
    case C4JRender::VIEWPORT_TYPE_SPLIT_LEFT:
        width = fullWidth * 0.5f;
        break;
    case C4JRender::VIEWPORT_TYPE_SPLIT_RIGHT:
        x = fullWidth * 0.5f;
        width = fullWidth * 0.5f;
        break;
    case C4JRender::VIEWPORT_TYPE_QUADRANT_TOP_LEFT:
        width = fullWidth * 0.5f;
        height = fullHeight * 0.5f;
        break;
    case C4JRender::VIEWPORT_TYPE_QUADRANT_TOP_RIGHT:
        x = fullWidth * 0.5f;
        width = fullWidth * 0.5f;
        height = fullHeight * 0.5f;
        break;
    case C4JRender::VIEWPORT_TYPE_QUADRANT_BOTTOM_LEFT:
        y = fullHeight * 0.5f;
        width = fullWidth * 0.5f;
        height = fullHeight * 0.5f;
        break;
    case C4JRender::VIEWPORT_TYPE_QUADRANT_BOTTOM_RIGHT:
        x = fullWidth * 0.5f;
        y = fullHeight * 0.5f;
        width = fullWidth * 0.5f;
        height = fullHeight * 0.5f;
        break;
    default:
        break;
    }

    D3D11_VIEWPORT viewport = {};
    viewport.TopLeftX = x;
    viewport.TopLeftY = y;
    viewport.Width = width;
    viewport.Height = height;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    m_pDeviceContext->RSSetViewports(1, &viewport);
    m_pDeviceContext->OMSetRenderTargets(1, &renderTargetView, depthStencilView);
}

void Renderer::StateSetEnableViewportClipPlanes(bool) {}

void Renderer::StateSetTexGenCol(int col, float x, float y, float z, float w, bool eyeSpace)
{
    Context &c = this->getContext();

    DirectX::XMVECTOR plane = DirectX::XMVectorSet(x, y, z, w);
    if (eyeSpace)
    {
        DirectX::XMFLOAT4X4 modelView;
        std::memset(&modelView, 0, sizeof(modelView));
        std::memcpy(&modelView, this->MatrixGet(MATRIX_MODE_MODELVIEW), sizeof(modelView));

        DirectX::XMVECTOR determinant = DirectX::XMVectorZero();
        const DirectX::XMMATRIX inverse = DirectX::XMMatrixInverse(&determinant, DirectX::XMLoadFloat4x4(&modelView));
        plane = DirectX::XMVector4Transform(plane, inverse);
    }

    DirectX::XMFLOAT4 transformed;
    DirectX::XMStoreFloat4(&transformed, plane);

    const int activeSet = eyeSpace ? 0 : 1;
    const int inactiveSet = eyeSpace ? 1 : 0;

    float *active = reinterpret_cast<float *>(&c.texGenMatrices[activeSet]);
    active[col + 0] = transformed.x;
    active[col + 4] = transformed.y;
    active[col + 8] = transformed.z;
    active[col + 12] = transformed.w;

    float *inactive = reinterpret_cast<float *>(&c.texGenMatrices[inactiveSet]);
    inactive[col + 0] = 0.0f;
    inactive[col + 4] = 0.0f;
    inactive[col + 8] = 0.0f;
    inactive[col + 12] = 0.0f;
}

void Renderer::StateSetStencil(D3D11_COMPARISON_FUNC function, uint8_t stencil_ref, uint8_t stencil_func_mask, uint8_t stencil_write_mask)
{
    Context &c = this->getContext();

    D3D11_DEPTH_STENCIL_DESC desc = c.depthStencilDesc;
    desc.StencilEnable = TRUE;
    desc.StencilReadMask = stencil_func_mask;
    desc.StencilWriteMask = stencil_write_mask;
    desc.FrontFace.StencilFunc = function;
    desc.BackFace.StencilFunc = function;

    ID3D11DepthStencilState *state = nullptr;
    m_pDevice->CreateDepthStencilState(&desc, &state);
    m_pDeviceContext->OMSetDepthStencilState(state, stencil_ref);
    if (state != nullptr)
    {
        state->Release();
    }
}

void Renderer::StateSetForceLOD(int LOD)
{
    Context &c = this->getContext();
    c.forcedLOD = LOD;
}

void Renderer::StateUpdate()
{
    Context &c = this->getContext();
    this->StateSetFaceCull(c.faceCullEnabled != 0);
    this->StateSetDepthMask(c.depthWriteEnabled != 0);
    this->StateSetDepthTestEnable(c.depthTestEnabled != 0);
    this->StateSetAlphaTestEnable(c.alphaTestEnabled != 0);
}
