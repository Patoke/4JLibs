#include "stdafx.h"
#include "Renderer.h"
#include "libpng/png.h"

unsigned char* dataStart;
unsigned char *dataCurr;
unsigned char *dataEnd;

DXGI_FORMAT Renderer::textureFormats[] = { DXGI_FORMAT_B8G8R8A8_UNORM };

void user_write_data_init(unsigned char* pBuffer, int size)
{
    dataStart = pBuffer;
    dataCurr = pBuffer;
    dataEnd = pBuffer + size;
}

int user_write_data_bytes_written()
{
    return static_cast<int>(dataCurr - dataStart);
}

void user_write_data(png_struct_def* png_ptr, unsigned char* src, size_t length)
{
    int bytesToWrite = static_cast<int>(dataEnd - dataCurr);
    if (static_cast<int>(length) < bytesToWrite)
        bytesToWrite = static_cast<int>(length);

    memcpy(dataCurr, src, bytesToWrite);
    dataCurr += bytesToWrite;
}

void user_flush_data(png_struct_def* png_ptr)
{
    // TODO(3UR): this is for a different platform? it's empty in Render_PC.lib but not Render.lib
}

int Renderer::TextureCreate()
{
    PROFILER_SCOPE("Renderer::TextureCreate", "TextureCreate", MP_PURPLE4)
    for (int i = 0; i < MAX_TEXTURES; i++)
    {
        if (!m_textures[i].allocated)
        {
            m_textures[i].texture = NULL;
            m_textures[i].allocated = true;
            m_textures[i].mipLevels = 1;
            m_textures[i].textureFormat = 0;
            return i;
        }
    }
    return -1;
}

void Renderer::TextureFree(int idx)
{
    PROFILER_SCOPE("Renderer::TextureFree", "TextureFree", MP_PURPLE4)
    if (m_textures[idx].texture)
    {
        m_textures[idx].texture->Release();
        m_textures[idx].texture = NULL;
    }
    if (m_textures[idx].view)
    {
        m_textures[idx].view->Release();
        m_textures[idx].view = NULL;
    }
    m_textures[idx].allocated = false;
}

void Renderer::TextureBind(int idx)
{
    PROFILER_SCOPE("Renderer::TextureBind", "TextureBind", MP_PURPLE4)
    if (idx == -1)
        idx = defaultTextureIndex;

    Context& c = getContext();

    if (c.commandBuffer && c.commandBuffer->isActive)
        c.commandBuffer->BindTexture(idx);

    c.textureIdx = idx;
    c.m_pDeviceContext->PSSetShaderResources(0, 1, &m_textures[idx].view);

    UpdateTextureState(false);
}

void Renderer::TextureBindVertex(int idx)
{
    PROFILER_SCOPE("Renderer::TextureBindVertex", "TextureBindVertex", MP_PURPLE4)
    if (idx == -1)
        idx = defaultTextureIndex;

    Context& c = getContext();

    c.textureIdx = idx;
    c.m_pDeviceContext->VSSetShaderResources(0, 1, &m_textures[idx].view);

    UpdateTextureState(true);
}

void Renderer::TextureSetTextureLevels(int levels)
{
    Context& c = getContext();
    m_textures[c.textureIdx].mipLevels = levels;
}

int Renderer::TextureGetTextureLevels()
{
    Context& c = getContext();
    return m_textures[c.textureIdx].mipLevels;
}

void Renderer::TextureData(int width, int height, void* data, int level, C4JRender::eTextureFormat format)
{
    PROFILER_SCOPE("Renderer::TextureData", "TextureData", MP_PURPLE4)
    Context& c = getContext();
    int idx = c.textureIdx;

    m_textures[idx].textureFormat = format;

    if (level == 0)
    {
        D3D11_TEXTURE2D_DESC desc;
        desc.Width = width;
        desc.Height = height;
        desc.MipLevels = m_textures[idx].mipLevels;
        desc.ArraySize = 1;
        desc.Format = textureFormats[format];
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;

        m_pDevice->CreateTexture2D(&desc, NULL, &m_textures[idx].texture);
        m_pDevice->CreateShaderResourceView(m_textures[idx].texture, NULL, &m_textures[idx].view);
    }

    c.m_pDeviceContext->UpdateSubresource(
        m_textures[idx].texture, 
        level, 
        NULL, 
        data, 
        static_cast<UINT>(width * 4),
        static_cast<UINT>(width * height * 4)
    );
}

void Renderer::TextureDataUpdate(int xoffset, int yoffset, int width, int height, void* data, int level)
{
    PROFILER_SCOPE("Renderer::TextureDataUpdate", "TextureDataUpdate", MP_PURPLE4)
    Context& c = getContext();
    int idx = c.textureIdx;

    D3D11_TEXTURE2D_DESC desc;
    m_textures[idx].texture->GetDesc(&desc);

    D3D11_BOX box;
    box.left = xoffset;
    box.top = yoffset;
    box.right = xoffset + width;
    box.bottom = yoffset + height;
    box.front = 0;
    box.back = 1;

    c.m_pDeviceContext->UpdateSubresource(
        m_textures[idx].texture,
        level,
        &box,
        data,
        static_cast<UINT>(width * 4),
        static_cast<UINT>(width * height * 4)
    );
}

void Renderer::TextureSetParam(int param, int value)
{
    Context& c = getContext();
    int idx = c.textureIdx;

    switch (param)
    {
    case GL_TEXTURE_MIN_FILTER:
        m_textures[idx].samplerParams &= ~4u;
        if (value == GL_LINEAR)
            m_textures[idx].samplerParams |= 4u;
        break;
    case GL_TEXTURE_MAG_FILTER:
        m_textures[idx].samplerParams &= ~8u;
        if (value == GL_LINEAR)
            m_textures[idx].samplerParams |= 8u;
        break;
    case GL_TEXTURE_WRAP_S:
        m_textures[idx].samplerParams &= ~1u;
        if (value == GL_CLAMP)
            m_textures[idx].samplerParams |= 1u;
        break;
    case GL_TEXTURE_WRAP_T:
        m_textures[idx].samplerParams &= ~2u;
        if (value == GL_CLAMP)
            m_textures[idx].samplerParams |= 2u;
        break;
    }
}

void Renderer::TextureDynamicUpdateStart()
{
    // TODO(3UR): this is for a different platform? it's empty in Render_PC.lib but not Render.lib
}

void Renderer::TextureDynamicUpdateEnd()
{
    // TODO(3UR): this is for a different platform? it's empty in Render_PC.lib but not Render.lib
}

void Renderer::UpdateTextureState(bool bVertex)
{
    Context& c = getContext();
    ID3D11SamplerState* pSampler = GetManagedSamplerState();

    if (bVertex)
        c.m_pDeviceContext->VSSetSamplers(0, 1, &pSampler);
    else
        c.m_pDeviceContext->PSSetSamplers(0, 1, &pSampler);
}

HRESULT Renderer::LoadTextureData(const char* szFilename, D3DXIMAGE_INFO* pSrcInfo, int** ppDataOut)
{
    PROFILER_SCOPE("Renderer::LoadTextureData_File", "LoadTextureData_File", MP_PURPLE4)
    png_image image;
    memset(&image, 0, sizeof(image));
    image.version = PNG_IMAGE_VERSION;

    if (!png_image_begin_read_from_file(&image, szFilename))
        return -1;

    // TODO(3UR): why crash?
    //if ((image.format & 3u) > 1)
    //    return -1;

    image.format = PNG_FORMAT_RGBA;

    *ppDataOut = new int[image.width * image.height];
    if (!*ppDataOut || !png_image_finish_read(&image, NULL, *ppDataOut, 0, NULL))
        return -1;

    pSrcInfo->Width = image.width;
    pSrcInfo->Height = image.height;
    return S_OK;
}

HRESULT Renderer::LoadTextureData(BYTE* pbData, DWORD dwBytes, D3DXIMAGE_INFO* pSrcInfo, int** ppDataOut)
{
    PROFILER_SCOPE("Renderer::LoadTextureData_Memory", "LoadTextureData_Memory", MP_PURPLE4)
    png_image image;
    memset(&image, 0, sizeof(image));
    image.version = PNG_IMAGE_VERSION;

    if (!png_image_begin_read_from_memory(&image, pbData, dwBytes))
        return -1;

    // TODO(3UR): why crash?
    //if ((image.format & 3u) > 1)
    //    return -1;

    image.format = PNG_FORMAT_RGBA;

    *ppDataOut = new int[image.width * image.height];
    if (!*ppDataOut || !png_image_finish_read(&image, NULL, *ppDataOut, 0, NULL))
        return -1;

    pSrcInfo->Width = image.width;
    pSrcInfo->Height = image.height;
    return S_OK;
}

HRESULT Renderer::SaveTextureData(const char* szFilename, D3DXIMAGE_INFO* pSrcInfo, int* ppDataOut)
{
    PROFILER_SCOPE("Renderer::SaveTextureData", "SaveTextureData", MP_PURPLE4)
    png_image image;
    memset(&image, 0, sizeof(image));
    image.width = pSrcInfo->Width;
    image.height = pSrcInfo->Height;
    image.version = PNG_IMAGE_VERSION;
    image.format = PNG_FORMAT_RGBA;

    png_image_write_to_file(&image, szFilename, 0, ppDataOut, 0, NULL);
    return S_OK;
}

HRESULT Renderer::SaveTextureDataToMemory(void* pOutput, int outputCapacity, int* outputLength, int width, int height, int* ppDataIn)
{
    PROFILER_SCOPE("Renderer::SaveTextureDataToMemory", "SaveTextureDataToMemory", MP_PURPLE4)
    png_image image;
    memset(&image, 0, sizeof(image));
    image.width = width;
    image.height = height;
    image.version = PNG_IMAGE_VERSION;
    image.format = PNG_FORMAT_RGBA;

    dataEnd = (BYTE*)pOutput + outputCapacity;
    dataStart = (BYTE*)pOutput;
    dataCurr = (BYTE*)pOutput;

    png_image_write_to_stdio(&image, NULL, 0, ppDataIn, 0, NULL, user_write_data, user_flush_data);

    *outputLength = static_cast<int>(dataCurr - dataStart);
    return S_OK;
}

void Renderer::TextureGetStats()
{
    // TODO(3UR): this is for a different platform? it's empty in Render_PC.lib but not Render.lib
}

ID3D11ShaderResourceView* Renderer::TextureGetTexture(int idx)
{
    if ((unsigned int)idx <= 511)
    {
        if (m_textures[idx].allocated)
            return m_textures[idx].view;
    }
    return NULL;
}