#include "sonic3air/data/ClassicModeRules.h"
#include "sonic3air/data/PlayerProgress.h"
#include "sonic3air/data/RetroAchievementCatalog.h"
#include "oxygen/simulation/PersistentData.h"
#include "oxygen/helper/ScaledScreenRect.h"
#include "sonic3air/scriptimpl/ScrollBuffer.h"
#include <array>
#include <filesystem>
#include <cassert>
#include <iostream>

int main(int argc, char** argv)
{
    assert(argc == 2);
    FTX::FileSystem.createDefault();
    const std::wstring root = std::filesystem::path(argv[1]).wstring() + L"/";
    const auto normal = ClassicModeRules::saveRoot(root, false);
    const auto classic = ClassicModeRules::saveRoot(root, true);
    assert(normal == root && classic != normal);
    assert(ClassicModeRules::gameSize(true) == Vec2i(320,224));
    assert(ClassicModeRules::gameSize(false) == Vec2i(400,224));
    using S = SharedDatabase::Setting;
    uint32 value = 99;
    for (uint32 key : {S::SETTING_SMOOTH_ROTATION, S::SETTING_EXTENDED_HUD,
                      S::SETTING_SPEEDUP_AFTERIMGS, S::SETTING_SUPERFAST_RUNANIM,
                      S::SETTING_GFX_ANTIFLICKER}) {
        assert(ClassicModeRules::forcedSetting(key, value) && value == 0);
    }
    assert(ClassicModeRules::forcedSetting(S::SETTING_LIVES_DISPLAY, value) && value == 2);
    assert(!ClassicModeRules::forcedSetting(S::SETTING_INFINITE_LIVES, value));

    // Exercise the actual persistent storage, including a process-style reload.
    PersistentData data;
    const uint64 file = rmx::getMurmur2_64("s3air_savedata");
    const uint64 key = rmx::getMurmur2_64("slot1");
    data.loadFromBasePath(normal + L"storage/");
    data.setData("s3air_savedata", "slot1", {1,2,3});
    data.updatePersistentData();
    data.loadFromBasePath(classic + L"storage/");
    assert(data.getData(file, key).empty());
    data.setData("s3air_savedata", "slot1", {7,8,9});
    data.updatePersistentData();
    for (int repeat = 0; repeat < 3; ++repeat) {
        data.loadFromBasePath(normal + L"storage/");
        assert(data.getData(file, key) == std::vector<uint8>({1,2,3}));
        data.loadFromBasePath(classic + L"storage/");
        assert(data.getData(file, key) == std::vector<uint8>({7,8,9}));
    }
    // Clearing a profile must also discard pending writes, even for matching hashes.
    data.setData("s3air_savedata", "slot1", {99});
    data.clear();
    data.loadFromBasePath(normal + L"storage/");
    data.updatePersistentData();
    assert(data.getData(file, key) == std::vector<uint8>({1,2,3}));
    assert(std::filesystem::is_regular_file(normal + L"storage/s3air_savedata.bin"));
    assert(std::filesystem::is_regular_file(classic + L"storage/s3air_savedata.bin"));

    // Shared mode routes both editions to Anniversary; disabling it restores Classic.
    for (bool isClassic : {false, true}) {
        const auto shared = ClassicModeRules::saveRoot(root, isClassic, true);
        assert(shared == normal);
        data.loadFromBasePath(shared + L"storage/");
        assert(data.getData(file, key) == std::vector<uint8>({1,2,3}));
    }
    data.loadFromBasePath(ClassicModeRules::saveRoot(root, true, false) + L"storage/");
    assert(data.getData(file, key) == std::vector<uint8>({7,8,9}));
    // A replay may read its own writes, including deletion, without persisting them.
    data.setTransient(true);
    data.setData("s3air_savedata", "slot1", {42});
    assert(data.getData(file, key) == std::vector<uint8>({42}));
    data.updatePersistentData();
    data.removeKey(file, key);
    assert(data.getData(file, key).empty());
    data.updatePersistentData();
    data.clear();
    data.setTransient(false);
    data.loadFromBasePath(classic + L"storage/");
    assert(data.getData(file, key) == std::vector<uint8>({7,8,9}));
    data.updatePersistentData();

    // Serialize the real achievement map, including imported IDs above 16 bits.
    detail::PlayerAchievementsData anniversaryAchievements, classicAchievements;
    anniversaryAchievements.mAchievementStates[0x0001] = 1; // Existing AIR unlock
    anniversaryAchievements.mAchievementStates[retroachievements::ID_BASE] = 1;
    classicAchievements.mAchievementStates[retroachievements::ID_BASE + retroachievements::CHEAT] = 1;
    const auto storeAchievements = [&](const std::wstring& path, detail::PlayerAchievementsData& achievements) {
        std::vector<uint8> buffer;
        VectorBinarySerializer serializer(false, buffer);
        achievements.serializeAchievements(serializer);
        data.loadFromBasePath(path + L"storage/");
        data.setData("s3air_achievements", "Achievements", buffer);
        data.updatePersistentData();
    };
    storeAchievements(normal, anniversaryAchievements);
    storeAchievements(classic, classicAchievements);
    for (int repeat=0; repeat<3; ++repeat) {
        for (bool isClassic : {false,true}) {
            data.loadFromBasePath((isClassic ? classic : normal) + L"storage/");
            const auto& buffer = data.getData(rmx::getMurmur2_64("s3air_achievements"), rmx::getMurmur2_64("Achievements"));
            VectorBinarySerializer reader(true, buffer);
            detail::PlayerAchievementsData reloaded;
            reloaded.serializeAchievements(reader);
            assert(reloaded.isEqual(isClassic ? classicAchievements : anniversaryAchievements));
            assert(reloaded.getAchievementState(0x0001) == (isClassic ? 0 : 1));
            assert(reloaded.getAchievementState(retroachievements::ID_BASE) == (isClassic ? 0 : 1));
            assert(reloaded.getAchievementState(retroachievements::ID_BASE + retroachievements::CHEAT) == (isClassic ? 1 : 0));
        }
    }

    // 224 scanlines end exactly where the dynamic-object list begins.
    struct Memory {
        std::array<uint8, 65536> bytes{};
        void writeMemory32(uint32 address, uint32 value) {
            const uint32 index = address & 0xffff;
            assert(index + 4 <= bytes.size());
            for (int i=0; i<4; ++i) bytes[index+i] = uint8(value >> (24 - 8*i));
        }
    } memory;
    for (int requested : {-1, 0, 224, 240, 300, 1024}) {
        memory.bytes.fill(0xcc);
        s3air::writeLegacyScrollOffsets(memory, 0x12345678, requested);
        for (int i=0xe380; i<0xe400; ++i) assert(memory.bytes[i] == 0xcc);
        if (requested >= 224) assert(memory.bytes[0xe37f] == 0x78);
    }
    for (Vec2i requested : {Vec2i(320,240), Vec2i(320,300), Vec2i(400,300), Vec2i(640,480)}) {
        assert(ClassicScreenLayout::constrain(true, requested).y == 224);
        assert(ClassicScreenLayout::constrain(false, requested) == requested);
    }
    assert(ClassicScreenLayout::compositionSize(true, Vec2i(400,224)) == Vec2i(320,224));
    assert(ClassicScreenLayout::compositionSize(true, Vec2i(320,224)) == Vec2i(320,224));
    assert(ClassicScreenLayout::compositionSize(true, Vec2i(320,224), true) == Vec2i(320,224));

    // Every Classic screen has the same physical 4:3 frame, including menus.
    for (Vec2i source : {Vec2i(320,224), Vec2i(400,224)}) {
        const Vec2i composed = ClassicScreenLayout::compositionSize(true, source);
        ScaledScreenRect viewport;
        viewport.setResolution(composed);
        viewport.buildRectAspectFit(Recti(0,0,1920,1080));
        viewport.setRectOnScreen(Recti(240,0,1440,1080));
        const Recti rect = viewport.getRectOnScreen();
        assert(rect.width == 1440 && rect.height == 1080);
        Vec2f center = viewport.getInnerPositionFromScreen(Vec2f(rect.x+rect.width*.5f, rect.y+rect.height*.5f));
        assert(std::abs(center.x - composed.x*.5f) < .01f);
        assert(std::abs(center.y - composed.y*.5f) < .01f);
        if (source.x == 400) {
            assert(ClassicScreenLayout::menuLeftShift(true) == 80);
            assert(ClassicScreenLayout::menuLeftShift(false) == 0);
            assert(218 - ClassicScreenLayout::menuLeftShift(true) == 138);
            assert(188 + 13 < composed.y); // Original row spacing and visible mode toggle.
        }
        const Vec2f point(100, 80);
        const auto restored = viewport.getInnerPositionFromScreen(viewport.getScreenPositionFromInner(point));
        assert(std::abs(point.x-restored.x) < .01f && std::abs(point.y-restored.y) < .01f);
    }
    std::cout << "Classic tests passed: real save isolation, fixed presets, scroll/object memory boundary, safe 224-line fields, compact 224-line menus and input mapping\n";
}
