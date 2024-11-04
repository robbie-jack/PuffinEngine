#include "puffin/core/application.h"

puffin::core::Application::Application(std::string name, std::shared_ptr<Engine> engine)
	: mName(std::move(name)), mEngine(std::move(engine))
{
}

puffin::core::Application::~Application()
{
	mEngine = nullptr;
}

void puffin::core::Application::RegisterTypes()
{
}

void puffin::core::Application::Initialize()
{
}

void puffin::core::Application::Deinitialize()
{
}

void puffin::core::Application::BeginPlay()
{
}

void puffin::core::Application::EndPlay()
{
}

void puffin::core::Application::Update(double deltaTime)
{
}

bool puffin::core::Application::ShouldUpdate()
{
	return false;
}

void puffin::core::Application::FixedUpdate(double fixedTimeStep)
{
}

bool puffin::core::Application::ShouldFixedUpdate()
{
	return false;
}

void puffin::core::Application::EngineUpdate(double deltaTime)
{
}

bool puffin::core::Application::ShouldEngineUpdate()
{
	return false;
}

const std::string& puffin::core::Application::GetName() const
{
	return mName;
}
