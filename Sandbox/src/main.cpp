#include <Swift/Core/Application.hpp>
#include <Swift/Entrypoint.hpp>

class Sandbox : public Swift::Application
{
public:
	Sandbox(const Swift::ApplicationSpecification& appInfo)
		: Swift::Application(appInfo)
	{
	}
};



// ----------------------------------------------------------------
//                    Set Application specs here...
// ----------------------------------------------------------------
Swift::Application* Swift::CreateApplication(int argc, char* argv[])
{
	ApplicationSpecification appInfo = {};
	appInfo.WindowSpecs.Name = "Custom";
	appInfo.WindowSpecs.VSync = false;

	return new Sandbox(appInfo);
}