#include "CastleScene.h"

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
    removeResource(geom);
    removeResource(geomData);
}
