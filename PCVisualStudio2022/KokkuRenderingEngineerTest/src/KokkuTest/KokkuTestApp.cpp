#include "KokkuTestApp.h"


// Interfaces
#include <Application/Interfaces/IScreenshot.h>
#include <Application/Interfaces/IInput.h>
#include <Renderer/Interfaces/IVisibilityBuffer.h>
#include <Utilities/Interfaces/IFileSystem.h>
#include <Utilities/Interfaces/ILog.h>
#include <Utilities/Interfaces/ITime.h>

// Renderer
#include <Graphics/Interfaces/IGraphics.h>
#include <Resources/ResourceLoader/Interfaces/IResourceLoader.h>


const char* pSkyBoxImageFileNames[] = { "Skybox_right1.tex",  "Skybox_left2.tex",  "Skybox_top3.tex",
                                        "Skybox_bottom4.tex", "Skybox_front5.tex", "Skybox_back6.tex" };

// Generate sky box vertex buffer
const float gSkyBoxPoints[] = {
    10.0f,  -10.0f, -10.0f, 6.0f, // -z
    -10.0f, -10.0f, -10.0f, 6.0f,   -10.0f, 10.0f,  -10.0f, 6.0f,   -10.0f, 10.0f,
    -10.0f, 6.0f,   10.0f,  10.0f,  -10.0f, 6.0f,   10.0f,  -10.0f, -10.0f, 6.0f,

    -10.0f, -10.0f, 10.0f,  2.0f, //-x
    -10.0f, -10.0f, -10.0f, 2.0f,   -10.0f, 10.0f,  -10.0f, 2.0f,   -10.0f, 10.0f,
    -10.0f, 2.0f,   -10.0f, 10.0f,  10.0f,  2.0f,   -10.0f, -10.0f, 10.0f,  2.0f,

    10.0f,  -10.0f, -10.0f, 1.0f, //+x
    10.0f,  -10.0f, 10.0f,  1.0f,   10.0f,  10.0f,  10.0f,  1.0f,   10.0f,  10.0f,
    10.0f,  1.0f,   10.0f,  10.0f,  -10.0f, 1.0f,   10.0f,  -10.0f, -10.0f, 1.0f,

    -10.0f, -10.0f, 10.0f,  5.0f, // +z
    -10.0f, 10.0f,  10.0f,  5.0f,   10.0f,  10.0f,  10.0f,  5.0f,   10.0f,  10.0f,
    10.0f,  5.0f,   10.0f,  -10.0f, 10.0f,  5.0f,   -10.0f, -10.0f, 10.0f,  5.0f,

    -10.0f, 10.0f,  -10.0f, 3.0f, //+y
    10.0f,  10.0f,  -10.0f, 3.0f,   10.0f,  10.0f,  10.0f,  3.0f,   10.0f,  10.0f,
    10.0f,  3.0f,   -10.0f, 10.0f,  10.0f,  3.0f,   -10.0f, 10.0f,  -10.0f, 3.0f,

    10.0f,  -10.0f, 10.0f,  4.0f, //-y
    10.0f,  -10.0f, -10.0f, 4.0f,   -10.0f, -10.0f, -10.0f, 4.0f,   -10.0f, -10.0f,
    -10.0f, 4.0f,   -10.0f, -10.0f, 10.0f,  4.0f,   10.0f,  -10.0f, 10.0f,  4.0f,
};


const char* gWindowTestScripts[] = { "TestFullScreen.lua", "TestCenteredWindow.lua", "TestNonCenteredWindow.lua", "TestBorderless.lua" };

bool KokkuTestApp::Init()
{
    // FILE PATHS
    fsSetPathForResourceDir(pSystemFileIO, RM_CONTENT, RD_SHADER_BINARIES, "CompiledShaders");
    fsSetPathForResourceDir(pSystemFileIO, RM_CONTENT, RD_TEXTURES, "Textures");
    fsSetPathForResourceDir(pSystemFileIO, RM_CONTENT, RD_MESHES, "Meshes");
    fsSetPathForResourceDir(pSystemFileIO, RM_CONTENT, RD_FONTS, "Fonts");
    fsSetPathForResourceDir(pSystemFileIO, RM_DEBUG, RD_SCREENSHOTS, "Screenshots");
    fsSetPathForResourceDir(pSystemFileIO, RM_CONTENT, RD_SCRIPTS, "Scripts");
    fsSetPathForResourceDir(pSystemFileIO, RM_DEBUG, RD_DEBUG, "Debug");

    gGpuProfileToken = PROFILE_INVALID_TOKEN;
    // window and renderer setup
    RendererDesc settings;
    memset(&settings, 0, sizeof(settings));
    settings.mD3D11Supported = true;
    settings.mGLESSupported = true;
    initRenderer(GetName(), &settings, &pRenderer);
    // check for init success
    if (!pRenderer)
        return false;

    if (pRenderer->pGpu->mSettings.mPipelineStatsQueries)
    {
        QueryPoolDesc poolDesc = {};
        poolDesc.mQueryCount = 3; // The count is 3 due to quest & multi-view use otherwise 2 is enough as we use 2 queries.
        poolDesc.mType = QUERY_TYPE_PIPELINE_STATISTICS;
        for (uint32_t i = 0; i < gDataBufferCount; ++i)
        {
            addQueryPool(pRenderer, &poolDesc, &pPipelineStatsQueryPool[i]);
        }
    }

    QueueDesc queueDesc = {};
    queueDesc.mType = QUEUE_TYPE_GRAPHICS;
    queueDesc.mFlag = QUEUE_FLAG_INIT_MICROPROFILE;
    addQueue(pRenderer, &queueDesc, &pGraphicsQueue);

    GpuCmdRingDesc cmdRingDesc = {};
    cmdRingDesc.pQueue = pGraphicsQueue;
    cmdRingDesc.mPoolCount = gDataBufferCount;
    cmdRingDesc.mCmdPerPoolCount = 1;
    cmdRingDesc.mAddSyncPrimitives = true;
    addGpuCmdRing(pRenderer, &cmdRingDesc, &gGraphicsCmdRing);

    addSemaphore(pRenderer, &pImageAcquiredSemaphore);

    initResourceLoaderInterface(pRenderer);

    // Loads Skybox Textures
    for (int i = 0; i < 6; ++i)
    {
        TextureLoadDesc textureDesc = {};
        textureDesc.pFileName = pSkyBoxImageFileNames[i];
        textureDesc.ppTexture = &pSkyBoxTextures[i];
        // Textures representing color should be stored in SRGB or HDR format
        textureDesc.mCreationFlag = TEXTURE_CREATION_FLAG_SRGB;
        addResource(&textureDesc, NULL);
    }

    // Dynamic sampler that is bound at runtime
    SamplerDesc samplerDesc = { FILTER_LINEAR,
                                FILTER_LINEAR,
                                MIPMAP_MODE_NEAREST,
                                ADDRESS_MODE_CLAMP_TO_EDGE,
                                ADDRESS_MODE_CLAMP_TO_EDGE,
                                ADDRESS_MODE_CLAMP_TO_EDGE };
    addSampler(pRenderer, &samplerDesc, &pSamplerSkyBox);

    //Skybox VB
    uint64_t       skyBoxDataSize = 4 * 6 * 6 * sizeof(float);
    BufferLoadDesc skyboxVbDesc = {};
    skyboxVbDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_VERTEX_BUFFER;
    skyboxVbDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
    skyboxVbDesc.mDesc.mSize = skyBoxDataSize;
    skyboxVbDesc.pData = gSkyBoxPoints;
    skyboxVbDesc.ppBuffer = &pSkyBoxVertexBuffer;
    addResource(&skyboxVbDesc, NULL);

    BufferLoadDesc ubDesc = {};
    ubDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ubDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
    ubDesc.mDesc.mFlags = BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT;
    ubDesc.pData = NULL;
    for (uint32_t i = 0; i < gDataBufferCount; ++i)
    {
        ubDesc.mDesc.pName = "ProjViewUniformBuffer";
        ubDesc.mDesc.mSize = sizeof(UniformBlock);
        ubDesc.ppBuffer = &pProjViewUniformBuffer[i];
        addResource(&ubDesc, NULL);
        ubDesc.mDesc.pName = "SkyboxUniformBuffer";
        ubDesc.mDesc.mSize = sizeof(UniformBlockSky);
        ubDesc.ppBuffer = &pSkyboxUniformBuffer[i];
        addResource(&ubDesc, NULL);
    }

    // Load fonts
    FontDesc font = {};
    font.pFontPath = "TitilliumText/TitilliumText-Bold.otf";
    fntDefineFonts(&font, 1, &gFontID);

    FontSystemDesc fontRenderDesc = {};
    fontRenderDesc.pRenderer = pRenderer;
    if (!initFontSystem(&fontRenderDesc))
        return false; // report?

    // Initialize Forge User Interface Rendering
    UserInterfaceDesc uiRenderDesc = {};
    uiRenderDesc.pRenderer = pRenderer;
    initUserInterface(&uiRenderDesc);

    // Initialize micro profiler and its UI.
    ProfilerDesc profiler = {};
    profiler.pRenderer = pRenderer;
    profiler.mWidthUI = mSettings.mWidth;
    profiler.mHeightUI = mSettings.mHeight;
    initProfiler(&profiler);

    // Gpu profiler can only be added after initProfile.
    gGpuProfileToken = addGpuProfiler(pRenderer, pGraphicsQueue, "Graphics");

    /************************************************************************/
    // GUI
    /************************************************************************/
    UIComponentDesc guiDesc = {};
    guiDesc.mStartPosition = vec2(mSettings.mWidth * 0.01f, mSettings.mHeight * 0.2f);
    uiCreateComponent(GetName(), &guiDesc, &pGuiWindow);


    if (pRenderer->pGpu->mSettings.mPipelineStatsQueries)
    {
        static float4     color = { 1.0f, 1.0f, 1.0f, 1.0f };
        DynamicTextWidget statsWidget;
        statsWidget.pText = &gPipelineStats;
        statsWidget.pColor = &color;
        uiCreateComponentWidget(pGuiWindow, "Pipeline Stats", &statsWidget, WIDGET_TYPE_DYNAMIC_TEXT);
    }

    const uint32_t numScripts = TF_ARRAY_COUNT(gWindowTestScripts);
    LuaScriptDesc  scriptDescs[numScripts] = {};
    for (uint32_t i = 0; i < numScripts; ++i)
        scriptDescs[i].pScriptFileName = gWindowTestScripts[i];
    DEFINE_LUA_SCRIPTS(scriptDescs, numScripts);

    samplerDesc = { FILTER_LINEAR,
                                FILTER_LINEAR,
                                MIPMAP_MODE_NEAREST,
                                ADDRESS_MODE_CLAMP_TO_EDGE,
                                ADDRESS_MODE_CLAMP_TO_EDGE,
                                ADDRESS_MODE_CLAMP_TO_EDGE };
    addSampler(pRenderer, &samplerDesc, &pSmaplerCastle);

    loadCastleTexs();

    loadCastle();

    waitForAllResourceLoads();

    //-----CAMERA-----//
    bool result = setupCamera();


    setupActions();

    gFrameIndex = 0;

    return result;
}

void KokkuTestApp::Exit()
{
    exitInputSystem();

    exitCameraController(pCameraController);

    exitUserInterface();

    exitFontSystem();

    // Exit profile
    exitProfiler();

    for (uint32_t i = 0; i < gDataBufferCount; ++i)
    {
        removeResource(pProjViewUniformBuffer[i]);
        removeResource(pSkyboxUniformBuffer[i]);
        if (pRenderer->pGpu->mSettings.mPipelineStatsQueries)
        {
            removeQueryPool(pRenderer, pPipelineStatsQueryPool[i]);
        }
    }

    removeResource(pSkyBoxVertexBuffer);

    for (uint i = 0; i < 6; ++i)
        removeResource(pSkyBoxTextures[i]);

    for (uint i = 0; i < 3; ++i)
    {
        removeResource(pCastleAlbedo[i]);
        removeResource(pCastleBump[i]);
    }

    removeSampler(pRenderer, pSamplerSkyBox);
    removeSampler(pRenderer, pSmaplerCastle);

    removeGpuCmdRing(pRenderer, &gGraphicsCmdRing);
    removeSemaphore(pRenderer, pImageAcquiredSemaphore);

    exitResourceLoaderInterface(pRenderer);

    removeQueue(pRenderer, pGraphicsQueue);

    exitRenderer(pRenderer);
    pRenderer = NULL;
}

bool KokkuTestApp::Load(ReloadDesc* pReloadDesc)
{
    if (pReloadDesc->mType & RELOAD_TYPE_SHADER)
    {
        addShaders();
        addRootSignatures();
        addDescriptorSets();
    }

    if (pReloadDesc->mType & (RELOAD_TYPE_RESIZE | RELOAD_TYPE_RENDERTARGET))
    {
        if (!addSwapChain())
            return false;

        if (!addDepthBuffer())
            return false;
    }

    if (pReloadDesc->mType & (RELOAD_TYPE_SHADER | RELOAD_TYPE_RENDERTARGET))
    {
        addPipelines();
    }

    prepareDescriptorSets();

    UserInterfaceLoadDesc uiLoad = {};
    uiLoad.mColorFormat = pSwapChain->ppRenderTargets[0]->mFormat;
    uiLoad.mHeight = mSettings.mHeight;
    uiLoad.mWidth = mSettings.mWidth;
    uiLoad.mLoadType = pReloadDesc->mType;
    loadUserInterface(&uiLoad);

    FontSystemLoadDesc fontLoad = {};
    fontLoad.mColorFormat = pSwapChain->ppRenderTargets[0]->mFormat;
    fontLoad.mHeight = mSettings.mHeight;
    fontLoad.mWidth = mSettings.mWidth;
    fontLoad.mLoadType = pReloadDesc->mType;
    loadFontSystem(&fontLoad);

    initScreenshotInterface(pRenderer, pGraphicsQueue);

    return true;
}

void KokkuTestApp::Unload(ReloadDesc* pReloadDesc)
{
    waitQueueIdle(pGraphicsQueue);

    unloadFontSystem(pReloadDesc->mType);
    unloadUserInterface(pReloadDesc->mType);

    if (pReloadDesc->mType & (RELOAD_TYPE_SHADER | RELOAD_TYPE_RENDERTARGET))
    {
        removePipelines();
    }

    if (pReloadDesc->mType & (RELOAD_TYPE_RESIZE | RELOAD_TYPE_RENDERTARGET))
    {
        removeSwapChain(pRenderer, pSwapChain);
        removeRenderTarget(pRenderer, pDepthBuffer);
    }

    if (pReloadDesc->mType & RELOAD_TYPE_SHADER)
    {
        removeDescriptorSets();
        removeRootSignatures();
        removeShaders();
    }

    exitScreenshotInterface();
}

void KokkuTestApp::Update(float deltaTime)
{
    updateInputSystem(deltaTime, mSettings.mWidth, mSettings.mHeight);

    pCameraController->update(deltaTime);
    /************************************************************************/
    // Scene Update
    /************************************************************************/
    static float currentTime = 0.0f;
    currentTime += deltaTime * 1000.0f;

    // update camera with time
    mat4 viewMat = pCameraController->getViewMatrix();

    const float  aspectInverse = (float)mSettings.mHeight / (float)mSettings.mWidth;
    const float  horizontal_fov = PI / 2.0f;
    CameraMatrix projMat = CameraMatrix::perspectiveReverseZ(horizontal_fov, aspectInverse, 0.1f, 1000.0f);
    gUniformData.mSubmeshCount = mCastleScene.getGeometry()->mDrawArgCount;
    gUniformData.mProjectView = projMat * viewMat;

    // point light parameters
    gUniformData.mLightPosition = vec3(0.5f, 0.5f, 0.5f);
    gUniformData.mLightColor = vec3(0.9f, 0.9f, 0.7f); // Pale Yellow

    // update transformations
    mat4 trans, scale;
    trans = mat4::identity();
    scale = mat4::identity();

    scale[0][0] *= 100;
    scale[1][1] *= 100;
    scale[2][2] *= 100;

    gUniformData.mScaleMat = trans * scale;

    viewMat.setTranslation(vec3(0));
    gUniformDataSky = {};
    gUniformDataSky.mProjectView = projMat * viewMat;
}

void KokkuTestApp::Draw()
{
    if (pSwapChain->mEnableVsync != mSettings.mVSyncEnabled)
    {
        waitQueueIdle(pGraphicsQueue);
        ::toggleVSync(pRenderer, &pSwapChain);
    }

    uint32_t swapchainImageIndex;
    acquireNextImage(pRenderer, pSwapChain, pImageAcquiredSemaphore, NULL, &swapchainImageIndex);

    RenderTarget* pRenderTarget = pSwapChain->ppRenderTargets[swapchainImageIndex];
    GpuCmdRingElement elem = getNextGpuCmdRingElement(&gGraphicsCmdRing, true, 1);

    // Stall if CPU is running "gDataBufferCount" frames ahead of GPU
    FenceStatus fenceStatus;
    getFenceStatus(pRenderer, elem.pFence, &fenceStatus);
    if (fenceStatus == FENCE_STATUS_INCOMPLETE)
        waitForFences(pRenderer, 1, &elem.pFence);

    // Update uniform buffers
    BufferUpdateDesc viewProjCbv = { pProjViewUniformBuffer[gFrameIndex] };
    beginUpdateResource(&viewProjCbv);
    memcpy(viewProjCbv.pMappedData, &gUniformData, sizeof(gUniformData));
    endUpdateResource(&viewProjCbv);

    BufferUpdateDesc skyboxViewProjCbv = { pSkyboxUniformBuffer[gFrameIndex] };
    beginUpdateResource(&skyboxViewProjCbv);
    memcpy(skyboxViewProjCbv.pMappedData, &gUniformDataSky, sizeof(gUniformDataSky));
    endUpdateResource(&skyboxViewProjCbv);

    // Reset cmd pool for this frame
    resetCmdPool(pRenderer, elem.pCmdPool);

    if (pRenderer->pGpu->mSettings.mPipelineStatsQueries)
    {
        QueryData data3D = {};
        QueryData data2D = {};
        getQueryData(pRenderer, pPipelineStatsQueryPool[gFrameIndex], 0, &data3D);
        getQueryData(pRenderer, pPipelineStatsQueryPool[gFrameIndex], 1, &data2D);
        bformat(&gPipelineStats,
            "\n"
            "Pipeline Stats 3D:\n"
            "    VS invocations:      %u\n"
            "    PS invocations:      %u\n"
            "    Clipper invocations: %u\n"
            "    IA primitives:       %u\n"
            "    Clipper primitives:  %u\n"
            "\n"
            "Pipeline Stats 2D UI:\n"
            "    VS invocations:      %u\n"
            "    PS invocations:      %u\n"
            "    Clipper invocations: %u\n"
            "    IA primitives:       %u\n"
            "    Clipper primitives:  %u\n",
            data3D.mPipelineStats.mVSInvocations, data3D.mPipelineStats.mPSInvocations, data3D.mPipelineStats.mCInvocations,
            data3D.mPipelineStats.mIAPrimitives, data3D.mPipelineStats.mCPrimitives, data2D.mPipelineStats.mVSInvocations,
            data2D.mPipelineStats.mPSInvocations, data2D.mPipelineStats.mCInvocations, data2D.mPipelineStats.mIAPrimitives,
            data2D.mPipelineStats.mCPrimitives);
    }

    Cmd* cmd = elem.pCmds[0];
    beginCmd(cmd);

    cmdBeginGpuFrameProfile(cmd, gGpuProfileToken);
    if (pRenderer->pGpu->mSettings.mPipelineStatsQueries)
    {
        cmdResetQuery(cmd, pPipelineStatsQueryPool[gFrameIndex], 0, 2);
        QueryDesc queryDesc = { 0 };
        cmdBeginQuery(cmd, pPipelineStatsQueryPool[gFrameIndex], &queryDesc);
    }

    RenderTargetBarrier barriers[] = {
        { pRenderTarget, RESOURCE_STATE_PRESENT, RESOURCE_STATE_RENDER_TARGET },
    };
    cmdResourceBarrier(cmd, 0, NULL, 0, NULL, 1, barriers);

    cmdBeginGpuTimestampQuery(cmd, gGpuProfileToken, "Draw Skybox");

    // simply record the screen cleaning command
    BindRenderTargetsDesc bindRenderTargets = {};
    bindRenderTargets.mRenderTargetCount = 1;
    bindRenderTargets.mRenderTargets[0] = { pRenderTarget, LOAD_ACTION_CLEAR };
    bindRenderTargets.mDepthStencil = { pDepthBuffer, LOAD_ACTION_CLEAR };
    cmdBindRenderTargets(cmd, &bindRenderTargets);
    cmdSetViewport(cmd, 0.0f, 0.0f, (float)pRenderTarget->mWidth, (float)pRenderTarget->mHeight, 0.0f, 1.0f);
    cmdSetScissor(cmd, 0, 0, pRenderTarget->mWidth, pRenderTarget->mHeight);

    const uint32_t skyboxVbStride = sizeof(float) * 4;
    // draw skybox
    cmdBeginGpuTimestampQuery(cmd, gGpuProfileToken, "Draw Skybox");
    cmdSetViewport(cmd, 0.0f, 0.0f, (float)pRenderTarget->mWidth, (float)pRenderTarget->mHeight, 1.0f, 1.0f);
    cmdBindPipeline(cmd, pSkyBoxDrawPipeline);
    cmdBindDescriptorSet(cmd, 0, pDescriptorSetTexture);
    cmdBindDescriptorSet(cmd, gFrameIndex * 2 + 0, pDescriptorSetUniforms);
    cmdBindVertexBuffer(cmd, 1, &pSkyBoxVertexBuffer, &skyboxVbStride, NULL);
    cmdDraw(cmd, 36, 0);
    cmdSetViewport(cmd, 0.0f, 0.0f, (float)pRenderTarget->mWidth, (float)pRenderTarget->mHeight, 0.0f, 1.0f);
    cmdEndGpuTimestampQuery(cmd, gGpuProfileToken);

    cmdBeginGpuTimestampQuery(cmd, gGpuProfileToken, "Draw Castle");

    cmdBindPipeline(cmd, pCastlePipeline);
    cmdBindDescriptorSet(cmd, 0, pDescriptorSetTexture);
    cmdBindDescriptorSet(cmd, gFrameIndex * 2 + 1, pDescriptorSetUniforms);
    cmdBindVertexBuffer(cmd, 3, mCastleScene.getGeometry()->pVertexBuffers, mCastleScene.getGeometry()->mVertexStrides, nullptr);
    cmdBindIndexBuffer(cmd, mCastleScene.getGeometry()->pIndexBuffer, INDEX_TYPE_UINT16, 0);

    cmdDrawIndexedInstanced(cmd, mCastleScene.getGeometry()->mIndexCount, 0, 1, 0, 0);
    cmdEndGpuTimestampQuery(cmd, gGpuProfileToken);
    cmdEndGpuTimestampQuery(cmd, gGpuProfileToken);
    cmdBindRenderTargets(cmd, NULL);

    if (pRenderer->pGpu->mSettings.mPipelineStatsQueries)
    {
        QueryDesc queryDesc = { 0 };
        cmdEndQuery(cmd, pPipelineStatsQueryPool[gFrameIndex], &queryDesc);

        queryDesc = { 1 };
        cmdBeginQuery(cmd, pPipelineStatsQueryPool[gFrameIndex], &queryDesc);
    }

    cmdBeginGpuTimestampQuery(cmd, gGpuProfileToken, "Draw UI");

    bindRenderTargets = {};
    bindRenderTargets.mRenderTargetCount = 1;
    bindRenderTargets.mRenderTargets[0] = { pRenderTarget, LOAD_ACTION_LOAD };
    bindRenderTargets.mDepthStencil = { NULL, LOAD_ACTION_DONTCARE };
    cmdBindRenderTargets(cmd, &bindRenderTargets);

    gFrameTimeDraw.mFontColor = 0xff00ffff;
    gFrameTimeDraw.mFontSize = 18.0f;
    gFrameTimeDraw.mFontID = gFontID;
    float2 txtSizePx = cmdDrawCpuProfile(cmd, float2(8.f, 15.f), &gFrameTimeDraw);
    cmdDrawGpuProfile(cmd, float2(8.f, txtSizePx.y + 75.f), gGpuProfileToken, &gFrameTimeDraw);

    cmdDrawUserInterface(cmd);

    cmdEndGpuTimestampQuery(cmd, gGpuProfileToken);
    cmdBindRenderTargets(cmd, NULL);

    barriers[0] = { pRenderTarget, RESOURCE_STATE_RENDER_TARGET, RESOURCE_STATE_PRESENT };
    cmdResourceBarrier(cmd, 0, NULL, 0, NULL, 1, barriers);

    cmdEndGpuFrameProfile(cmd, gGpuProfileToken);

    if (pRenderer->pGpu->mSettings.mPipelineStatsQueries)
    {
        QueryDesc queryDesc = { 1 };
        cmdEndQuery(cmd, pPipelineStatsQueryPool[gFrameIndex], &queryDesc);
        cmdResolveQuery(cmd, pPipelineStatsQueryPool[gFrameIndex], 0, 2);
    }

    endCmd(cmd);

    FlushResourceUpdateDesc flushUpdateDesc = {};
    flushUpdateDesc.mNodeIndex = 0;
    flushResourceUpdates(&flushUpdateDesc);
    Semaphore* waitSemaphores[2] = { flushUpdateDesc.pOutSubmittedSemaphore, pImageAcquiredSemaphore };

    QueueSubmitDesc submitDesc = {};
    submitDesc.mCmdCount = 1;
    submitDesc.mSignalSemaphoreCount = 1;
    submitDesc.mWaitSemaphoreCount = TF_ARRAY_COUNT(waitSemaphores);
    submitDesc.ppCmds = &cmd;
    submitDesc.ppSignalSemaphores = &elem.pSemaphore;
    submitDesc.ppWaitSemaphores = waitSemaphores;
    submitDesc.pSignalFence = elem.pFence;
    queueSubmit(pGraphicsQueue, &submitDesc);

    QueuePresentDesc presentDesc = {};
    presentDesc.mIndex = swapchainImageIndex;
    presentDesc.mWaitSemaphoreCount = 1;
    presentDesc.pSwapChain = pSwapChain;
    presentDesc.ppWaitSemaphores = &elem.pSemaphore;
    presentDesc.mSubmitDone = true;

    queuePresent(pGraphicsQueue, &presentDesc);
    flipProfiler();

    gFrameIndex = (gFrameIndex + 1) % gDataBufferCount;
}

const char* KokkuTestApp::GetName()
{
    return "KokkuRenderingEngineerTestApp";
}

bool KokkuTestApp::addSwapChain()
{
    SwapChainDesc swapChainDesc = {};
    swapChainDesc.mWindowHandle = pWindow->handle;
    swapChainDesc.mPresentQueueCount = 1;
    swapChainDesc.ppPresentQueues = &pGraphicsQueue;
    swapChainDesc.mWidth = mSettings.mWidth;
    swapChainDesc.mHeight = mSettings.mHeight;
    swapChainDesc.mImageCount = getRecommendedSwapchainImageCount(pRenderer, &pWindow->handle);
    swapChainDesc.mColorFormat = getSupportedSwapchainFormat(pRenderer, &swapChainDesc, COLOR_SPACE_SDR_SRGB);
    swapChainDesc.mColorSpace = COLOR_SPACE_SDR_SRGB;
    swapChainDesc.mEnableVsync = mSettings.mVSyncEnabled;
    swapChainDesc.mFlags = SWAP_CHAIN_CREATION_FLAG_ENABLE_FOVEATED_RENDERING_VR;
    ::addSwapChain(pRenderer, &swapChainDesc, &pSwapChain);

    return pSwapChain != NULL;
}

bool KokkuTestApp::addDepthBuffer()
{
    // Add depth buffer
    RenderTargetDesc depthRT = {};
    depthRT.mArraySize = 1;
    depthRT.mClearValue.depth = 0.0f;
    depthRT.mClearValue.stencil = 0;
    depthRT.mDepth = 1;
    depthRT.mFormat = TinyImageFormat_D32_SFLOAT;
    depthRT.mStartState = RESOURCE_STATE_DEPTH_WRITE;
    depthRT.mHeight = mSettings.mHeight;
    depthRT.mSampleCount = SAMPLE_COUNT_1;
    depthRT.mSampleQuality = 0;
    depthRT.mWidth = mSettings.mWidth;
    depthRT.mFlags = TEXTURE_CREATION_FLAG_ON_TILE | TEXTURE_CREATION_FLAG_VR_MULTIVIEW;
    addRenderTarget(pRenderer, &depthRT, &pDepthBuffer);

    return pDepthBuffer != NULL;
}

void KokkuTestApp::addDescriptorSets()
{
    DescriptorSetDesc desc = { pRootSignature, DESCRIPTOR_UPDATE_FREQ_NONE, 1 };
    addDescriptorSet(pRenderer, &desc, &pDescriptorSetTexture);
    desc = { pRootSignature, DESCRIPTOR_UPDATE_FREQ_PER_FRAME, gDataBufferCount * 2 };
    addDescriptorSet(pRenderer, &desc, &pDescriptorSetUniforms);
}

void KokkuTestApp::removeDescriptorSets()
{
    removeDescriptorSet(pRenderer, pDescriptorSetTexture);
    removeDescriptorSet(pRenderer, pDescriptorSetUniforms);
}

void KokkuTestApp::addRootSignatures()
{
    Shader* shaders[2];
    uint32_t shadersCount = 0;
    shaders[shadersCount++] = pCastleShader;
    shaders[shadersCount++] = pSkyBoxDrawShader;

    RootSignatureDesc rootDesc = {};
    rootDesc.mShaderCount = shadersCount;
    rootDesc.ppShaders = shaders;
    addRootSignature(pRenderer, &rootDesc, &pRootSignature);
}

void KokkuTestApp::removeRootSignatures() { removeRootSignature(pRenderer, pRootSignature); }

void KokkuTestApp::addShaders()
{
    ShaderLoadDesc skyShader = {};
    skyShader.mStages[0].pFileName = "skybox.vert";
    skyShader.mStages[1].pFileName = "skybox.frag";

    ShaderLoadDesc basicShader = {};
    basicShader.mStages[0].pFileName = "basic.vert";
    basicShader.mStages[1].pFileName = "basic.frag";

    addShader(pRenderer, &skyShader, &pSkyBoxDrawShader);
    addShader(pRenderer, &basicShader, &pCastleShader);
}

void KokkuTestApp::removeShaders()
{
    removeShader(pRenderer, pCastleShader);
    removeShader(pRenderer, pSkyBoxDrawShader);
}

void KokkuTestApp::addPipelines()
{
    RasterizerStateDesc rasterizerStateDesc = {};
    rasterizerStateDesc.mCullMode = CULL_MODE_NONE;

    RasterizerStateDesc castleRasterizerStateDesc = {};
    castleRasterizerStateDesc.mCullMode = CULL_MODE_NONE;

    DepthStateDesc depthStateDesc = {};
    depthStateDesc.mDepthTest = true;
    depthStateDesc.mDepthWrite = true;
    depthStateDesc.mDepthFunc = CMP_GEQUAL;

    PipelineDesc desc = {};
    desc.mType = PIPELINE_TYPE_GRAPHICS;
    GraphicsPipelineDesc& pipelineSettings = desc.mGraphicsDesc;
    pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
    pipelineSettings.mRenderTargetCount = 1;
    pipelineSettings.pDepthState = &depthStateDesc;
    pipelineSettings.pColorFormats = &pSwapChain->ppRenderTargets[0]->mFormat;
    pipelineSettings.mSampleCount = pSwapChain->ppRenderTargets[0]->mSampleCount;
    pipelineSettings.mSampleQuality = pSwapChain->ppRenderTargets[0]->mSampleQuality;
    pipelineSettings.mDepthStencilFormat = pDepthBuffer->mFormat;
    pipelineSettings.pRootSignature = pRootSignature;
    pipelineSettings.pShaderProgram = pCastleShader;
    pipelineSettings.pVertexLayout = &gCastleVertexLayout;
    pipelineSettings.pRasterizerState = &castleRasterizerStateDesc;
    pipelineSettings.mVRFoveatedRendering = true;
    addPipeline(pRenderer, &desc, &pCastlePipeline);

    // layout and pipeline for skybox draw
    VertexLayout vertexLayout = {};
    vertexLayout.mBindingCount = 1;
    vertexLayout.mBindings[0].mStride = sizeof(float4);
    vertexLayout.mAttribCount = 1;
    vertexLayout.mAttribs[0].mSemantic = SEMANTIC_POSITION;
    vertexLayout.mAttribs[0].mFormat = TinyImageFormat_R32G32B32A32_SFLOAT;
    vertexLayout.mAttribs[0].mBinding = 0;
    vertexLayout.mAttribs[0].mLocation = 0;
    vertexLayout.mAttribs[0].mOffset = 0;
    pipelineSettings.pVertexLayout = &vertexLayout;

    pipelineSettings.pDepthState = NULL;
    pipelineSettings.pRasterizerState = &rasterizerStateDesc;
    pipelineSettings.pShaderProgram = pSkyBoxDrawShader; //-V519
    addPipeline(pRenderer, &desc, &pSkyBoxDrawPipeline);
}

void KokkuTestApp::removePipelines()
{
    removePipeline(pRenderer, pSkyBoxDrawPipeline);
    removePipeline(pRenderer, pCastlePipeline);
}

void KokkuTestApp::prepareDescriptorSets()
{
    // Prepare descriptor sets
    DescriptorData params[15] = {};
    params[0].pName = "RightText";
    params[0].ppTextures = &pSkyBoxTextures[0];
    params[1].pName = "LeftText";
    params[1].ppTextures = &pSkyBoxTextures[1];
    params[2].pName = "TopText";
    params[2].ppTextures = &pSkyBoxTextures[2];
    params[3].pName = "BotText";
    params[3].ppTextures = &pSkyBoxTextures[3];
    params[4].pName = "FrontText";
    params[4].ppTextures = &pSkyBoxTextures[4];
    params[5].pName = "BackText";
    params[5].ppTextures = &pSkyBoxTextures[5];
    params[6].pName = "uSampler0";
    params[6].ppSamplers = &pSamplerSkyBox;
    params[7].pName = "Albedo1";
    params[7].ppTextures = &pCastleAlbedo[0];
    params[8].pName = "Albedo2";
    params[8].ppTextures = &pCastleAlbedo[1];
    params[9].pName = "Albedo3";
    params[9].ppTextures = &pCastleAlbedo[2];
    params[10].pName = "Bump1";
    params[10].ppTextures = &pCastleBump[0];
    params[11].pName = "Bump2";
    params[11].ppTextures = &pCastleBump[1];
    params[12].pName = "Bump3";
    params[12].ppTextures = &pCastleBump[2];
    params[13].pName = "uSampler1";
    params[13].ppSamplers = &pSmaplerCastle;
    params[14].pName = "submeshSizes";
    params[14].ppBuffers = &pSubmeshSizes;

    updateDescriptorSet(pRenderer, 0, pDescriptorSetTexture, 15, params);

    for (uint32_t i = 0; i < gDataBufferCount; ++i)
    {
        DescriptorData params[1] = {};
        params[0].pName = "uniformBlock";
        params[0].ppBuffers = &pSkyboxUniformBuffer[i];
        updateDescriptorSet(pRenderer, i * 2 + 0, pDescriptorSetUniforms, 1, params);

        params[0].pName = "uniformBlock";
        params[0].ppBuffers = &pProjViewUniformBuffer[i];
        updateDescriptorSet(pRenderer, i * 2 + 1, pDescriptorSetUniforms, 1, params);
    }
}

void KokkuTestApp::loadCastleTexs()
{
    //Albedo:
    TextureLoadDesc textureDesc = {};
    textureDesc.pFileName = "Castle Exterior Texture.dds";
    textureDesc.ppTexture = &pCastleAlbedo[0];
    // Textures representing color should be stored in SRGB or HDR format
    textureDesc.mCreationFlag = TEXTURE_CREATION_FLAG_SRGB;
    addResource(&textureDesc, NULL);
    textureDesc = {};
    textureDesc.pFileName = "Castle Interior Texture.dds";
    textureDesc.ppTexture = &pCastleAlbedo[1];
    // Textures representing color should be stored in SRGB or HDR format
    textureDesc.mCreationFlag = TEXTURE_CREATION_FLAG_SRGB;
    addResource(&textureDesc, NULL);
    textureDesc = {};
    textureDesc.pFileName = "Ground and Fountain Texture.dds";
    textureDesc.ppTexture = &pCastleAlbedo[2];
    // Textures representing color should be stored in SRGB or HDR format
    textureDesc.mCreationFlag = TEXTURE_CREATION_FLAG_SRGB;
    addResource(&textureDesc, NULL);

    //Bumps

    textureDesc.pFileName = "Castle Exterior Texture Bump.dds";
    textureDesc.ppTexture = &pCastleBump[0];
    // Textures representing color should be stored in SRGB or HDR format
    textureDesc.mCreationFlag = TEXTURE_CREATION_FLAG_SRGB;
    addResource(&textureDesc, NULL);
    textureDesc = {};
    textureDesc.pFileName = "Castle Interior Texture Bump.dds";
    textureDesc.ppTexture = &pCastleBump[1];
    // Textures representing color should be stored in SRGB or HDR format
    textureDesc.mCreationFlag = TEXTURE_CREATION_FLAG_SRGB;
    addResource(&textureDesc, NULL);
    textureDesc = {};
    textureDesc.pFileName = "Ground and Fountain Texture Bump.dds";
    textureDesc.ppTexture = &pCastleBump[2];
    // Textures representing color should be stored in SRGB or HDR format
    textureDesc.mCreationFlag = TEXTURE_CREATION_FLAG_SRGB;
    addResource(&textureDesc, NULL);
}

void KokkuTestApp::loadCastle()
{
    GeometryLoadDesc sceneLoadDesc = {};
    mCastleScene.Load(&sceneLoadDesc, false);

    uint32_t numSubmeshes = mCastleScene.getGeometry()->mDrawArgCount;
    const uint32_t maxSubmeshes = 256;
    uint32_t subMeshSizes[maxSubmeshes];
    uint32_t temp = 0;
    for (uint32_t i = 0; i < numSubmeshes && i < maxSubmeshes; i++)
    {
        temp += mCastleScene.getGeometry()->pDrawArgs[i].mIndexCount / 3;
        subMeshSizes[i] = temp;
    }

    BufferLoadDesc bDesc = {};
    bDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_BUFFER;
    bDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
    bDesc.mDesc.mFlags = BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT;
    bDesc.pData = subMeshSizes;
    bDesc.mDesc.mSize = sizeof(uint32_t) * numSubmeshes;
    bDesc.mDesc.pName = "submeshSizes";
    bDesc.ppBuffer = &pSubmeshSizes;
    bDesc.mDesc.mStartState = RESOURCE_STATE_SHADER_RESOURCE;
    bDesc.mDesc.mElementCount = numSubmeshes;
    bDesc.mDesc.mStructStride = sizeof(uint32_t);

    addResource(&bDesc, NULL);

    gCastleVertexLayout = {};
    gCastleVertexLayout.mAttribCount = 3;
    gCastleVertexLayout.mBindingCount = 3;
    gCastleVertexLayout.mAttribs[0].mSemantic = SEMANTIC_POSITION;
    gCastleVertexLayout.mAttribs[0].mFormat = TinyImageFormat_R32G32B32_SFLOAT;
    gCastleVertexLayout.mAttribs[0].mBinding = 0;
    gCastleVertexLayout.mAttribs[0].mLocation = 0;
    gCastleVertexLayout.mAttribs[1].mSemantic = SEMANTIC_NORMAL;
    gCastleVertexLayout.mAttribs[1].mFormat = TinyImageFormat_R16G16_UNORM;
    gCastleVertexLayout.mAttribs[1].mBinding = 1;
    gCastleVertexLayout.mAttribs[1].mLocation = 1;
    gCastleVertexLayout.mAttribs[2].mSemantic = SEMANTIC_TEXCOORD0;
    gCastleVertexLayout.mAttribs[2].mFormat = TinyImageFormat_R16G16_SFLOAT;
    gCastleVertexLayout.mAttribs[2].mBinding = 2;
    gCastleVertexLayout.mAttribs[2].mLocation = 2;

    waitForAllResourceLoads();
}

void KokkuTestApp::add_attribute(VertexLayout* layout, ShaderSemantic semantic, TinyImageFormat format, uint32_t offset)
{
    uint32_t n_attr = layout->mAttribCount++;

    VertexAttrib* attr = layout->mAttribs + n_attr;

    attr->mSemantic = semantic;
    attr->mFormat = format;
    attr->mBinding = 0;
    attr->mLocation = n_attr;
    attr->mOffset = offset;
}

void KokkuTestApp::copy_attribute(VertexLayout* layout, void* buffer_data, uint32_t offset, uint32_t size, uint32_t vcount, void* data)
{
    uint8_t* dst_data = static_cast<uint8_t*>(buffer_data);
    uint8_t* src_data = static_cast<uint8_t*>(data);
    for (uint32_t i = 0; i < vcount; ++i)
    {
        memcpy(dst_data + offset, src_data, size);

        dst_data += layout->mBindings[0].mStride;
        src_data += size;
    }
}

void KokkuTestApp::compute_normal(const float* src, float* dst)
{
    float len = sqrtf(src[0] * src[0] + src[1] * src[1] + src[2] * src[2]);
    if (len == 0)
    {
        dst[0] = 0;
        dst[1] = 0;
        dst[2] = 0;
    }
    else
    {
        dst[0] = src[0] / len;
        dst[1] = src[1] / len;
        dst[2] = src[2] / len;
    }
}

void KokkuTestApp::setupActions()
{

    // App Actions
    InputActionDesc actionDesc = { DefaultInputActions::DUMP_PROFILE_DATA,
                                   [](InputActionContext* ctx)
                                   {
                                       dumpProfileData(((Renderer*)ctx->pUserData)->pName);
                                       return true;
                                   },
                                   pRenderer };
    addInputAction(&actionDesc);
    actionDesc = { DefaultInputActions::TOGGLE_FULLSCREEN,
                   [](InputActionContext* ctx)
                   {
                       WindowDesc* winDesc = ((IApp*)ctx->pUserData)->pWindow;
                       if (winDesc->fullScreen)
                           winDesc->borderlessWindow
                               ? setBorderless(winDesc, getRectWidth(&winDesc->clientRect), getRectHeight(&winDesc->clientRect))
                               : setWindowed(winDesc, getRectWidth(&winDesc->clientRect), getRectHeight(&winDesc->clientRect));
                       else
                           setFullscreen(winDesc);
                       return true;
                   },
                   this };
    addInputAction(&actionDesc);
    actionDesc = { DefaultInputActions::EXIT, [](InputActionContext* ctx)
                   {
                       requestShutdown();
                       return true;
                   } };
    addInputAction(&actionDesc);
    InputActionCallback onAnyInput = [](InputActionContext* ctx)
    {
        if (ctx->mActionId > UISystemInputActions::UI_ACTION_START_ID_)
        {
            uiOnInput(ctx->mActionId, ctx->mBool, ctx->pPosition, &ctx->mFloat2);
        }

        return true;
    };
    static ICameraController* pCameraCtrl = pCameraController;
    typedef bool (*CameraInputHandler)(InputActionContext* ctx, DefaultInputActions::DefaultInputAction action);
    static CameraInputHandler onCameraInput = [](InputActionContext* ctx, DefaultInputActions::DefaultInputAction action)
    {
        if (*(ctx->pCaptured))
        {
            float2 delta = uiIsFocused() ? float2(0.f, 0.f) : ctx->mFloat2;
            switch (action)
            {
            case DefaultInputActions::ROTATE_CAMERA:
                pCameraCtrl->onRotate(delta);
                break;
            case DefaultInputActions::TRANSLATE_CAMERA:
                pCameraCtrl->onMove(delta);
                break;
            case DefaultInputActions::TRANSLATE_CAMERA_VERTICAL:
                pCameraCtrl->onMoveY(delta[0]);
                break;
            default:
                break;
            }
        }
        return true;
    };
    actionDesc = { DefaultInputActions::CAPTURE_INPUT,
                   [](InputActionContext* ctx)
                   {
                       setEnableCaptureInput(!uiIsFocused() && INPUT_ACTION_PHASE_CANCELED != ctx->mPhase);
                       return true;
                   },
                   NULL };
    addInputAction(&actionDesc);
    actionDesc = { DefaultInputActions::ROTATE_CAMERA,
                   [](InputActionContext* ctx) { return onCameraInput(ctx, DefaultInputActions::ROTATE_CAMERA); }, NULL };
    addInputAction(&actionDesc);
    actionDesc = { DefaultInputActions::TRANSLATE_CAMERA,
                   [](InputActionContext* ctx) { return onCameraInput(ctx, DefaultInputActions::TRANSLATE_CAMERA); }, NULL };
    addInputAction(&actionDesc);
    actionDesc = { DefaultInputActions::TRANSLATE_CAMERA_VERTICAL,
                   [](InputActionContext* ctx) { return onCameraInput(ctx, DefaultInputActions::TRANSLATE_CAMERA_VERTICAL); }, NULL };
    addInputAction(&actionDesc);
    actionDesc = { DefaultInputActions::RESET_CAMERA, [](InputActionContext* ctx)
                   {
                       if (!uiWantTextInput())
                           pCameraCtrl->resetView();
                       return true;
                   } };
    addInputAction(&actionDesc);
    GlobalInputActionDesc globalInputActionDesc = { GlobalInputActionDesc::ANY_BUTTON_ACTION, onAnyInput, this };
    setGlobalInputAction(&globalInputActionDesc);
}

bool KokkuTestApp::setupCamera()
{
    CameraMotionParameters cmp{ 160.0f, 600.0f, 200.0f };
    vec3                   camPos{ 48.0f, 48.0f, 20.0f };
    vec3                   lookAt{ vec3(0) };

    pCameraController = initFpsCameraController(camPos, lookAt);

    pCameraController->setMotionParameters(cmp);

    InputSystemDesc inputDesc = {};
    inputDesc.pRenderer = pRenderer;
    inputDesc.pWindow = pWindow;
    inputDesc.pJoystickTexture = "circlepad.tex";
    if (!initInputSystem(&inputDesc))
        return false;
    
    return true;
}
