#pragma once
#include <Graphics/Interfaces/IGraphics.h>
#include <Resources/ResourceLoader/Interfaces/IResourceLoader.h>

// Type definitions

class CastleScene
{
private:
    Geometry* geom;
    GeometryData* geomData;

public:
    Geometry* getGeometry() { return geom; }

    void Load(const GeometryLoadDesc* pTemplate, bool transparentFlags);
    void Unload();
};

