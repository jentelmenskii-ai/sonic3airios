#!/usr/bin/env bash
set -euo pipefail
repo_root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../../../../.." && pwd)
test_build_dir=$(mktemp -d)
trap 'rm -rf "$test_build_dir"' EXIT
mkdir -p "$test_build_dir/include"
ln -s "$repo_root/framework/external/sdl/SDL2/include" "$test_build_dir/include/SDL2"
"${CXX:-c++}" -std=c++17 -ffunction-sections -fdata-sections \
  -I "$repo_root/librmx/source" -I "$test_build_dir/include" \
  -I "$repo_root/Oxygen/oxygenengine/source" -I "$repo_root/Oxygen/lemonscript/source" \
  -I "$repo_root/Oxygen/sonic3air/source" \
  -I "$repo_root/framework/external/ogg-vorbis/libogg/include" \
  -I "$repo_root/framework/external/ogg-vorbis/libvorbis/include" \
  -I "$repo_root/framework/external/zlib/zlib/contrib/minizip" \
  "$repo_root/Oxygen/sonic3air/build/_ios/tests/default_profile_test.cpp" \
  "$repo_root/Oxygen/sonic3air/source/sonic3air/ConfigurationImpl.cpp" \
  "$repo_root/Oxygen/lemonscript/source/lemon/utility/FlyweightString.cpp" \
  "$repo_root/librmx/source/rmxbase/memory/OneTimeAllocPool.cpp" \
  -Wl,--gc-sections -o "$test_build_dir/default_profile_test"
"$test_build_dir/default_profile_test" "$repo_root/Oxygen/sonic3air"
"${CXX:-c++}" -std=c++17 -I "$repo_root/librmx/source" \
  "$repo_root/Oxygen/sonic3air/build/_ios/tests/window_pixel_mapping_test.cpp" \
  -o "$test_build_dir/window_pixel_mapping_test"
"$test_build_dir/window_pixel_mapping_test"

shopt -s globstar
base_sources=("$repo_root"/librmx/source/rmxbase/**/*.cpp)
"${CXX:-c++}" -std=c++17 -ffunction-sections -fdata-sections \
  -I "$repo_root/librmx/source" -I "$test_build_dir/include" \
  -I "$repo_root/Oxygen/oxygenengine/source" -I "$repo_root/Oxygen/lemonscript/source" \
  -I "$repo_root/Oxygen/sonic3air/source" \
  -I "$repo_root/framework/external/ogg-vorbis/libogg/include" \
  -I "$repo_root/framework/external/ogg-vorbis/libvorbis/include" \
  -I "$repo_root/framework/external/zlib/zlib/contrib/minizip" \
  "$repo_root/Oxygen/sonic3air/build/_ios/tests/classic_profile_test.cpp" \
  "$repo_root/Oxygen/sonic3air/source/sonic3air/data/PlayerProgress.cpp" \
  "$repo_root/Oxygen/oxygenengine/source/oxygen/simulation/PersistentData.cpp" \
  "$repo_root/Oxygen/oxygenengine/source/oxygen/helper/ScaledScreenRect.cpp" \
  "$repo_root/Oxygen/oxygenengine/source/oxygen/rendering/utils/RenderUtils.cpp" \
  "$repo_root/Oxygen/lemonscript/source/lemon/utility/FlyweightString.cpp" \
  "${base_sources[@]}" -lz -Wl,--gc-sections -o "$test_build_dir/classic_profile_test"
"$test_build_dir/classic_profile_test" "$test_build_dir/profiles"

python3 "$repo_root/Oxygen/sonic3air/build/_ios/tests/compact_menu_test.py" "$repo_root"

"${CXX:-c++}" -std=c++17 -I "$repo_root/Oxygen/sonic3air/source" \
  "$repo_root/Oxygen/sonic3air/build/_ios/tests/retro_achievements_test.cpp" \
  -o "$test_build_dir/retro_achievements_test"
"$test_build_dir/retro_achievements_test"

"${CXX:-c++}" -std=c++17 -I "$repo_root/Oxygen/sonic3air/source" \
  "$repo_root/Oxygen/sonic3air/build/_ios/tests/replay_data_test.cpp" \
  -o "$test_build_dir/replay_data_test"
"$test_build_dir/replay_data_test"
