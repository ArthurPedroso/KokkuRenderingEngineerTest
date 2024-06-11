#pragma once
#include "CastleScene.h"

#include <Application/Interfaces/IApp.h>
#include <Application/Interfaces/ICameraController.h>
#include <Application/Interfaces/IFont.h>
#include <Application/Interfaces/IInput.h>
#include <Application/Interfaces/IProfiler.h>
#include <Application/Interfaces/IScreenshot.h>
#include <Application/Interfaces/IUI.h>
#include <Game/Interfaces/IScripting.h>
#include <Utilities/Interfaces/IFileSystem.h>
#include <Utilities/Interfaces/ILog.h>
#include <Utilities/Interfaces/ITime.h>

#include <Utilities/RingBuffer.h>

// Renderer
#include <Graphics/Interfaces/IGraphics.h>
#include <Resources/ResourceLoader/Interfaces/IResourceLoader.h>

// Math

#include <Utilities/Interfaces/IMemory.h>
class KokkuTestApp : public IApp
{
private:
    /// Demo structures
    struct PlanetInfoStruct
    {
        mat4  mTranslationMat;
        mat4  mScaleMat;
        vec4  mColor;
    };

    struct UniformBlock
    {
        CameraMatrix mProjectView;
        mat4         mToWorldMat;
        vec4         mColor;
        float        mGeometryWeight[4];

        // Point Light Information
        vec3 mLightPosition;
        vec3 mLightColor;
    };

    struct UniformBlockSky
    {
        CameraMatrix mProjectView;
    };

    // But we only need Two sets of resources (one in flight and one being used on CPU)
    static const uint32_t gDataBufferCount = 2;
    static const uint     gNumPlanets = 11;     // Sun, Mercury -> Neptune, Pluto, Moon
    static const uint     gTimeOffset = 600000; // For visually better starting locations
    const float    gRotSelfScale = 0.0004f;
    const float    gRotOrbitYScale = 0.001f;
    const float    gRotOrbitZScale = 0.00001f;

    Renderer* pRenderer = NULL;

    Queue* pGraphicsQueue = NULL;
    GpuCmdRing gGraphicsCmdRing = {};

    SwapChain* pSwapChain = NULL;
    RenderTarget* pDepthBuffer = NULL;
    Semaphore* pImageAcquiredSemaphore = NULL;

    Shader* pSphereShader = NULL;
    Buffer* pSphereVertexBuffer = NULL;
    Buffer* pSphereIndexBuffer = NULL;
    uint32_t     gSphereIndexCount = 0;
    Pipeline* pSpherePipeline = NULL;
    VertexLayout gSphereVertexLayout = {};
    VertexLayout gCastleVertexLayout = {};
    uint32_t     gSphereLayoutType = 0;

    Shader* pSkyBoxDrawShader = NULL;
    Buffer* pSkyBoxVertexBuffer = NULL;
    Pipeline* pSkyBoxDrawPipeline = NULL;
    RootSignature* pRootSignature = NULL;
    Sampler* pSamplerSkyBox = NULL;
    Texture* pSkyBoxTextures[6];
    DescriptorSet* pDescriptorSetTexture = { NULL };
    DescriptorSet* pDescriptorSetUniforms = { NULL };

    Buffer* pProjViewUniformBuffer[gDataBufferCount] = { NULL };
    Buffer* pSkyboxUniformBuffer[gDataBufferCount] = { NULL };

    uint32_t     gFrameIndex = 0;
    ProfileToken gGpuProfileToken;

    int              gNumberOfSpherePoints;
    UniformBlock     gUniformData;
    UniformBlockSky  gUniformDataSky;
    PlanetInfoStruct gPlanetInfoData[gNumPlanets];

    ICameraController* pCameraController = NULL;

    UIComponent* pGuiWindow = NULL;

    uint32_t gFontID = 0;

    QueryPool* pPipelineStatsQueryPool[gDataBufferCount] = {};
    CastleScene castleScene = {};

    unsigned char gPipelineStatsCharArray[2048] = {};
    bstring       gPipelineStats = bfromarr(gPipelineStatsCharArray);

    FontDrawDesc gFrameTimeDraw;
    
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
    void add_attribute(VertexLayout* layout, ShaderSemantic semantic, TinyImageFormat format, uint32_t offset);
    void copy_attribute(VertexLayout* layout, void* buffer_data, uint32_t offset, uint32_t size, uint32_t vcount, void* data);
    void compute_normal(const float* src, float* dst);
    void generate_complex_mesh();
public:
    bool Init();
    void Exit();

    bool Load(ReloadDesc* pReloadDesc);
    void Unload(ReloadDesc* pReloadDesc);

    void Update(float deltaTime);
    void Draw();

    const char* GetName();
};

