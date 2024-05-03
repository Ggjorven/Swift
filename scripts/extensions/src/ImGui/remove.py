import shutil as sh
import os

def main() -> None:
    print("-----------------------------")
    print("Removing ImGui from Swift.")
    print("-----------------------------\n")

    print(f"Current working directory: {os.getcwd()}\n")

    sh.copy2("src/ImGui/Files/Original/Application.cpp", "../../Core/src/Swift/Core/Application.cpp")
    sh.copy2("src/ImGui/Files/Original/Application.hpp", "../../Core/src/Swift/Core/Application.hpp")

    sh.copy2("src/ImGui/Files/Original/Layer.hpp", "../../Core/src/Swift/Core/Layer.hpp")
    
    os.remove("../../Core/src/Swift/Utils/BaseImGuiLayer.cpp")
    os.remove("../../Core/src/Swift/Utils/BaseImGuiLayer.hpp")

    os.remove("../../Core/src/Swift/Utils/ImGuiBuild.cpp")

    os.remove("../../Core/src/Swift/Vulkan/VulkanImGuiLayer.cpp")
    os.remove("../../Core/src/Swift/Vulkan/VulkanImGuiLayer.hpp")

    print("-----------------------------")
    print("Finished removing ImGui...")
    print("-----------------------------")

    print("\nDon't forget to reload/regenerate the project.\n")

if __name__ == '__main__':
    main()