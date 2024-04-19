#include <VkOutline/Core/Application.hpp>
#include <VkOutline/Entrypoint.hpp>

class Sandbox : public VkOutline::Application
{
public:
	Sandbox(const VkOutline::ApplicationSpecification& appInfo)
		: VkOutline::Application(appInfo)
	{
	}
};



// ----------------------------------------------------------------
//                    Set Application specs here...
// ----------------------------------------------------------------
VkOutline::Application* VkOutline::CreateApplication(int argc, char* argv[])
{
	ApplicationSpecification appInfo = {};
	appInfo.WindowSpecs.Name = "Custom";
	appInfo.WindowSpecs.VSync = false;

	return new Sandbox(appInfo);
}