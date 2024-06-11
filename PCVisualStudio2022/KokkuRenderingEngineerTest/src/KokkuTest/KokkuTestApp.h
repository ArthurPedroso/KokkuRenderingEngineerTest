#pragma once

#include "CastleScene.h"

#include <Application/Interfaces/IApp.h>
#include <Application/Interfaces/IFont.h>
#include <Application/Interfaces/ICameraController.h>
#include <Application/Interfaces/IProfiler.h>
#include <Application/Interfaces/IScreenshot.h>
#include <Application/Interfaces/IUI.h>
#include <Game/Interfaces/IScripting.h>

#include <Utilities/RingBuffer.h>

// Math
#include <Utilities/Interfaces/IMemory.h>

class KokkuTestApp : public IApp
{
private:

    struct UniformBlock
    {
        CameraMatrix mProjectView;
        mat4         mScaleMat;

        // Point Light Information
        vec3 mLightPosition;
        vec3 mLightColor;

        uint32_t mSubmeshCount;
    };

    struct UniformBlockSky
    {
        CameraMatrix mProjectView;
    };

    // But we only need Two sets of resources (one in flight and one being used on CPU)
    static const uint32_t gDataBufferCount = 2;

    Renderer* pRenderer = NULL;

    Queue* pGraphicsQueue = NULL;
    GpuCmdRing gGraphicsCmdRing = {};

    SwapChain* pSwapChain = NULL;
    RenderTarget* pDepthBuffer = NULL;
    Semaphore* pImageAcquiredSemaphore = NULL;

    Shader* pCastleShader = NULL;
    Pipeline* pCastlePipeline = NULL;
    VertexLayout gCastleVertexLayout = {};

    Shader* pSkyBoxDrawShader = NULL;
    Buffer* pSkyBoxVertexBuffer = NULL;
    Pipeline* pSkyBoxDrawPipeline = NULL;
    RootSignature* pRootSignature = NULL;
    Sampler* pSamplerSkyBox = NULL;
    Sampler* pSmaplerCastle = NULL;
    Texture* pCastleAlbedo[3];
    Texture* pCastleBump[3];
    Texture* pSkyBoxTextures[6];
    DescriptorSet* pDescriptorSetTexture = { NULL };
    DescriptorSet* pDescriptorConstCastle = { NULL };
    DescriptorSet* pDescriptorSetUniforms = { NULL };

    Buffer* pProjViewUniformBuffer[gDataBufferCount] = { NULL };
    Buffer* pSkyboxUniformBuffer[gDataBufferCount] = { NULL };

    uint32_t     gFrameIndex = 0;
    ProfileToken gGpuProfileToken;

    UniformBlock     gUniformData;
    UniformBlockSky  gUniformDataSky;

    ICameraController* pCameraController = NULL;

    UIComponent* pGuiWindow = NULL;

    uint32_t gFontID = 0;

    QueryPool* pPipelineStatsQueryPool[gDataBufferCount] = {};

    unsigned char gPipelineStatsCharArray[2048] = {};
    bstring       gPipelineStats = bfromarr(gPipelineStatsCharArray);

    FontDrawDesc gFrameTimeDraw;

    CastleScene mCastleScene = {};
    Buffer* pSubmeshSizes = NULL;
    Texture** ppDiffuseTexs;

    void setupActions();
    
    bool addSwapChain();

    bool addDepthBuffer();

    void addDescriptorSets();

    void removeDescriptorSets();

    void addRootSignatures();

    void removeRootSignatures();

    void addShaders();
    void removeShaders();

    void addPipelines();

    void removePipelines();

    void prepareDescriptorSets();

    void loadCastle();
    void loadCastleTexs();

    bool setupCamera();

    void add_attribute(VertexLayout* layout, ShaderSemantic semantic, TinyImageFormat format, uint32_t offset);
    void copy_attribute(VertexLayout* layout, void* buffer_data, uint32_t offset, uint32_t size, uint32_t vcount, void* data);
    void compute_normal(const float* src, float* dst);
public:
    bool Init();
    void Exit();

    bool Load(ReloadDesc* pReloadDesc);
    void Unload(ReloadDesc* pReloadDesc);

    void Update(float deltaTime);
    void Draw();

    const char* GetName();
};

