#pragma once
#include "Renderer.h"

#include <cstdint>
#include <cstdio>
#include <cstring>

D3D11_PRIMITIVE_TOPOLOGY *Renderer::m_Topologies = nullptr;

void Renderer::DrawVertexBuffer(C4JRender::ePrimitiveType PrimitiveType, int count, ID3D11Buffer *buffer, C4JRender::eVertexType vType,
                                C4JRender::ePixelShaderType psType)
{
    Renderer::Context &c = this->getContext();
    ID3D11DeviceContext *d3d11 = c.m_pDeviceContext;

    int drawCount = count;
    bool indexed = false;
    this->DrawVertexSetup(vType, psType, PrimitiveType, &drawCount, &indexed);
    this->StateUpdate();

    const UINT stride = vertexStrideTable[vType];
    const UINT offset = 0;
    d3d11->IASetVertexBuffers(0, 1, &buffer, &stride, &offset);

    if (indexed)
    {
        d3d11->DrawIndexed(drawCount, 0, 0);
    }
    else
    {
        d3d11->Draw(count, 0);
    }
}

void Renderer::DrawVertexSetup(C4JRender::eVertexType vType, C4JRender::ePixelShaderType psType, C4JRender::ePrimitiveType PrimitiveType, int *count,
                               bool *indexed)
{
    Renderer::Context &c = this->getContext();
    ID3D11DeviceContext *d3d11 = c.m_pDeviceContext;

    C4JRender::eVertexType effectiveVertexType = vType;
    if (effectiveVertexType == C4JRender::VERTEX_TYPE_PF3_TF2_CB4_NB4_XW1 && c.lightingEnabled)
    {
        effectiveVertexType = C4JRender::VERTEX_TYPE_PF3_TF2_CB4_NB4_XW1_LIT;
    }

    if (effectiveVertexType != activeVertexType)
    {
        d3d11->VSSetShader(vertexShaderTable[effectiveVertexType], nullptr, 0);
        d3d11->IASetInputLayout(inputLayoutTable[effectiveVertexType]);
        activeVertexType = effectiveVertexType;
    }

    if (psType != activePixelType)
    {
        d3d11->PSSetShader(pixelShaderTable[psType], nullptr, 0);
        activePixelType = psType;
    }

    D3D11_MAPPED_SUBRESOURCE mapped = {};

    if (c.matrixDirty[MATRIX_MODE_MODELVIEW])
    {
        d3d11->Map(c.m_modelViewMatrix, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        std::memcpy(mapped.pData, this->MatrixGet(MATRIX_MODE_MODELVIEW), sizeof(DirectX::XMMATRIX));
        d3d11->Unmap(c.m_modelViewMatrix, 0);
        c.matrixDirty[MATRIX_MODE_MODELVIEW] = false;
    }

    if (c.matrixDirty[MATRIX_MODE_MODELVIEW_PROJECTION])
    {
        d3d11->Map(c.m_projectionMatrix, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        std::memcpy(mapped.pData, this->MatrixGet(MATRIX_MODE_MODELVIEW_PROJECTION), sizeof(DirectX::XMMATRIX));
        d3d11->Unmap(c.m_projectionMatrix, 0);
        c.matrixDirty[MATRIX_MODE_MODELVIEW_PROJECTION] = false;
    }

    if (c.matrixDirty[MATRIX_MODE_MODELVIEW_TEXTURE])
    {
        d3d11->Map(c.m_textureMatrix, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        std::memcpy(mapped.pData, this->MatrixGet(MATRIX_MODE_MODELVIEW_TEXTURE), sizeof(DirectX::XMMATRIX));
        d3d11->Unmap(c.m_textureMatrix, 0);
        c.matrixDirty[MATRIX_MODE_MODELVIEW_TEXTURE] = false;
    }

    this->UpdateFogState();
    this->UpdateViewportState();
    this->UpdateLightingState();
    this->UpdateTexGenState();

    d3d11->IASetPrimitiveTopology(m_Topologies[PrimitiveType]);

    if (PrimitiveType == C4JRender::PRIMITIVE_TYPE_QUAD_LIST)
    {
        d3d11->IASetIndexBuffer(quadIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
        *count = (*count * 6) / 4;
        *indexed = true;
        return;
    }

    if (PrimitiveType == C4JRender::PRIMITIVE_TYPE_TRIANGLE_FAN)
    {
        d3d11->IASetIndexBuffer(fanIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
        *count = (*count - 2) * 3;
        *indexed = true;
        return;
    }

    d3d11->IASetIndexBuffer(nullptr, DXGI_FORMAT_R16_UINT, 0);
    *indexed = false;
}

void Renderer::DrawVertices(C4JRender::ePrimitiveType PrimitiveType, int count, void *vertices, C4JRender::eVertexType vType,
                            C4JRender::ePixelShaderType psType)
{
    Renderer::Context &c = this->getContext();
    ID3D11DeviceContext *d3d11 = c.m_pDeviceContext;
    Renderer::CommandBuffer *commandBuffer = c.commandBuffer;

    if (commandBuffer != nullptr)
    {
        C4JRender::eVertexType effectiveVertexType = vType;
        if (effectiveVertexType == C4JRender::VERTEX_TYPE_PF3_TF2_CB4_NB4_XW1 && c.lightingEnabled)
        {
            effectiveVertexType = C4JRender::VERTEX_TYPE_PF3_TF2_CB4_NB4_XW1_LIT;
        }

        c.recordingPrimitiveType = PrimitiveType;
        c.recordingVertexType = effectiveVertexType;
        const UINT stride = vertexStrideTable[effectiveVertexType];
        commandBuffer->AddVertices(stride, static_cast<UINT>(count), vertices, c);
        return;
    }

    int drawCount = count;
    bool indexed = false;
    this->DrawVertexSetup(vType, psType, PrimitiveType, &drawCount, &indexed);

    const UINT stride = vertexStrideTable[vType];
    const UINT vertexBytes = stride * static_cast<UINT>(count);

    assert(vertexBytes <= Context::VERTEX_BUFFER_SIZE);

    if (c.dynamicVertexOffset + vertexBytes > Context::VERTEX_BUFFER_SIZE)
    {
        c.dynamicVertexOffset = 0;
    }

    D3D11_MAPPED_SUBRESOURCE mapped = {};
    const D3D11_MAP mapType = c.dynamicVertexOffset == 0 ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE_NO_OVERWRITE;
    const HRESULT hr = d3d11->Map(c.dynamicVertexBuffer, 0, mapType, 0, &mapped);
    if (hr != 0)
    {
        std::printf("ERROR: 0x%x\n", static_cast<unsigned int>(hr));
    }

    std::memcpy(static_cast<std::uint8_t *>(mapped.pData) + c.dynamicVertexOffset, vertices, vertexBytes);
    d3d11->Unmap(c.dynamicVertexBuffer, 0);

    this->StateUpdate();

    ID3D11Buffer *dynamicBuffer = c.dynamicVertexBuffer;
    const UINT vertexOffset = c.dynamicVertexOffset;
    d3d11->IASetVertexBuffers(0, 1, &dynamicBuffer, &stride, &vertexOffset);

    if (indexed)
    {
        d3d11->DrawIndexed(drawCount, 0, 0);
    }
    else
    {
        d3d11->Draw(count, 0);
    }

    c.dynamicVertexOffset += vertexBytes;
}
