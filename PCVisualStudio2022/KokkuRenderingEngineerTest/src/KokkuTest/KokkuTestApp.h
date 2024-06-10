#pragma once

#include <Application/Interfaces/IApp.h>
class KokkuTestApp : public IApp
{
private:
   
public:
    bool Init();
    void Exit();

    bool Load(ReloadDesc* pReloadDesc);
    void Unload(ReloadDesc* pReloadDesc);

    void Update(float deltaTime);
    void Draw();

    const char* GetName();



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
};

