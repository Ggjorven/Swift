import shutil as sh
import os

def main() -> None:
    print("-----------------------------")
    print("Removing Assimp from Swift...")
    print("-----------------------------\n")

    print(f"Current working directory: {os.getcwd()}\n")

    os.remove("../../Core/src/Swift/Utils/Mesh.cpp")
    os.remove("../../Core/src/Swift/Utils/Mesh.hpp")

    sh.copy2("src/Assimp/Files/Core/original.lua", "../../Core/premake5.lua")
    sh.copy2("src/Assimp/Files/Sandbox/original.lua", "../../Sandbox/premake5.lua")

    print("-----------------------------")
    print("Finished removing Assimp.")
    print("-----------------------------")

if __name__ == '__main__':
    main()