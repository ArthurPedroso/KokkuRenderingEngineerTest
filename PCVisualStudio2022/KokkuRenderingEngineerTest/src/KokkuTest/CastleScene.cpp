#include "CastleScene.h"


#define DEFAULT_ALBEDO           "Default"
#define DEFAULT_NORMAL           "Default_NRM"
#define DEFAULT_SPEC             "Default_SPEC"
#define DEFAULT_SPEC_TRANSPARENT "Default_SPEC_TRANS"

/* TODO RM
void CastleScene::setTextures(int index, const char* albedo, const char* specular, const char* normal, uint32_t matFlags)
{
    materialFlags[index] = matFlags;

    // default textures
    textures[index] = (char*)tf_calloc(strlen(albedo) + 5, sizeof(char));
    specularMaps[index] = (char*)tf_calloc(strlen(specular) + 5, sizeof(char));
    normalMaps[index] = (char*)tf_calloc(strlen(normal) + 5, sizeof(char));

    snprintf(textures[index], strlen(albedo) + 5, "%s.tex", albedo);
    snprintf(specularMaps[index], strlen(specular) + 5, "%s.tex", specular);
    snprintf(normalMaps[index], strlen(normal) + 5, "%s.tex", normal);
}

void CastleScene::setDefaultTextures(int index)
{
    setTextures(index, DEFAULT_ALBEDO, DEFAULT_SPEC, DEFAULT_NORMAL, MATERIAL_FLAG_NONE);
}

void CastleScene::SetMaterials(bool transparentFlags)
{
    int index = 0;

    //setTextures(index++, "ForgeFlags", DEFAULT_SPEC, DEFAULT_NORMAL, MATERIAL_FLAG_TWO_SIDED | MATERIAL_FLAG_ALPHA_TESTED);
}
*/
void CastleScene::createCubeBuffers(Renderer* pRenderer, Buffer** outVertexBuffer, Buffer** outIndexBuffer)
{
    UNREF_PARAM(pRenderer);
    // Create vertex buffer
    static float vertexData[] = {
        -1, -1, -1, 1, 1, -1, -1, 1, 1, 1, -1, 1, -1, 1, -1, 1, -1, -1, 1, 1, 1, -1, 1, 1, 1, 1, 1, 1, -1, 1, 1, 1,
    };

    BufferLoadDesc vbDesc = {};
    vbDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_VERTEX_BUFFER;
    vbDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
    vbDesc.mDesc.mSize = sizeof(vertexData);
    vbDesc.pData = vertexData;
    vbDesc.ppBuffer = outVertexBuffer;
    vbDesc.mDesc.pName = "VB Desc";
    addResource(&vbDesc, NULL);

    // Create index buffer
    static constexpr uint16_t indices[6 * 6] = { 0, 1, 3, 3, 1, 2, 1, 5, 2, 2, 5, 6, 5, 4, 6, 6, 4, 7,
                                                 4, 0, 7, 7, 0, 3, 3, 2, 7, 7, 2, 6, 4, 5, 0, 0, 5, 1 };

    BufferLoadDesc ibDesc = {};
    ibDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_INDEX_BUFFER;
    ibDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
    ibDesc.mDesc.mSize = sizeof(indices);
    ibDesc.pData = indices;
    ibDesc.ppBuffer = outIndexBuffer;
    ibDesc.mDesc.pName = "IB Desc";
    addResource(&ibDesc, NULL);
}

void CastleScene::Load(const GeometryLoadDesc* pTemplate, bool transparentFlags)
{
    GeometryLoadDesc loadDesc = *pTemplate;

    VertexLayout vertexLayout = {};

    vertexLayout.mAttribCount = 3;
    vertexLayout.mBindingCount = 3;
    vertexLayout.mAttribs[0].mSemantic = SEMANTIC_POSITION;
    vertexLayout.mAttribs[0].mFormat = TinyImageFormat_R32G32B32_SFLOAT;
    vertexLayout.mAttribs[0].mBinding = 0;
    vertexLayout.mAttribs[0].mLocation = 0;
    vertexLayout.mAttribs[1].mSemantic = SEMANTIC_NORMAL;
    vertexLayout.mAttribs[1].mFormat = TinyImageFormat_R16G16_UNORM;
    vertexLayout.mAttribs[1].mBinding = 1;
    vertexLayout.mAttribs[1].mLocation = 1;
    vertexLayout.mAttribs[2].mSemantic = SEMANTIC_TEXCOORD0;
    vertexLayout.mAttribs[2].mFormat = TinyImageFormat_R16G16_SFLOAT;
    vertexLayout.mAttribs[2].mBinding = 2;
    vertexLayout.mAttribs[2].mLocation = 2;

    loadDesc.pVertexLayout = &vertexLayout;

    loadDesc.pFileName = "castle.bin";
    loadDesc.ppGeometryData = &geomData;
    loadDesc.ppGeometry = &geom;

    addResource(&loadDesc, NULL);

    //waitForToken(&token);
    waitForAllResourceLoads();

}

void CastleScene::Unload()
{
}
