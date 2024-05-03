import shutil as sh
import os

def main() -> None:
    print("-----------------------------")
    print("Installing Assimp into Swift.")
    print("-----------------------------\n")

    print(f"Current working directory: {os.getcwd()}\n")

    sh.copy2("src/Assimp/Files/Mesh.cpp", "../../Core/src/Swift/Utils/Mesh.cpp")
    sh.copy2("src/Assimp/Files/Mesh.hpp", "../../Core/src/Swift/Utils/Mesh.hpp")

    sh.copy2("src/Assimp/Files/Core/premake5.lua", "../../Core/premake5.lua")
    sh.copy2("src/Assimp/Files/Sandbox/premake5.lua", "../../Sandbox/premake5.lua")

    print("-----------------------------")
    print("Finished installing Assimp...")
    print("-----------------------------")

if __name__ == '__main__':
    main()