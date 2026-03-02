#pragma once
#include "Renderer.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <new>

Renderer::CommandBuffer::CommandBuffer(bool full)
    : m_vertexBuffer(nullptr), m_vertexData(nullptr), m_vertexDataLength(0), m_commands(), m_allocated(0x1000), isActive(full ? 1 : 0)
{
    m_vertexData = std::malloc(m_allocated);
    EnterCriticalSection(&Renderer::totalAllocCS);
    Renderer::totalAlloc += static_cast<int>(m_allocated);
    LeaveCriticalSection(&Renderer::totalAllocCS);
}

Renderer::CommandBuffer::~CommandBuffer()
{
    if (m_vertexBuffer)
    {
        m_vertexBuffer->Release();
    }

    std::free(m_vertexData);

    EnterCriticalSection(&Renderer::totalAllocCS);
    Renderer::totalAlloc -= static_cast<int>(m_allocated);
    LeaveCriticalSection(&Renderer::totalAllocCS);
}

void Renderer::CommandBuffer::StartRecording() {}

void Renderer::CommandBuffer::EndRecording(ID3D11Device *device)
{
    if (m_vertexDataLength != 0)
    {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = m_vertexDataLength;
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA data = {};
        data.pSysMem = m_vertexData;
        device->CreateBuffer(&desc, &data, &m_vertexBuffer);
    }

    std::free(m_vertexData);
    m_vertexData = nullptr;
}

std::uint64_t Renderer::CommandBuffer::GetAllocated()
{
    return m_allocated;
}

bool Renderer::CommandBuffer::IsBusy()
{
    return false;
}

void Renderer::CommandBuffer::AddMatrix(const float *matrix)
{
    Command command = {};
    command.m_command_type = COMMAND_ADD_MATRIX;
    std::memcpy(command.add_matrix.m_matrix, matrix, sizeof(command.add_matrix.m_matrix));
    m_commands.push_back(command);
}

void Renderer::CommandBuffer::AddVertices(unsigned int stride, unsigned int count, void *dataIn, Renderer::Context &c)
{
    if (c.matrixDirty[MATRIX_MODE_MODELVIEW_CBUFF])
    {
        this->AddMatrix(InternalRenderManager.MatrixGet(MATRIX_MODE_MODELVIEW_CBUFF));
        c.matrixDirty[MATRIX_MODE_MODELVIEW_CBUFF] = false;
    }

    const std::uint64_t vertexOffset = m_vertexDataLength;
    const std::uint64_t copySize = std::uint64_t(stride) * std::uint64_t(count);

    Command command = {};
    command.m_command_type = COMMAND_ADD_VERTICES;
    command.add_vertices.m_vertex_index_start = (unsigned int)vertexOffset;
    command.add_vertices.m_vertex_count = count;

    m_vertexDataLength = vertexOffset + copySize;
    if (m_vertexDataLength > m_allocated)
    {
        EnterCriticalSection(&Renderer::totalAllocCS);
        Renderer::totalAlloc -= static_cast<int>(m_allocated);
        LeaveCriticalSection(&Renderer::totalAllocCS);

        m_allocated = ((m_vertexDataLength + (0x1000u - 1u)) & ~(0x1000u - 1u));
        m_vertexData = std::realloc(m_vertexData, m_allocated);

        EnterCriticalSection(&Renderer::totalAllocCS);
        Renderer::totalAlloc += static_cast<int>(m_allocated);
        LeaveCriticalSection(&Renderer::totalAllocCS);
    }

    const std::size_t byteCount = std::size_t(stride) * std::size_t(count);
    std::memcpy(static_cast<std::uint8_t *>(m_vertexData) + vertexOffset, dataIn, byteCount);
    m_commands.push_back(command);
}

void Renderer::CommandBuffer::BindTexture(int idx)
{
    Command command = {};
    command.m_command_type = COMMAND_BIND_TEXTURE;
    command.bind_texture.m_texture_index = idx;
    m_commands.push_back(command);
}

void Renderer::CommandBuffer::SetColor(float r, float g, float b, float a)
{
    Command command = {};
    command.m_command_type = COMMAND_SET_COLOR;
    command.set_color.m_color[0] = r;
    command.set_color.m_color[1] = g;
    command.set_color.m_color[2] = b;
    command.set_color.m_color[3] = a;
    m_commands.push_back(command);
}

void Renderer::CommandBuffer::SetDepthFunc(int func)
{
    Command command = {};
    command.m_command_type = COMMAND_SET_DEPTH_FUNC;
    command.set_depth_func.m_depth_func = func;
    m_commands.push_back(command);
}

void Renderer::CommandBuffer::SetDepthMask(bool enable)
{
    Command command = {};
    command.m_command_type = COMMAND_SET_DEPTH_MASK;
    command.set_depth_mask.m_enable = enable;
    m_commands.push_back(command);
}

void Renderer::CommandBuffer::SetDepthTestEnable(bool enable)
{
    Command command = {};
    command.m_command_type = COMMAND_SET_DEPTH_TEST;
    command.set_depth_test.m_enable = enable;
    m_commands.push_back(command);
}

void Renderer::CommandBuffer::SetLightingEnable(bool enable)
{
    Command command = {};
    command.m_command_type = COMMAND_SET_LIGHTING_ENABLE;
    command.set_lighting_enable.m_enable = enable;
    m_commands.push_back(command);
}

void Renderer::CommandBuffer::SetLightEnable(int light, bool enable)
{
    Command command = {};
    command.m_command_type = COMMAND_SET_LIGHT_ENABLE;
    command.set_light_enable.m_light_index = light;
    command.set_light_enable.m_enable = enable;
    m_commands.push_back(command);
}

void Renderer::CommandBuffer::SetLightDirection(int light, float x, float y, float z)
{
    Renderer::Context &c = InternalRenderManager.getContext();
    const std::uint32_t depth = c.stackPos[MATRIX_MODE_MODELVIEW_CBUFF];
    const DirectX::XMMATRIX &matrix = c.matrixStacks[MATRIX_MODE_MODELVIEW_CBUFF][depth];

    DirectX::XMVECTOR direction = DirectX::XMVectorSet(x, y, z, 0.0f);
    direction = DirectX::XMVector3TransformNormal(direction, matrix);
    direction = DirectX::XMVector3Normalize(direction);

    Command command = {};
    command.m_command_type = COMMAND_SET_LIGHT_DIRECTION;
    command.set_light_direction.m_light_index = light;
    DirectX::XMFLOAT4 outDirection;
    DirectX::XMStoreFloat4(&outDirection, direction);
    command.set_light_direction.m_direction[0] = outDirection.x;
    command.set_light_direction.m_direction[1] = outDirection.y;
    command.set_light_direction.m_direction[2] = outDirection.z;
    command.set_light_direction.m_direction[3] = outDirection.w;
    m_commands.push_back(command);
}

void Renderer::CommandBuffer::SetLightColour(int light, float r, float g, float b)
{
    Command command = {};
    command.m_command_type = COMMAND_SET_LIGHT_COLOUR;
    command.set_light_colour.m_light_index = light;
    command.set_light_colour.m_color[0] = r;
    command.set_light_colour.m_color[1] = g;
    command.set_light_colour.m_color[2] = b;
    m_commands.push_back(command);
}

void Renderer::CommandBuffer::SetLightAmbientColour(float r, float g, float b)
{
    Command command = {};
    command.m_command_type = COMMAND_SET_LIGHT_AMBIENT_COLOUR;
    command.set_light_ambient_colour.m_color[0] = r;
    command.set_light_ambient_colour.m_color[1] = g;
    command.set_light_ambient_colour.m_color[2] = b;
    m_commands.push_back(command);
}

void Renderer::CommandBuffer::SetBlendEnable(bool enable)
{
    Command command = {};
    command.m_command_type = COMMAND_SET_BLEND_ENABLE;
    command.set_blend_enable.m_enable = enable;
    m_commands.push_back(command);
}

void Renderer::CommandBuffer::SetBlendFunc(int src, int dst)
{
    Command command = {};
    command.m_command_type = COMMAND_SET_BLEND_FUNC;
    command.set_blend_func.m_src = src;
    command.set_blend_func.m_dst = dst;
    m_commands.push_back(command);
}

void Renderer::CommandBuffer::SetBlendFactor(unsigned int factor)
{
    Command command = {};
    command.m_command_type = COMMAND_SET_BLEND_FACTOR;
    command.set_blend_factor.m_blend_factor = factor;
    m_commands.push_back(command);
}

void Renderer::CommandBuffer::SetFaceCull(bool enable)
{
    Command command = {};
    command.m_command_type = COMMAND_SET_FACE_CULL;
    command.set_face_cull.m_enable = enable;
    m_commands.push_back(command);
}

void Renderer::CommandBuffer::Render(C4JRender::eVertexType vType, Renderer::Context &c, int primitiveType)
{
    if (!m_vertexBuffer)
    {
        return;
    }

    int drawVertexType = vType;
    int shaderVertexType = drawVertexType;
    bool matrixOverride = false;

    for (std::vector<Command>::const_iterator it = m_commands.begin(); it != m_commands.end(); ++it)
    {
        const Command &command = *it;
        switch (command.m_command_type)
        {
        case COMMAND_ADD_MATRIX:
        {
            if (drawVertexType == C4JRender::VERTEX_TYPE_COMPRESSED)
            {
                const float row[4] = {command.add_matrix.m_matrix[12], command.add_matrix.m_matrix[13], command.add_matrix.m_matrix[14],
                                      command.add_matrix.m_matrix[15]};
                D3D11_MAPPED_SUBRESOURCE mappedAux0 = {};
                c.m_pDeviceContext->Map(c.m_compressedTranslationBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedAux0);
                std::memcpy(mappedAux0.pData, row, sizeof(row));
                c.m_pDeviceContext->Unmap(c.m_compressedTranslationBuffer, 0);
            }
            else
            {
                D3D11_MAPPED_SUBRESOURCE mappedMatrix1 = {};
                c.m_pDeviceContext->Map(c.m_localTransformMatrix, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedMatrix1);
                std::memcpy(mappedMatrix1.pData, command.add_matrix.m_matrix, sizeof(command.add_matrix.m_matrix));
                c.m_pDeviceContext->Unmap(c.m_localTransformMatrix, 0);
                matrixOverride = true;
            }
            break;
        }
        case COMMAND_ADD_VERTICES:
        {
            if (isActive)
            {
                InternalRenderManager.UpdateLightingState();

                if (drawVertexType == C4JRender::VERTEX_TYPE_PF3_TF2_CB4_NB4_XW1)
                {
                    if (c.lightingEnabled)
                    {
                        drawVertexType = C4JRender::VERTEX_TYPE_PF3_TF2_CB4_NB4_XW1_LIT;
                        shaderVertexType = C4JRender::VERTEX_TYPE_PF3_TF2_CB4_NB4_XW1_LIT;
                    }
                }
                else if (drawVertexType == C4JRender::VERTEX_TYPE_PF3_TF2_CB4_NB4_XW1_LIT && !c.lightingEnabled)
                {
                    drawVertexType = C4JRender::VERTEX_TYPE_PF3_TF2_CB4_NB4_XW1;
                    shaderVertexType = C4JRender::VERTEX_TYPE_PF3_TF2_CB4_NB4_XW1;
                }

                if (static_cast<DWORD>(drawVertexType) != InternalRenderManager.activeVertexType)
                {
                    c.m_pDeviceContext->VSSetShader(InternalRenderManager.vertexShaderTable[shaderVertexType], nullptr, 0);
                    c.m_pDeviceContext->IASetInputLayout(InternalRenderManager.inputLayoutTable[shaderVertexType]);
                    InternalRenderManager.activeVertexType = drawVertexType;
                }
            }

            unsigned int drawCount = command.add_vertices.m_vertex_count;
            bool drawIndexed = false;
            if (primitiveType == C4JRender::PRIMITIVE_TYPE_QUAD_LIST)
            {
                drawCount = (drawCount * 6u) / 4u;
                drawIndexed = true;
            }
            else if (primitiveType == C4JRender::PRIMITIVE_TYPE_TRIANGLE_FAN)
            {
                drawCount = (drawCount - 2u) * 3u;
                drawIndexed = true;
            }

            ID3D11Buffer *buffer = m_vertexBuffer;
            const UINT stride = InternalRenderManager.vertexStrideTable[drawVertexType];
            const UINT offset = command.add_vertices.m_vertex_index_start;
            c.m_pDeviceContext->IASetVertexBuffers(0, 1, &buffer, &stride, &offset);

            if (drawIndexed)
            {
                c.m_pDeviceContext->DrawIndexed(drawCount, 0, 0);
            }
            else
            {
                c.m_pDeviceContext->Draw(drawCount, 0);
            }
            break;
        }
        case COMMAND_BIND_TEXTURE:
        {
            c.textureIdx = command.bind_texture.m_texture_index;
            ID3D11ShaderResourceView *view = InternalRenderManager.m_textures[c.textureIdx].view;
            c.m_pDeviceContext->PSSetShaderResources(0, 1, &view);
            InternalRenderManager.UpdateTextureState(false);
            break;
        }
        case COMMAND_SET_COLOR:
        {
            D3D11_MAPPED_SUBRESOURCE mappedColour = {};
            c.m_pDeviceContext->Map(c.m_tintColorBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedColour);
            std::memcpy(mappedColour.pData, command.set_color.m_color, sizeof(command.set_color.m_color));
            c.m_pDeviceContext->Unmap(c.m_tintColorBuffer, 0);
            break;
        }
        case COMMAND_SET_DEPTH_FUNC:
        {
            c.depthStencilDesc.DepthFunc = static_cast<D3D11_COMPARISON_FUNC>(command.set_depth_func.m_depth_func);
            c.m_pDeviceContext->OMSetDepthStencilState(InternalRenderManager.GetManagedDepthStencilState(), 0);
            break;
        }
        case COMMAND_SET_DEPTH_MASK:
        {
            c.depthWriteEnabled = command.set_depth_mask.m_enable ? 1 : 0;
            c.depthStencilDesc.DepthWriteMask = command.set_depth_mask.m_enable ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
            c.m_pDeviceContext->OMSetDepthStencilState(InternalRenderManager.GetManagedDepthStencilState(), 0);
            break;
        }
        case COMMAND_SET_DEPTH_TEST:
        {
            c.depthTestEnabled = command.set_depth_test.m_enable ? 1 : 0;
            c.depthStencilDesc.DepthEnable = command.set_depth_test.m_enable ? TRUE : FALSE;
            c.m_pDeviceContext->OMSetDepthStencilState(InternalRenderManager.GetManagedDepthStencilState(), 0);
            break;
        }
        case COMMAND_SET_LIGHTING_ENABLE:
        {
            c.lightingEnabled = command.set_lighting_enable.m_enable ? 1 : 0;
            break;
        }
        case COMMAND_SET_LIGHT_ENABLE:
        {
            const int light = command.set_light_enable.m_light_index;
            if (light >= 0 && light < 2)
            {
                c.lightEnabled[light] = command.set_light_enable.m_enable ? 1 : 0;
                c.lightingDirty = 1;
            }
            break;
        }
        case COMMAND_SET_LIGHT_DIRECTION:
        {
            const int light = command.set_light_direction.m_light_index;
            if (light >= 0 && light < 2)
            {
                c.lightDirection[light].x = command.set_light_direction.m_direction[0];
                c.lightDirection[light].y = command.set_light_direction.m_direction[1];
                c.lightDirection[light].z = command.set_light_direction.m_direction[2];
                c.lightDirection[light].w = command.set_light_direction.m_direction[3];
                c.lightingDirty = 1;
            }
            break;
        }
        case COMMAND_SET_LIGHT_COLOUR:
        {
            const int light = command.set_light_colour.m_light_index;
            if (light >= 0 && light < 2)
            {
                c.lightColour[light].x = command.set_light_colour.m_color[0];
                c.lightColour[light].y = command.set_light_colour.m_color[1];
                c.lightColour[light].z = command.set_light_colour.m_color[2];
                c.lightColour[light].w = 1.0f;
                c.lightingDirty = 1;
            }
            break;
        }
        case COMMAND_SET_LIGHT_AMBIENT_COLOUR:
        {
            c.lightAmbientColour.x = command.set_light_ambient_colour.m_color[0];
            c.lightAmbientColour.y = command.set_light_ambient_colour.m_color[1];
            c.lightAmbientColour.z = command.set_light_ambient_colour.m_color[2];
            c.lightAmbientColour.w = 1.0f;
            c.lightingDirty = 1;
            break;
        }
        case COMMAND_SET_BLEND_ENABLE:
        {
            c.blendDesc.RenderTarget[0].BlendEnable = command.set_blend_enable.m_enable ? TRUE : FALSE;
            break;
        }
        case COMMAND_SET_BLEND_FUNC:
        {
            c.blendDesc.RenderTarget[0].SrcBlend = static_cast<D3D11_BLEND>(command.set_blend_func.m_src);
            c.blendDesc.RenderTarget[0].DestBlend = static_cast<D3D11_BLEND>(command.set_blend_func.m_dst);
            c.m_pDeviceContext->OMSetBlendState(InternalRenderManager.GetManagedBlendState(), c.blendFactor, 0xFFFFFFFFu);
            break;
        }
        case COMMAND_SET_BLEND_FACTOR:
        {
            const unsigned int factor = command.set_blend_factor.m_blend_factor;
            c.blendFactor[0] = float((factor >> 0) & 0xFFu) / 255.0f;
            c.blendFactor[1] = float((factor >> 8) & 0xFFu) / 255.0f;
            c.blendFactor[2] = float((factor >> 16) & 0xFFu) / 255.0f;
            c.blendFactor[3] = float((factor >> 24) & 0xFFu) / 255.0f;
            c.m_pDeviceContext->OMSetBlendState(InternalRenderManager.GetManagedBlendState(), c.blendFactor, 0xFFFFFFFFu);
            break;
        }
        case COMMAND_SET_FACE_CULL:
        {
            c.rasterizerDesc.CullMode = command.set_face_cull.m_enable ? D3D11_CULL_BACK : D3D11_CULL_NONE;
            c.m_pDeviceContext->RSSetState(InternalRenderManager.GetManagedRasterizerState());
            c.faceCullEnabled = command.set_face_cull.m_enable ? 1 : 0;
            break;
        }
        default:
            break;
        }
    }

    if (matrixOverride)
    {
        const DirectX::XMMATRIX identity = DirectX::XMMatrixIdentity();
        D3D11_MAPPED_SUBRESOURCE mappedIdentity = {};
        c.m_pDeviceContext->Map(c.m_localTransformMatrix, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedIdentity);
        std::memcpy(mappedIdentity.pData, &identity, sizeof(identity));
        c.m_pDeviceContext->Unmap(c.m_localTransformMatrix, 0);
    }
}

bool Renderer::CBuffCall(int index, bool full)
{
    EnterCriticalSection(&rtl_critical_section100);

    bool result = false;
    const int commandIndex = m_commandHandleToIndex[index];
    if (commandIndex >= 0)
    {
        Renderer::Context &c = this->getContext();
        const std::uint8_t vertexType = m_commandVertexTypes[commandIndex];
        const std::uint8_t primitiveType = m_commandPrimitiveTypes[commandIndex];

        if (full)
        {
            if (c.matrixDirty[MATRIX_MODE_MODELVIEW])
            {
                D3D11_MAPPED_SUBRESOURCE mappedMatrix0 = {};
                c.m_pDeviceContext->Map(c.m_modelViewMatrix, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedMatrix0);
                std::memcpy(mappedMatrix0.pData, this->MatrixGet(MATRIX_MODE_MODELVIEW), sizeof(DirectX::XMMATRIX));
                c.m_pDeviceContext->Unmap(c.m_modelViewMatrix, 0);
                c.matrixDirty[MATRIX_MODE_MODELVIEW] = false;
            }

            if (c.matrixDirty[MATRIX_MODE_MODELVIEW_PROJECTION])
            {
                D3D11_MAPPED_SUBRESOURCE mappedMatrix2 = {};
                c.m_pDeviceContext->Map(c.m_projectionMatrix, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedMatrix2);
                std::memcpy(mappedMatrix2.pData, this->MatrixGet(MATRIX_MODE_MODELVIEW_PROJECTION), sizeof(DirectX::XMMATRIX));
                c.m_pDeviceContext->Unmap(c.m_projectionMatrix, 0);
                c.matrixDirty[MATRIX_MODE_MODELVIEW_PROJECTION] = false;
            }

            if (c.matrixDirty[MATRIX_MODE_MODELVIEW_TEXTURE])
            {
                D3D11_MAPPED_SUBRESOURCE mappedMatrix3 = {};
                c.m_pDeviceContext->Map(c.m_textureMatrix, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedMatrix3);
                std::memcpy(mappedMatrix3.pData, this->MatrixGet(MATRIX_MODE_MODELVIEW_TEXTURE), sizeof(DirectX::XMMATRIX));
                c.m_pDeviceContext->Unmap(c.m_textureMatrix, 0);
                c.matrixDirty[MATRIX_MODE_MODELVIEW_TEXTURE] = false;
            }

            this->UpdateFogState();
            this->UpdateLightingState();
            this->UpdateViewportState();
            this->UpdateTexGenState();

            if (vertexType != activeVertexType)
            {
                c.m_pDeviceContext->VSSetShader(vertexShaderTable[vertexType], nullptr, 0);
                c.m_pDeviceContext->IASetInputLayout(inputLayoutTable[vertexType]);
                activeVertexType = vertexType;
            }

            int pixelType = 0;
            if (static_cast<int>(c.forcedLOD) > -1)
            {
                const float forcedLod[4] = {static_cast<float>(static_cast<int>(c.forcedLOD)), 0.0f, 0.0f, 0.0f};
                D3D11_MAPPED_SUBRESOURCE mappedAux4 = {};
                c.m_pDeviceContext->Map(c.m_forcedLODBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedAux4);
                std::memcpy(mappedAux4.pData, forcedLod, sizeof(forcedLod));
                c.m_pDeviceContext->Unmap(c.m_forcedLODBuffer, 0);
                pixelType = C4JRender::PIXEL_SHADER_TYPE_FORCELOD;
            }

            if (static_cast<DWORD>(pixelType) != activePixelType)
            {
                c.m_pDeviceContext->PSSetShader(pixelShaderTable[pixelType], nullptr, 0);
                activePixelType = pixelType;
            }

            c.m_pDeviceContext->IASetPrimitiveTopology(m_Topologies[primitiveType]);

            ID3D11Buffer *indexBuffer = nullptr;
            if (primitiveType == C4JRender::PRIMITIVE_TYPE_QUAD_LIST)
            {
                indexBuffer = quadIndexBuffer;
            }
            else if (primitiveType == C4JRender::PRIMITIVE_TYPE_TRIANGLE_FAN)
            {
                indexBuffer = fanIndexBuffer;
            }

            c.m_pDeviceContext->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R16_UINT, 0);
        }

        m_commandBuffers[commandIndex]->Render(static_cast<C4JRender::eVertexType>(vertexType), c, primitiveType);

        if (full)
        {
            this->MultWithStack(m_commandMatrices[commandIndex]);
            c.matrixStacks[MATRIX_MODE_MODELVIEW_CBUFF][0] = DirectX::XMMatrixIdentity();
            c.matrixDirty[MATRIX_MODE_MODELVIEW_CBUFF] = true;
        }

        result = true;
    }

    LeaveCriticalSection(&rtl_critical_section100);
    return result;
}

void Renderer::CBuffClear(int index)
{
    EnterCriticalSection(&rtl_critical_section100);

    std::int16_t *externalToInternal = static_cast<std::int16_t *>(m_commandHandleToIndex);
    const int internalIndex = externalToInternal[index];
    if (internalIndex >= 0)
    {
        this->DeleteInternalBuffer(internalIndex);
        externalToInternal[index] = static_cast<std::int16_t>(-2);
    }

    LeaveCriticalSection(&rtl_critical_section100);
}

int Renderer::CBuffCreate(int count)
{
    EnterCriticalSection(&rtl_critical_section100);

    int first = reservedRendererDword1;
    if (first < NUM_COMMAND_HANDLES)
    {
        int probe = first;
        int end = first + count;
        while (true)
        {
            assert(first < NUM_COMMAND_HANDLES);

            int cursor = probe;
            while (cursor < end && cursor < NUM_COMMAND_HANDLES && m_commandHandleToIndex[cursor] == static_cast<std::int16_t>(-1))
            {
                ++cursor;
            }

            if (cursor >= end)
            {
                break;
            }

            ++first;
            ++probe;
            ++end;
            if (first >= NUM_COMMAND_HANDLES || end > NUM_COMMAND_HANDLES)
            {
                first = -1;
                break;
            }
        }

        if (first >= 0)
        {
            const int allocationEnd = first + count;
            for (int i = first; i < allocationEnd; ++i)
            {
                m_commandHandleToIndex[i] = static_cast<std::int16_t>(-2);
            }

            if (reservedRendererByte1)
            {
                reservedRendererDword1 = allocationEnd;
            }
        }
    }
    else
    {
        first = -1;
    }

    LeaveCriticalSection(&rtl_critical_section100);
    return first;
}

void Renderer::CBuffDeferredModeEnd()
{
    Renderer::Context &c = this->getContext();
    if (!c.deferredModeEnabled)
    {
        return;
    }

    EnterCriticalSection(&rtl_critical_section100);
    c.deferredModeEnabled = 0;

    for (std::vector<Renderer::DeferredCBuff>::const_iterator it = c.deferredBuffers.begin(); it != c.deferredBuffers.end(); ++it)
    {
        const Renderer::DeferredCBuff &deferred = *it;
        const int existingIndex = m_commandHandleToIndex[deferred.m_vertex_index];
        if (existingIndex >= 0)
        {
            this->DeleteInternalBuffer(existingIndex);
        }

        if (static_cast<int>(reservedRendererDword2 + reservedRendererDword3 + 10u) > MAX_COMMAND_BUFFERS)
        {
            DebugBreak();
        }

        const int internalSlot = reservedRendererDword2;
        ++reservedRendererDword2;

        m_commandHandleToIndex[deferred.m_vertex_index] = static_cast<std::int16_t>(internalSlot);
        m_commandIndexToHandle[internalSlot] = deferred.m_vertex_index;
        m_commandVertexTypes[internalSlot] = static_cast<std::uint8_t>(deferred.m_vertex_type);
        m_commandPrimitiveTypes[internalSlot] = static_cast<std::uint8_t>(deferred.m_primitive_type);
        m_commandBuffers[internalSlot] = deferred.m_command_buf;
        m_commandMatrices[internalSlot] = deferred.m_matrix;
    }

    c.deferredBuffers.clear();
    LeaveCriticalSection(&rtl_critical_section100);
}

void Renderer::CBuffDeferredModeStart()
{
    this->getContext().deferredModeEnabled = 1;
}

void Renderer::CBuffDelete(int first, int count)
{
    EnterCriticalSection(&rtl_critical_section100);

    const int end = first + count;
    for (int i = first; i < end; ++i)
    {
        const int internalIndex = m_commandHandleToIndex[i];
        if (internalIndex >= 0)
        {
            this->DeleteInternalBuffer(internalIndex);
        }

        m_commandHandleToIndex[i] = static_cast<std::int16_t>(-1);
    }

    LeaveCriticalSection(&rtl_critical_section100);
}

void Renderer::CBuffEnd()
{
    Renderer::Context &c = this->getContext();

    assert(c.stackType == MATRIX_MODE_MODELVIEW_CBUFF);
    assert(c.stackPos[MATRIX_MODE_MODELVIEW_CBUFF] == 0);

    EnterCriticalSection(&rtl_critical_section100);

    if (c.deferredModeEnabled)
    {
        Renderer::DeferredCBuff deferred;
        deferred.m_command_buf = c.commandBuffer;
        deferred.m_vertex_index = c.recordingBufferIndex;
        deferred.m_vertex_type = c.recordingVertexType;
        deferred.m_primitive_type = c.recordingPrimitiveType;
        deferred.m_matrix = c.matrixStacks[MATRIX_MODE_MODELVIEW_CBUFF][0];
        c.deferredBuffers.push_back(deferred);
    }
    else
    {
        const int existingIndex = m_commandHandleToIndex[c.recordingBufferIndex];
        if (existingIndex >= 0)
        {
            this->DeleteInternalBuffer(existingIndex);
        }

        if (static_cast<int>(reservedRendererDword2 + reservedRendererDword3 + 10u) > MAX_COMMAND_BUFFERS)
        {
            DebugBreak();
        }

        const int internalSlot = reservedRendererDword2;
        ++reservedRendererDword2;

        m_commandHandleToIndex[c.recordingBufferIndex] = static_cast<std::int16_t>(internalSlot);
        m_commandIndexToHandle[internalSlot] = c.recordingBufferIndex;
        m_commandVertexTypes[internalSlot] = static_cast<std::uint8_t>(c.recordingVertexType);
        m_commandPrimitiveTypes[internalSlot] = static_cast<std::uint8_t>(c.recordingPrimitiveType);
        m_commandBuffers[internalSlot] = c.commandBuffer;
        m_commandMatrices[internalSlot] = c.matrixStacks[MATRIX_MODE_MODELVIEW_CBUFF][0];
    }

    c.stackType = MATRIX_MODE_MODELVIEW;
    c.commandBuffer->EndRecording(m_pDevice);
    c.commandBuffer = nullptr;

    LeaveCriticalSection(&rtl_critical_section100);
}

void Renderer::CBuffLockStaticCreations()
{
    reservedRendererByte1 = 0;
}

int Renderer::CBuffSize(int index)
{
    if (index == -1)
    {
        return totalAlloc < 0 ? 0 : totalAlloc;
    }

    unsigned int size = 0;
    EnterCriticalSection(&rtl_critical_section100);
    const int commandIndex = m_commandHandleToIndex[index];
    if (commandIndex >= 0)
    {
        size = m_commandBuffers[commandIndex]->GetAllocated();
    }
    LeaveCriticalSection(&rtl_critical_section100);
    return size;
}

void Renderer::CBuffStart(int index, bool full)
{
    Renderer::Context &c = this->getContext();
    c.commandBuffer = new (std::nothrow) Renderer::CommandBuffer(full);
    c.recordingBufferIndex = index;

    assert(c.stackType == MATRIX_MODE_MODELVIEW);

    c.stackType = MATRIX_MODE_MODELVIEW_CBUFF;
    c.stackPos[MATRIX_MODE_MODELVIEW_CBUFF] = 0;
    c.matrixStacks[MATRIX_MODE_MODELVIEW_CBUFF][0] = DirectX::XMMatrixIdentity();
    c.matrixDirty[MATRIX_MODE_MODELVIEW_CBUFF] = true;
}

void Renderer::CBuffTick()
{
    EnterCriticalSection(&rtl_critical_section100);

    const int firstPending = MAX_COMMAND_BUFFERS - static_cast<int>(reservedRendererDword3);
    for (int i = firstPending; i < MAX_COMMAND_BUFFERS; ++i)
    {
        Renderer::CommandBuffer *buffer = m_commandBuffers[i];
        if (buffer)
        {
            delete buffer;
        }
        m_commandBuffers[i] = nullptr;
    }

    reservedRendererDword3 = 0;
    LeaveCriticalSection(&rtl_critical_section100);
}

void Renderer::DeleteInternalBuffer(int index)
{
    EnterCriticalSection(&rtl_critical_section100);

    ++reservedRendererDword3;
    const int recycledSlot = MAX_COMMAND_BUFFERS - static_cast<int>(reservedRendererDword3);

    m_commandBuffers[recycledSlot] = m_commandBuffers[index];
    m_commandMatrices[recycledSlot] = m_commandMatrices[index];

    if (reservedRendererDword2-- != 1)
    {
        const int lastActive = reservedRendererDword2;

        m_commandBuffers[index] = m_commandBuffers[lastActive];
        m_commandMatrices[index] = m_commandMatrices[lastActive];
        m_commandVertexTypes[index] = m_commandVertexTypes[lastActive];
        m_commandPrimitiveTypes[index] = m_commandPrimitiveTypes[lastActive];

        const int commandIndex = m_commandIndexToHandle[lastActive];
        m_commandHandleToIndex[commandIndex] = static_cast<std::int16_t>(index);
        m_commandIndexToHandle[index] = commandIndex;
    }

    LeaveCriticalSection(&rtl_critical_section100);
}
