/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "sonic3air/audio/RemasteredMusicDownload.h"
#include "sonic3air/client/GameClient.h"
#include "sonic3air/data/PlayerProgress.h"
#include "sonic3air/data/RetroAchievements.h"
#include "sonic3air/data/PlayerRecorder.h"
#include "sonic3air/data/ReplayData.h"
#include "sonic3air/data/GameSettings.h"
#include "sonic3air/helper/BlueSpheresRendering.h"
#include "sonic3air/resources/DynamicSprites.h"

#include "oxygen/resources/ResourcesCache.h"

namespace lemon
{
	class Module;
}


class Game : public SingleInstance<Game>
{
public:
	enum class Mode
	{
		UNDEFINED = 0,	// Undefined mode is used in debugging
		TITLE_SCREEN,	// SEGA logo, intro, title screen
		NORMAL_GAME,	// Normal game (started from the menu)
		ACT_SELECT,		// Act Select mode
		TIME_ATTACK,	// Time Attack mode
		COMPETITION,	// Competition mode
		BLUE_SPHERE,	// Blue Sphere game mode
		MAIN_MENU_BG	// Main menu background
	};

public:
	Game();

	void startup(EmulatorInterface& emulatorInterface);
	void shutdown();
	bool setSharedModeSaves(bool enabled);
	bool setClassicMode(bool enabled);
	struct ReplayEntry { std::wstring filename; s3air::ReplayData data; };
	std::vector<ReplayEntry> listReplays();
	bool playReplay(const std::wstring& filename);
	bool deleteReplay(const std::wstring& filename);
	bool finishReplayRecording(bool save);
	bool isPlayingReplay() const { return mReplayPlaying; }
	void stopReplay();
	bool prepareReplayForStateLoad();
	bool isClassicMode();
	void update(float timeElapsed);

	void registerScriptBindings(lemon::Module& module);

	uint32 getSetting(uint32 settingId, bool ignoreGameMode) const;
	void setSetting(uint32 settingId, uint32 value);

	void checkForUnlockedSecrets(bool saveIfAnyUnlocked);
	void unlockSecret(uint32 secretId);

	void startIntoTitleScreen();
	void startIntoDataSelect();
	void startIntoActSelect();
	void startIntoLevel(Mode mode, uint32 submode, uint16 zoneAndAct, uint8 characters);
	void restartLevel();
	void restartAtCheckpoint();
	void restartTimeAttack(bool skipFadeout);
	void startIntoCompetitionMode();
	void startIntoBlueSphere();
	void startIntoLevelSelect();
	void startIntoMainMenuBG();

	void onPreUpdateFrame();
	void onPostUpdateFrame();
	void retroEvent(uint32 event, uint32 a, uint32 b);
	void onUpdateControls();

	void updateSpecialInput(float timeElapsed);

	inline bool isInNormalGameMode() const	{ return mMode == Mode::NORMAL_GAME; }
	inline bool isInTimeAttackMode() const	{ return mMode == Mode::TIME_ATTACK; }
	inline bool isInMainMenuMode() const	{ return mMode == Mode::MAIN_MENU_BG; }
	inline void resetCurrentMode()			{ mMode = Mode::UNDEFINED; }

	inline PlayerRecorder& getPlayerRecorder()	{ return mPlayerRecorder; }

	RemasteredMusicDownload& getRemasteredMusicDownload()  { return mRemasteredMusicDownload; }

	void onActiveModsChanged();

	bool shouldPauseOnFocusLoss() const;

	void fillDebugVisualization(Bitmap& bitmap, int& mode);

	void onGameRecordingHeaderLoaded(const std::string& buildString, const std::vector<uint8>& buffer);
	void onGameRecordingHeaderSave(std::vector<uint8>& buffer);

private:
	void checkActiveModsUsedFeatures();

	void startIntoGameInternal();
	void beginReplayRecording();
	void completeReplayRecording(uint16 zoneAndAct, bool leavingZone = false);
	void captureReplayAction();
	void applyReplayActions();
	std::string replayCompatibility() const;

	// Script bindings
	uint32 useSetting(uint32 settingId);

	int32 getAchievementValue(uint32 achievementId);
	void setAchievementValue(uint32 achievementId, int32 value);
	bool isAchievementComplete(uint32 achievementId);
	void setAchievementComplete(uint32 achievementId);

	bool isSecretUnlocked(uint32 secretId);
	void setSecretUnlocked(uint32 secretId);

	void triggerRestart();
	void onGamePause(uint8 canRestart);
	void allowRestartInGamePause(uint8 canRestart);
	void onLevelStart();
	void onZoneActCompleted(uint16 zoneAndAct);
	uint16 onTriggerNextZone(uint16 zoneAndAct);
	uint16 onFadedOutLoadingZone(uint16 zoneAndAct);
	bool onCharacterDied(uint8 playerIndex);
	void returnToMainMenu();
	void openOptionsMenu();

	inline bool isNormalGame()	{ return isInNormalGameMode(); }
	inline bool isTimeAttack()	{ return isInTimeAttackMode(); }
	bool onTimeAttackFinish();

	void changePlanePatternRectAtex(uint16 px, uint16 py, uint16 width, uint16 height, uint8 planeIndex, uint8 atex);

	void setupBlueSpheresGroundSprites();
	void writeBlueSpheresData(uint32 targetAddress, uint32 sourceAddress, uint16 px, uint16 py, uint8 rotation);

	void startSkippableCutscene();
	void endSkippableCutscene();
	bool isInSkippableCutscene();

private:
	s3air::ReplayData mReplay;
	bool mReplayPlaying = false;
	bool mReplayRecording = false;
	bool mReplayStartPending = false;
	bool mReplaySnapshotPending = false;
	size_t mReplayAction = 0;
	size_t mReplaySnapshotBytes = 0;
	bool mReplayFinishPending = false;
	bool mReplayPromptPending = false;
	size_t mReplayFrame = 0;
	size_t mReplaySettingEvent = 0;
	std::unordered_map<uint32, uint32> mReplayCurrentSettings;
	bool mReplayOldClassic = false;
	bool mReplayOldSpeedrun = false;
	uint32 mReplayCutsceneFrames = 0;
	GameSettings* mReplayOldSettings = nullptr;
	bool mSpeedrunCutsceneActive = false;
	EmulatorInterface* mEmulatorInterface = nullptr;

	retroachievements::Tracker mRetroAchievements;
	Mode mMode = Mode::UNDEFINED;
	uint32 mSubMode = 0;

	BlueSpheresRendering mBlueSpheresRendering;
	PlayerProgress mPlayerProgress;
	void loadModeProfile();
	std::wstring mAnniversarySaveStatesDir;
	std::wstring mAnniversarySaveStatesDirLocal;
	PlayerRecorder mPlayerRecorder;
	GameClient mGameClient;
	DynamicSprites mDynamicSprites;
	RemasteredMusicDownload mRemasteredMusicDownload;

	uint16 mLastZoneAndAct = 0;
	uint8  mLastCharacters = 0;

	bool mReceivedTimeAttackFinished = false;
	bool mReturnToMenuTriggered = false;
	bool mRestartTriggered = false;
	bool mRestartingTimeAttack = false;
	bool mAllowRestartInGamePause = false;

	uint32 mSkippableCutsceneFrames = 0;	// If > 0, a skippable cutscene is active
	bool mButtonYPressedDuringSkippableCutscene = false;

	bool mTimeAttackRestartCharging = false;
	float mTimeAttackRestartCharge = 0.0f;

	float mTimeoutUntilDiscordRefresh = 1.0f;
};
