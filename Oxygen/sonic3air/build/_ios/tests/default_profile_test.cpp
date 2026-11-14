#include "sonic3air/ConfigurationImpl.h"
#include "oxygen/application/GameProfile.h"
#include <filesystem>
#include <iostream>
int main(int argc, char** argv) {
    GameProfile profile;
    ConfigurationImpl::fillDefaultGameProfile(profile);
    const auto script = std::filesystem::path(argv[1]) / "scripts" / profile.mMainScriptName;
    if (profile.mMainScriptName.empty() || !std::filesystem::is_regular_file(script)) {
        std::cerr << "Main script resolves to a directory or missing file: " << script << '\n';
        return 1;
    }
    std::cout << "Main script resolves to an existing file: " << script << '\n';
    return 0;
}
