#pragma once
#include "4J_Render.h"
#include <cstdint>
#include <unordered_map>
#include <vector>

#define MATRIX_MODE_MODELVIEW            0
#define MATRIX_MODE_MODELVIEW_PROJECTION 1
#define MATRIX_MODE_MODELVIEW_TEXTURE    2
#define MATRIX_MODE_MODELVIEW_CBUFF      3
#define MATRIX_MODE_MODELVIEW_MAX        4

#define STACK_TYPES    4
#define STACK_SIZE     16
#define MAX_MIP_LEVELS 5

#define NUM_COMMAND_HANDLES 0x800000
#define MAX_COMMAND_BUFFERS 16000

class Renderer
{
public:
    struct Context;
    struct CommandBuffer;

    void UpdateGamma(unsigned short usGamma);
    void MatrixMode(int type);
    void MatrixSetIdentity();
    void MatrixTranslate(float x, float y, float z);
    void MatrixRotate(float angle, float x, float y, float z);
    void MatrixScale(float x, float y, float z);
    void MatrixPerspective(float fovy, float aspect, float zNear, float zFar);
    void MatrixOrthogonal(float left, float right, float bottom, float top, float zNear, float zFar);
    void MatrixPop();
    void MatrixPush();
    void MatrixMult(float *mat);
    const float *MatrixGet(int type);
    void Set_matrixDirty();
    void Initialise(ID3D11Device *pDevice, IDXGISwapChain *pSwapChain);
    ID3D11DeviceContext *InitialiseContext(bool fromPresent);
    void StartFrame();
    void DoScreenGrabOnNextPresent();
    void Present();
    void Clear(int flags, D3D11_RECT *pRect);
    void SetClearColour(const float colourRGBA[4]);
    bool IsWidescreen();
    bool IsHiDef();
    void CaptureThumbnail(ImageFileBuffer *pngOut);
    void CaptureScreen(ImageFileBuffer *jpgOut, XSOCIAL_PREVIEWIMAGE *previewOut);
    void BeginConditionalSurvey(int identifier);
    void EndConditionalSurvey();
    void BeginConditionalRendering(int identifier);
    void EndConditionalRendering();
    void DrawVertices(C4JRender::ePrimitiveType PrimitiveType, int count, void *dataIn, C4JRender::eVertexType vType,
                      C4JRender::ePixelShaderType psType);
    void DrawVertexBuffer(C4JRender::ePrimitiveType PrimitiveType, int count, ID3D11Buffer *buffer, C4JRender::eVertexType vType,
                          C4JRender::ePixelShaderType psType);
    void CBuffLockStaticCreations();
    int CBuffCreate(int count);
    void CBuffDelete(int first, int count);
    void CBuffStart(int index, bool full);
    void CBuffClear(int index);
    int CBuffSize(int index);
    void CBuffEnd();
    bool CBuffCall(int index, bool full);
    void CBuffTick();
    void CBuffDeferredModeStart();
    void CBuffDeferredModeEnd();
    int TextureCreate();
    void TextureFree(int idx);
    void TextureBind(int idx);
    void TextureBindVertex(int idx);
    void TextureSetTextureLevels(int levels);
    int TextureGetTextureLevels();
    void TextureSetParam(int param, int value);
    void TextureDynamicUpdateStart();
    void TextureDynamicUpdateEnd();
    void TextureData(int width, int height, void *data, int level, C4JRender::eTextureFormat format);
    void TextureDataUpdate(int xoffset, int yoffset, int width, int height, void *data, int level);
    HRESULT LoadTextureData(const char *szFilename, D3DXIMAGE_INFO *pSrcInfo, int **ppDataOut);
    HRESULT LoadTextureData(BYTE *pbData, DWORD dwBytes, D3DXIMAGE_INFO *pSrcInfo, int **ppDataOut);
    HRESULT SaveTextureData(const char *szFilename, D3DXIMAGE_INFO *pSrcInfo, int *ppDataOut);
    HRESULT SaveTextureDataToMemory(void *pOutput, int outputCapacity, int *outputLength, int width, int height, int *ppDataIn);
    void TextureGetStats();
    ID3D11ShaderResourceView *TextureGetTexture(int idx);
    void StateSetColour(float r, float g, float b, float a);
    void StateSetDepthMask(bool enable);
    void StateSetBlendEnable(bool enable);
    void StateSetBlendFunc(int src, int dst);
    void StateSetBlendFactor(unsigned int colour);
    void StateSetAlphaFunc(int func, float param);
    void StateSetDepthFunc(int func);
    void StateSetFaceCull(bool enable);
    void StateSetFaceCullCW(bool enable);
    void StateSetLineWidth(float width);
    void StateSetWriteEnable(bool red, bool green, bool blue, bool alpha);
    void StateSetDepthTestEnable(bool enable);
    void StateSetAlphaTestEnable(bool enable);
    void StateSetDepthSlopeAndBias(float slope, float bias);
    void StateSetFogEnable(bool enable);
    void StateSetFogMode(int mode);
    void StateSetFogNearDistance(float dist);
    void StateSetFogFarDistance(float dist);
    void StateSetFogDensity(float density);
    void StateSetFogColour(float red, float green, float blue);
    void StateSetLightingEnable(bool enable);
    void StateSetVertexTextureUV(float u, float v);
    void StateSetLightColour(int light, float red, float green, float blue);
    void StateSetLightAmbientColour(float red, float green, float blue);
    void StateSetLightDirection(int light, float x, float y, float z);
    void StateSetLightEnable(int light, bool enable);
    void StateSetViewport(C4JRender::eViewportType viewportType);
    void StateSetEnableViewportClipPlanes(bool enable);
    void StateSetTexGenCol(int col, float x, float y, float z, float w, bool eyeSpace);
    void StateSetStencil(D3D11_COMPARISON_FUNC function, uint8_t stencil_ref, uint8_t stencil_func_mask, uint8_t stencil_write_mask);
    void StateSetForceLOD(int LOD);
    void BeginEvent(LPCWSTR eventName);
    void EndEvent();
    void Suspend();
    bool Suspended();
    void Resume();
    void StateUpdate();
private:
    void SetupShaders();
    void ConvertLinearToPng(ImageFileBuffer *pngOut, unsigned char *linearData, unsigned int width, unsigned int height);
    void DrawVertexSetup(C4JRender::eVertexType vType, C4JRender::ePixelShaderType psType, C4JRender::ePrimitiveType primitiveType, int *count,
                         bool *drawIndexed);
    void UpdateTexGenState();
    void UpdateLightingState();
    void UpdateViewportState();
    void UpdateFogState();
    void UpdateTextureState(bool vertexSampler);
    void MultWithStack(DirectX::XMMATRIX matrix);
    ID3D11DepthStencilState *GetManagedDepthStencilState();
    ID3D11BlendState *GetManagedBlendState();
    ID3D11RasterizerState *GetManagedRasterizerState();
    ID3D11SamplerState *GetManagedSamplerState();
    void DeleteInternalBuffer(int index);
    Renderer::Context &getContext();
    static D3D11_PRIMITIVE_TOPOLOGY *m_Topologies;
    static DXGI_FORMAT textureFormats[C4JRender::MAX_TEXTURE_FORMATS];
public:
    struct Texture
    {
        bool allocated;
        ID3D11Texture2D *texture;
        ID3D11ShaderResourceView *view;
        DWORD textureFlags;
        DWORD mipLevels;
        DWORD textureFormat;
        DWORD samplerParams;
    };

    struct TexgenCBuffer
    {
        DirectX::XMMATRIX unk0;
        DirectX::XMMATRIX unk1;
    };

    enum eCommandType
    {
        COMMAND_ADD_MATRIX,
        COMMAND_ADD_VERTICES,
        COMMAND_BIND_TEXTURE,
        COMMAND_SET_COLOR,
        COMMAND_SET_DEPTH_FUNC,
        COMMAND_SET_DEPTH_MASK,
        COMMAND_SET_DEPTH_TEST,
        COMMAND_SET_LIGHTING_ENABLE,
        COMMAND_SET_LIGHT_ENABLE,
        COMMAND_SET_LIGHT_DIRECTION,
        COMMAND_SET_LIGHT_COLOUR,
        COMMAND_SET_LIGHT_AMBIENT_COLOUR,
        COMMAND_SET_BLEND_ENABLE,
        COMMAND_SET_BLEND_FUNC,
        COMMAND_SET_BLEND_FACTOR,
        COMMAND_SET_FACE_CULL,
    };

    struct CommandBuffer
    {
        CommandBuffer(bool full);
        ~CommandBuffer();
        void StartRecording();
        void EndRecording(ID3D11Device *device);
        std::uint64_t GetAllocated();
        bool IsBusy();
        void AddMatrix(const float *matrix);
        void AddVertices(unsigned int stride, unsigned int count, void *dataIn, Renderer::Context &c);
        void BindTexture(int idx);
        void SetColor(float r, float g, float b, float a);
        void SetDepthFunc(int func);
        void SetDepthMask(bool enable);
        void SetDepthTestEnable(bool enable);
        void SetLightingEnable(bool enable);
        void SetLightEnable(int light, bool enable);
        void SetLightDirection(int light, float x, float y, float z);
        void SetLightColour(int light, float r, float g, float b);
        void SetLightAmbientColour(float r, float g, float b);
        void SetBlendEnable(bool enable);
        void SetBlendFunc(int src, int dst);
        void SetBlendFactor(unsigned int factor);
        void SetFaceCull(bool enable);
        void Render(C4JRender::eVertexType vType, Renderer::Context &c, int primitiveType);

        struct Command
        {
            Renderer::eCommandType m_command_type;
            BYTE commandPadding[12];

            union
            {
                BYTE data[64];

                struct
                {
                    float m_matrix[16];
                    // DirectX::XMMATRIX m_matrix;
                } add_matrix;

                struct
                {
                    unsigned int m_vertex_index_start;
                    unsigned int m_vertex_count;
                } add_vertices;

                struct
                {
                    unsigned int m_texture_index;
                } bind_texture;

                struct
                {
                    float m_color[4];
                } set_color;

                struct
                {
                    int m_depth_func;
                } set_depth_func;

                struct
                {
                    bool m_enable;
                } set_depth_mask;

                struct
                {
                    bool m_enable;
                } set_depth_test;

                struct
                {
                    bool m_enable;
                } set_lighting_enable;

                struct
                {
                    int m_light_index;
                    bool m_enable;
                } set_light_enable;

                struct
                {
                    int m_light_index;
                    float padding[3];
                    float m_direction[4];
                } set_light_direction;

                struct
                {
                    int m_light_index;
                    float m_color[3];
                } set_light_colour;

                struct
                {
                    BYTE padding;
                    float m_color[3];
                } set_light_ambient_colour;

                struct
                {
                    bool m_enable;
                } set_blend_enable;

                struct
                {
                    int m_src;
                    int m_dst;
                } set_blend_func;

                struct
                {
                    unsigned int m_blend_factor;
                } set_blend_factor;

                struct
                {
                    bool m_enable;
                } set_face_cull;
            };
        };
        ID3D11Buffer *m_vertexBuffer;
        void *m_vertexData;
        std::uint64_t m_vertexDataLength;
        std::vector<Command> m_commands;
        std::uint64_t m_allocated;
        BYTE isActive;
    };

    struct DeferredCBuff
    {
        Renderer::CommandBuffer *m_command_buf;
        int m_vertex_index;
        int m_vertex_type;
        int m_primitive_type;
        DirectX::XMMATRIX m_matrix;
    };

    struct Context
    {
        static const unsigned int VERTEX_BUFFER_SIZE = 0x100000;

        Context(ID3D11Device *device, ID3D11DeviceContext *deviceContext);

        ID3D11DeviceContext *m_pDeviceContext;
        ID3DUserDefinedAnnotation *userAnnotation;
        DWORD annotateDepth;
        DirectX::XMMATRIX matrixStacks[MATRIX_MODE_MODELVIEW_MAX][STACK_SIZE];
        bool matrixDirty[MATRIX_MODE_MODELVIEW_MAX];
        DWORD stackPos[MATRIX_MODE_MODELVIEW_MAX];
        DWORD stackType;
        DWORD textureIdx;
        BYTE faceCullEnabled;
        BYTE depthTestEnabled;
        BYTE alphaTestEnabled;
        float alphaReference;
        BYTE depthWriteEnabled;
        BYTE fogEnabled;
        float fogNearDistance;
        float fogFarDistance;
        float fogDensity;
        float fogColourRed;
        float fogColourBlue;
        float fogColourGreen;
        DWORD fogMode;
        BYTE lightingEnabled;
        BYTE lightEnabled[2];
        BYTE lightingDirty;
        DWORD forcedLOD;
        BYTE paddingAfterForceLOD[4];
        DirectX::XMFLOAT4 lightDirection[2];
        DirectX::XMFLOAT4 lightColour[2];
        DirectX::XMFLOAT4 lightAmbientColour;
        ID3D11Buffer *m_modelViewMatrix;
        ID3D11Buffer *m_localTransformMatrix;
        ID3D11Buffer *m_projectionMatrix;
        ID3D11Buffer *m_textureMatrix;
        ID3D11Buffer *m_vertexTexcoordBuffer;
        ID3D11Buffer *m_fogParamsBuffer;
        ID3D11Buffer *m_lightingStateBuffer;
        ID3D11Buffer *m_texGenMatricesBuffer;
        ID3D11Buffer *m_compressedTranslationBuffer;
        ID3D11Buffer *m_thumbnailBoundsBuffer;
        ID3D11Buffer *m_tintColorBuffer;
        ID3D11Buffer *m_fogColourBuffer;
        ID3D11Buffer *m_unkColorBuffer;
        ID3D11Buffer *m_alphaTestBuffer;
        ID3D11Buffer *m_clearColorBuffer;
        ID3D11Buffer *m_forcedLODBuffer;
        uint64_t dynamicVertexBase;
        DWORD dynamicVertexOffset;
        ID3D11Buffer *dynamicVertexBuffer;
        DirectX::XMMATRIX texGenMatrices[2];
        Renderer::CommandBuffer *commandBuffer;
        DWORD recordingBufferIndex;
        DWORD recordingVertexType;
        DWORD recordingPrimitiveType;
        BYTE deferredModeEnabled;
        std::vector<DeferredCBuff> deferredBuffers;
        D3D11_BLEND_DESC blendDesc;
        D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
        D3D11_RASTERIZER_DESC rasterizerDesc;
        float blendFactor[4];
    };

    static DWORD tlsIdx;
    static unsigned int s_auiWidths[MAX_MIP_LEVELS + 1];
    static unsigned int s_auiHeights[MAX_MIP_LEVELS + 1];
    static D3D11_INPUT_ELEMENT_DESC g_vertex_PTN_Elements_PF3_TF2_CB4_NB4_XW1[5];
    static D3D11_INPUT_ELEMENT_DESC g_vertex_PTN_Elements_Compressed[2];
    static D3D11_PRIMITIVE_TOPOLOGY g_topologies[C4JRender::PRIMITIVE_TYPE_COUNT];
    static int totalAlloc;
    static _RTL_CRITICAL_SECTION totalAllocCS;

    float m_fClearColor[4];
    ID3D11Device *m_pDevice;
    ID3D11DeviceContext *m_pDeviceContext;
    IDXGISwapChain *m_pSwapChain;
    ID3D11RenderTargetView *renderTargetView;
    ID3D11RenderTargetView *renderTargetViews[4];
    ID3D11ShaderResourceView *renderTargetShaderResourceView;
    ID3D11ShaderResourceView *renderTargetShaderResourceViews[4];
    ID3D11Texture2D *renderTargetTextures[4];
    ID3D11DepthStencilView *depthStencilView;
    ID3D11VertexShader **vertexShaderTable;
    ID3D11VertexShader *screenSpaceVertexShader;
    ID3D11VertexShader *screenClearVertexShader;
    ID3D11PixelShader **pixelShaderTable;
    ID3D11PixelShader *screenSpacePixelShader;
    ID3D11PixelShader *screenClearPixelShader;
    unsigned int *vertexStrideTable;
    ID3D11InputLayout **inputLayoutTable;
    ID3D11Buffer *quadIndexBuffer;
    ID3D11Buffer *fanIndexBuffer;
    DWORD defaultTextureIndex;
    WORD reservedRendererWord0;
    BYTE paddingAfterRendererWord0[2];
    DWORD presentCount;
    BYTE rendererFlag0;
    BYTE paddingAfterRendererFlag0[3];
    _RTL_CRITICAL_SECTION rtl_critical_section100;
    DWORD activeVertexType;
    DWORD activePixelType;
    C4JRender::eViewportType m_ViewportType;
    BYTE reservedRendererByte0;
    BYTE paddingAfterViewportType[3];
    Renderer::Texture m_textures[512];
    DWORD backBufferWidth;
    DWORD backBufferHeight;
    BYTE reservedRendererByte1;
    BYTE paddingAfterRendererByte1[3];
    DWORD reservedRendererDword1;
    int16_t *m_commandHandleToIndex;
    CommandBuffer **m_commandBuffers;
    uint8_t *m_commandPrimitiveTypes;
    DirectX::XMMATRIX *m_commandMatrices;
    int *m_commandIndexToHandle;
    uint8_t *m_commandVertexTypes;
    DWORD reservedRendererDword2;
    DWORD reservedRendererDword3;
    std::unordered_map<int, ID3D11BlendState *> managedBlendStates;
    std::unordered_map<int, ID3D11DepthStencilState *> managedDepthStencilStates;
    std::unordered_map<int, ID3D11SamplerState *> managedSamplerStates;
    std::unordered_map<int, ID3D11RasterizerState *> managedRasterizerStates;
    BYTE shouldScreenGrabNextFrame;
    BYTE suspended;
    BYTE paddingAfterSuspendState[2];
};

// Singleton
extern Renderer InternalRenderManager;
