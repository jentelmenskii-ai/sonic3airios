// Native, offline adaptation; see RetroAchievementCatalog.h for attribution.
#pragma once
#include "RetroAchievementCatalog.h"

namespace retroachievements
{
// Values are shared with Game.retroEvent calls in the scripts.
enum Event : unsigned { FRAME, LEVEL_START, ACT_FINISH, DAMAGE, DEATH, BONUS_ENTER,
    GAME_FINISH, SPECIAL_START, SPECIAL_FRAME, SPECIAL_WIN, JACKPOT_EVENT,
    GLOW_EXIT_EVENT, SHIELD_CHANGE, LIFE_MONITOR, HPZ_WIN, DEZ_ROBO, COMPETITION_FINISH,
    CHEAT_EVENT, PAUSE, INVALIDATE };
struct Snapshot
{
    unsigned zoneAct = 0, rings = 0, lives = 0, time = 0, character = 0;
    unsigned chaos = 0, superEmeralds = 0, shield = 0, checkpoint = 0;
    unsigned x = 0, y = 0, cameraY = 0, input = 0, blueRemaining = 0, ringsRemaining = 0;
    bool transformed = false, active = false, countersActive = false, normal = false, competition = false;
};

class Tracker
{
public:
    template<class Award>
    void process(Event event, const Snapshot& s, unsigned a, unsigned b, Award award)
    {
        const auto unlock = [&](Index index) {
            const uint64_t bit = uint64_t(1) << index;
            if ((mEmitted & bit) == 0) { mEmitted |= bit; award(ID_BASE + index); }
        };
        if (event == CHEAT_EVENT)
        {
            unlock(CHEAT); mBlocked = true; invalidateAttempts(); return;
        }
        if (event == INVALIDATE) { invalidateAttempts(); mSpecial = false; return; }
        if (mBlocked || (!s.normal && !s.competition)) return;
        if (event == COMPETITION_FINISH)
        {
            if (!s.competition || a < 0x0e || a > 0x12 || b == 0) return;
            // ROM order is Azure, Balloon, Desert, Chrome, Endless.
            constexpr Index ids[] = {AZURE, BALLOON, DESERT, CHROME, MINE};
            constexpr unsigned limits[] = {3000, 5000, 4000, 4500, 6000};
            const unsigned track = a - 0x0e;
            if (b < limits[track]) unlock(ids[track]);
            if (s.character == 0) // Grand Prix, as opposed to Match Race / Time Attack
            {
                if (track == 0) { mGPTracks = 0; mGPTime = 0; }
                const unsigned bit = 1u << track;
                if ((mGPTracks & bit) == 0) { mGPTime += b; mGPTracks |= bit; }
                if (mGPTracks == 31 && mGPTime < 24000) unlock(GRAND_PRIX);
            }
            return;
        }
        if (!s.normal) return;
        if (event == LEVEL_START)
        {
            // Returning from a bonus or special stage must not restart a challenge.
            const bool continuation = mAway && mAct == s.zoneAct;
            if (!continuation)
            {
                mAct = s.zoneAct;
                mAttempt = s.checkpoint == 0 && s.time == 0;
                mNoLeft = mAttempt; mNoDamage = mAttempt;
                if (s.zoneAct == 0x0b00) mDeathless = mAttempt;
                else if (s.zoneAct != 0x0b01 && s.zoneAct != 0x1700) mDeathless = false;
            }
            mAway = false; mLastLives = s.lives; mHaveLives = true;
            return;
        }
        if (event == DAMAGE) mNoDamage = false;
        if (event == DEATH) { invalidateAttempts(); mSpecial = false; }
        if (event == BONUS_ENTER)
        {
            mAway = true; mNoDamage = false; mDeathless = false;
            mShieldChanges = 0; mLastShield = s.shield & 0x70;
        }
        if (event == PAUSE) mBackwardsValid = false;
        if (event == FRAME && s.countersActive)
        {
            // Bonus stages can grant rings and extra lives too. Their counters
            // remain valid, while their coordinates are excluded below.
            if (s.rings >= 100) unlock(RINGS100);
            if (s.rings >= 200) unlock(RINGS200);
            if (mHaveLives && s.lives > mLastLives)
            {
                if (s.lives == 10) unlock(LIVES10);
                if (s.lives == 20) unlock(LIVES20);
            }
            mLastLives = s.lives; mHaveLives = true;
        }
        if (event == FRAME && s.active)
        {
            if (s.input & 4) mNoLeft = false; // Raw player input, including control overrides
            if (s.transformed) { mNoDamage = false; mDeathless = false; }
            if (s.zoneAct == 0x0c00 && s.rings >= 100) unlock(DDZ_RINGS);
            // World bounds from the original S3&K ring/object placements. Camera
            // comparisons use signed deltas so vertical wrapping cannot award MHZ_TOP.
            if ((s.zoneAct >> 8) == 7 && s.cameraY == 0 && s.y >= 0xffe0) unlock(MHZ_TOP);
            if (s.zoneAct == 0x0100 && s.x >= 0x2560 && s.x <= 0x2748 && s.y >= 0x308 && s.y <= 0x398) unlock(LOAN);
            if (s.zoneAct == 0x0501 && s.x >= 0x3f90 && s.x <= 0x4170 &&
                ((s.y >= 0x20 && s.y <= 0x110) || (s.y >= 0x350 && s.y <= 0x460))) unlock(FROZEN);
        }
        if (event == ACT_FINISH)
        {
            if (s.transformed) { mNoDamage = false; mDeathless = false; }
            const unsigned zone = a >> 8;
            if (a & 1)
            {
                switch (zone)
                {
                    case 0: unlock(AIZ); break; case 1: unlock(HCZ); break;
                    case 2: unlock(MGZ); break; case 3: unlock(CNZ); break;
                    case 5: unlock(ICZ); break; case 6: unlock(LBZ); break;
                    case 7: unlock(MHZ); break; case 4: unlock(FBZ); break;
                    case 8: unlock(SOZ); break;
                    case 11: unlock(DEZ); break;
                    case 9: if ((s.character == 3 || s.character == 4)) unlock(LRZ); break;
                    default: break;
                }
            }
            if (a == 0x1600) { unlock(LRZ); if (s.character != 3 && s.character != 4 && (s.shield & 0x20)) unlock(LRZ_LIGHTNING); }
            if (a == 0x0a00 && s.character != 3 && s.character != 4) unlock(SSZ);
            if (a == 0x0000 && s.time < 120 * 60) unlock(AIZ_FAST);
            if (a == 0x0601 && s.rings >= 100) unlock(LBZ_RINGS);
            if (a == 0x0700 && s.time < 180 * 60 && (s.shield & 0x40)) unlock(MHZ_BUBBLE);
            if (a == 0x0801 && (s.shield & 0x10)) unlock(SOZ_FIRE);
            if (s.time >= 599 * 60 && s.time < 600 * 60) unlock(LAST_SECOND);
            if (zone == 6 && mNoLeft && mAct == a) unlock(NO_LEFT);
            if (s.rings >= 400) unlock(RINGS400);
            if (a == 0x0200 && mNoDamage && mAct == a) unlock(NO_DAMAGE);
        }
        if (event == GAME_FINISH)
        {
            if (s.character == 1 || s.character == 0)
            { unlock(SONIC_END); if (s.superEmeralds == 7) unlock(SONIC_EMERALD_END); }
            if (s.character == 3 || s.character == 4)
            { unlock(KNUCKLES_END); if (s.superEmeralds == 7) unlock(KNUCKLES_EMERALD_END); }
            if (s.character == 2)
            { unlock(TAILS_END); if (s.superEmeralds == 7) unlock(TAILS_EMERALD_END); }
            if (s.character != 3 && s.character != 4) unlock(DEZ);
        }
        if (event == HPZ_WIN && s.character != 3 && s.character != 4) unlock(HPZ);
        if (event == DEZ_ROBO && mDeathless && !s.transformed) unlock(DEATHLESS);
        if (event == LIFE_MONITOR && s.zoneAct == 0x0001 &&
            ((a == 0x1ba8 || a == 0x2f6e) && b == 0x203)) unlock(HIDDEN_LIFE);
        if (event == JACKPOT_EVENT) unlock(JACKPOT);
        if (event == GLOW_EXIT_EVENT) unlock(GLOW_EXIT);
        if (event == SHIELD_CHANGE)
        {
            if (a != 0 && a != mLastShield)
            {
                ++mShieldChanges;
                if (mShieldChanges >= 5) unlock(SHIELDS);
            }
            mLastShield = a;
        }
        if (event == SPECIAL_START)
        {
            mSpecial = true; mBackwardsValid = a == 0 && b == 0;
            mInitialBlue = s.blueRemaining; mSeenBackwards = false;
        }
        if (event == SPECIAL_FRAME && mSpecial)
        {
            const int speed = static_cast<int16_t>(a);
            if (speed < 0) mSeenBackwards = true;
            if (speed > 0 && (mSeenBackwards || s.blueRemaining < mInitialBlue)) mBackwardsValid = false;
        }
        if (event == SPECIAL_WIN)
        {
            if (s.ringsRemaining == 0)
            {
                unlock(PERFECT);
                if (mSpecial && mBackwardsValid && mSeenBackwards) unlock(BACKWARDS);
            }
            if (b == 0) { unlock(EMERALD); if (s.chaos == 7) unlock(CHAOS); }
            else if (s.superEmeralds == 7) unlock(SUPER);
            mSpecial = false;
        }
    }
    void invalidateAttempts() { mAttempt = mNoLeft = mNoDamage = mDeathless = false; mBackwardsValid = false; }
private:
    uint64_t mEmitted = 0;
    unsigned mAct = 0xffff, mLastLives = 0, mInitialBlue = 0, mShieldChanges = 0, mLastShield = 0;
    unsigned mGPTracks = 0, mGPTime = 0;
    bool mBlocked = false, mAttempt = false, mNoLeft = false, mNoDamage = false, mDeathless = false;
    bool mHaveLives = false, mAway = false, mSpecial = false, mBackwardsValid = false, mSeenBackwards = false;
};
}
