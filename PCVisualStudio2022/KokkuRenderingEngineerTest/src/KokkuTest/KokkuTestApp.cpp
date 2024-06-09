#include "KokkuTestApp.h"

bool KokkuTestApp::Init()
{
	return true;
}

void KokkuTestApp::Exit()
{
}

bool KokkuTestApp::Load(ReloadDesc* pReloadDesc)
{
	return true;
}

void KokkuTestApp::Unload(ReloadDesc* pReloadDesc)
{
}

void KokkuTestApp::Update(float deltaTime)
{
}

void KokkuTestApp::Draw()
{
}

const char* KokkuTestApp::GetName()
{
	return "KokkuRenderingEngineerTestApp";
}
