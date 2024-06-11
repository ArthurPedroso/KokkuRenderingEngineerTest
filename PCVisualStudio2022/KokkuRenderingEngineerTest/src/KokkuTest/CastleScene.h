#pragma once
#include <Graphics/Interfaces/IGraphics.h>
#include <Resources/ResourceLoader/Interfaces/IResourceLoader.h>

// Type definitions

typedef uint32_t MaterialFlags;

enum MaterialFlagBits
{
    MATERIAL_FLAG_NONE = 0,
    MATERIAL_FLAG_TWO_SIDED = (1 << 0),
    MATERIAL_FLAG_ALPHA_TESTED = (1 << 1),
    MATERIAL_FLAG_TRANSPARENT = (1 << 2),
    MATERIAL_FLAG_DOUBLE_VOXEL_SIZE = (1 << 3),
    MATERIAL_FLAG_ALL = MATERIAL_FLAG_TWO_SIDED | MATERIAL_FLAG_ALPHA_TESTED | MATERIAL_FLAG_DOUBLE_VOXEL_SIZE
};

class CastleScene
{
private:
    Geometry* geom;
    GeometryData* geomData;
    MaterialFlags* materialFlags;
    char** textures;
    char** normalMaps;
    char** specularMaps;
    uint32_t       materialCount;

    void setTextures(int index, const char* albedo, const char* specular, const char* normal, uint32_t matFlags);
    void setDefaultTextures(int index);
    void SetMaterials(bool transparentFlags);

public:
    Geometry* getGeometry() { return geom; }
    MaterialFlags* getMatFlags() { return materialFlags; }

    void createCubeBuffers(Renderer* pRenderer, Buffer** outVertexBuffer, Buffer** outIndexBuffer);
    void Load(const GeometryLoadDesc* pTemplate, bool transparentFlags);
    void Unload();
};

