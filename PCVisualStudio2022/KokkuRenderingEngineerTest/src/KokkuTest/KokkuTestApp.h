#pragma once

#include <Application/Interfaces/IApp.h>

class KokkuTestApp : public IApp
{
public:
    bool Init();
    void Exit();

    bool Load(ReloadDesc* pReloadDesc);
    void Unload(ReloadDesc* pReloadDesc);

    void Update(float deltaTime);
    void Draw();

    const char* GetName();
};

