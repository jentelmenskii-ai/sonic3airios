/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "sonic3air/pch.h"
#include "sonic3air/Game.h"
#include "sonic3air/ConfigurationImpl.h"
#include "sonic3air/audio/AudioOut.h"
#include "sonic3air/data/SharedDatabase.h"
#include "sonic3air/data/TimeAttackData.h"
#include "sonic3air/data/ClassicModeRules.h"
#include "oxygen/simulation/PersistentData.h"
#include "sonic3air/helper/GameUtils.h"
#include "sonic3air/helper/DiscordIntegration.h"
#include "sonic3air/menu/GameApp.h"
#include "sonic3air/menu/SharedResources.h"
#include "sonic3air/scriptimpl/ScriptImplementations.h"

#include "oxygen/application/Application.h"
#include "oxygen/application/Configuration.h"
#include "oxygen/application/gameview/GameView.h"
#include "oxygen/application/input/ControlsIn.h"
#include "oxygen/application/input/InputManager.h"
#include "oxygen/application/video/VideoOut.h"
#include "oxygen/drawing/software/Blitter.h"
#include "oxygen/engine/modding/ModManager.h"
#include "oxygen/helper/FileHelper.h"
#include "oxygen/platform/PlatformFunctions.h"
#include "oxygen/engine/EngineMain.h"
#include "oxygen/rendering/parts/RenderParts.h"
#include "oxygen/simulation/CodeExec.h"
#include "oxygen/simulation/EmulatorInterface.h"
#include "oxygen/simulation/Simulation.h"
#include "oxygen/simulation/SaveStateSerializer.h"
#include "oxygen/simulation/GameRecorder.h"
#include "oxygen/simulation/LemonScriptProgram.h"
#include <ctime>

#include <lemon/program/function/FunctionWrapper.h>
#include <lemon/program/Module.h>
#include <lemon/program/ModuleBindingsBuilder.h>


namespace
{
	const constexpr float CUTSCENE_SKIPPING_SPEED = 4.0f;	// Should be more or less "unique", not one of the debug game speeds (3.0f or 5.0f)

	void setDiscordDetails(lemon::StringRef text)
	{
		if (text.isValid())
			DiscordIntegration::setModdedDetails(text.getString());
	}

	void setDiscordState(lemon::StringRef text)
	{
		if (text.isValid())
			DiscordIntegration::setModdedState(text.getString());
	}

	void setDiscordLargeImage(lemon::StringRef imageName)
	{
		if (imageName.isValid())
			DiscordIntegration::setModdedLargeImage(imageName.getString());
	}

	void setDiscordSmallImage(lemon::StringRef imageName)
	{
		if (imageName.isValid())
			DiscordIntegration::setModdedSmallImage(imageName.getString());
	}

	void setUnderwaterAudioEffect(uint8 value)
	{
		AudioOut::instance().enableUnderwaterEffect((float)value / 255.0f);
	}
}


Game::Game()
{
}

void Game::startup(EmulatorInterface& emulatorInterface)
{
	mEmulatorInterface = &emulatorInterface;
	mPlayerRecorder.setEmulatorInterface(emulatorInterface);

	mAnniversarySaveStatesDir = Configuration::instance().mSaveStatesDir;
	mAnniversarySaveStatesDirLocal = Configuration::instance().mSaveStatesDirLocal;
	loadModeProfile();
	mBlueSpheresRendering.startup();
	mGameClient.setupClient();
	checkActiveModsUsedFeatures();

	DiscordIntegration::startup();
}

void Game::shutdown()
{
	stopReplay();
	mPlayerProgress.save();

	DiscordIntegration::shutdown();
}

void Game::update(float timeElapsed)
{
	// Update game client
	mGameClient.updateClient(timeElapsed);

	// Update sprite redirects (like input icons)
	mDynamicSprites.updateSpriteRedirects();

	// Discord rich presence update
	{
		mTimeoutUntilDiscordRefresh -= timeElapsed;
		if (mTimeoutUntilDiscordRefresh <= 0.0f)
		{
			DiscordIntegration::updateInfo(mMode, mSubMode, *mEmulatorInterface);
			mTimeoutUntilDiscordRefresh = 3.0f;
		}
		DiscordIntegration::update();
	}

	if (EngineMain::getDelegate().useDeveloperFeatures() && (FTX::keyState(SDLK_LALT) || FTX::keyState(SDLK_RALT)))
	{
		// Alt + X (and maybe Shift as well) for testing unlock windows
		static bool pressed = false;
		if (FTX::keyState('x') != pressed)
		{
			pressed = !pressed;
			if (pressed)
			{
				if (FTX::keyState(SDLK_LSHIFT))
					GameApp::instance().showUnlockedWindow(SecretUnlockedWindow::EntryType::SECRET, "Secret unlocked!", "Knuckles & Tails character combination can now be selected in Normal Game and Act Select.");
				else
					GameApp::instance().showUnlockedWindow(SecretUnlockedWindow::EntryType::ACHIEVEMENT, "Achievement unlocked!", "Cheated an achievement. Well done!");
			}
		}
	}

	if (mReplayPromptPending)
	{
		mReplayPromptPending = false;
		GameApp::instance().showReplaySavePrompt();
	}
	if (mReturnToMenuTriggered)
	{
		stopReplay();
		Application::instance().getSimulation().setSpeed(0.0f);
		GameApp::instance().returnToMenu();
		mReturnToMenuTriggered = false;
	}
	else if (mRestartTriggered)
	{
		startIntoLevel(mMode, mSubMode, mLastZoneAndAct, mLastCharacters);
		mRestartTriggered = false;
	}
}

void Game::registerScriptBindings(lemon::Module& module)
{
	lemon::ModuleBindingsBuilder builder(module);

	const BitFlagSet<lemon::Function::Flag> defaultFlags(lemon::Function::Flag::ALLOW_INLINE_EXECUTION);
	const BitFlagSet<lemon::Function::Flag> noInlineExecution;

	// Game
	{
		builder.addNativeFunction("Game.isClassicMode", lemon::wrap(*this, &Game::isClassicMode), defaultFlags);
		builder.addNativeFunction("Game.getSetting", lemon::wrap(*this, &Game::useSetting), defaultFlags)
			.setParameters("settingId");

		builder.addNativeFunction("Game.isSecretUnlocked", lemon::wrap(*this, &Game::isSecretUnlocked), defaultFlags)
			.setParameters("secretId");

		builder.addNativeFunction("Game.setSecretUnlocked", lemon::wrap(*this, &Game::setSecretUnlocked), defaultFlags)
			.setParameters("secretId");

		builder.addNativeFunction("Game.triggerRestart", lemon::wrap(*this, &Game::triggerRestart), defaultFlags);

		builder.addNativeFunction("Game.onGamePause", lemon::wrap(*this, &Game::onGamePause), defaultFlags)
			.setParameters("canRestart");

		builder.addNativeFunction("Game.allowRestartInGamePause", lemon::wrap(*this, &Game::allowRestartInGamePause), defaultFlags)
			.setParameters("canRestart");

		builder.addNativeFunction("Game.retroEvent", lemon::wrap(*this, &Game::retroEvent), defaultFlags);
		builder.addNativeFunction("Game.onLevelStart", lemon::wrap(*this, &Game::onLevelStart), defaultFlags);

		builder.addNativeFunction("Game.onZoneActCompleted", lemon::wrap(*this, &Game::onZoneActCompleted), defaultFlags)
			.setParameters("zoneAndAct");

		builder.addNativeFunction("Game.onTriggerNextZone", lemon::wrap(*this, &Game::onTriggerNextZone), defaultFlags)
			.setParameters("zoneAndAct");

		builder.addNativeFunction("Game.onFadedOutLoadingZone", lemon::wrap(*this, &Game::onFadedOutLoadingZone), defaultFlags)
			.setParameters("zoneAndAct");

		builder.addNativeFunction("Game.onCharacterDied", lemon::wrap(*this, &Game::onCharacterDied), noInlineExecution)		// No inline execution as this function manipulated the call stack
			.setParameters("playerIndex");

		builder.addNativeFunction("Game.returnToMainMenu", lemon::wrap(*this, &Game::returnToMainMenu), defaultFlags);
		builder.addNativeFunction("Game.openOptionsMenu", lemon::wrap(*this, &Game::openOptionsMenu), defaultFlags);

		builder.addNativeFunction("Game.isNormalGame", lemon::wrap(*this, &Game::isNormalGame), defaultFlags);
		builder.addNativeFunction("Game.isTimeAttack", lemon::wrap(*this, &Game::isTimeAttack), defaultFlags);
		builder.addNativeFunction("Game.onTimeAttackFinish", lemon::wrap(*this, &Game::onTimeAttackFinish), defaultFlags);

		builder.addNativeFunction("Game.changePlanePatternRectAtex", lemon::wrap(*this, &Game::changePlanePatternRectAtex), defaultFlags)
			.setParameters("px", "py", "width", "height", "planeIndex", "atex");

		builder.addNativeFunction("Game.setupBlueSpheresGroundSprites", lemon::wrap(*this, &Game::setupBlueSpheresGroundSprites), defaultFlags);

		builder.addNativeFunction("Game.writeBlueSpheresData", lemon::wrap(*this, &Game::writeBlueSpheresData), defaultFlags)
			.setParameters("targetAddress", "sourceAddress", "px", "py", "rotation");

		builder.addNativeFunction("Game.getAchievementValue", lemon::wrap(*this, &Game::getAchievementValue), defaultFlags)
			.setParameters("achievementId");

		builder.addNativeFunction("Game.setAchievementValue", lemon::wrap(*this, &Game::setAchievementValue), defaultFlags)
			.setParameters("achievementId", "value");

		builder.addNativeFunction("Game.isAchievementComplete", lemon::wrap(*this, &Game::isAchievementComplete), defaultFlags)
			.setParameters("achievementId");

		builder.addNativeFunction("Game.setAchievementComplete", lemon::wrap(*this, &Game::setAchievementComplete), defaultFlags)
			.setParameters("achievementId");

		builder.addNativeFunction("Game.startSkippableCutscene", lemon::wrap(*this, &Game::startSkippableCutscene), defaultFlags);
		builder.addNativeFunction("Game.endSkippableCutscene", lemon::wrap(*this, &Game::endSkippableCutscene), defaultFlags);
		builder.addNativeFunction("Game.isInSkippableCutscene", lemon::wrap(*this, &Game::isInSkippableCutscene), defaultFlags);
	}

	// Discord
	{
		builder.addNativeFunction("Game.setDiscordDetails", lemon::wrap(&setDiscordDetails), defaultFlags)
			.setParameters("text");

		builder.addNativeFunction("Game.setDiscordState", lemon::wrap(&setDiscordState), defaultFlags)
			.setParameters("text");

		builder.addNativeFunction("Game.setDiscordLargeImage", lemon::wrap(&setDiscordLargeImage), defaultFlags)
			.setParameters("imageName");

		builder.addNativeFunction("Game.setDiscordSmallImage", lemon::wrap(&setDiscordSmallImage), defaultFlags)
			.setParameters("imageName");
	}

	// Audio
	{
		builder.addNativeFunction("Game.setUnderwaterAudioEffect", lemon::wrap(&setUnderwaterAudioEffect), defaultFlags)
			.setParameters("value");
	}

	ScriptImplementations::registerScriptBindings(module);
}

uint32 Game::getSetting(uint32 settingId, bool ignoreGameMode) const
{
	uint32 forcedValue;
	if (Configuration::instance().mClassicMode && ClassicModeRules::forcedSetting(settingId, forcedValue))
		return forcedValue;
	const SharedDatabase::Setting* setting = SharedDatabase::getSetting(settingId);

	if (!ignoreGameMode && isInTimeAttackMode())
	{
		// In Time Attack, non-visual settings are always using the default value
		if ((settingId & 0x80000000) == 0)
		{
			const bool explicitlyAllowInTimeAttack = (nullptr != setting && setting->mAllowInTimeAttack);
			if (!explicitlyAllowInTimeAttack)
			{
				// Only exception are the "max control" settings
				if (settingId == SharedDatabase::Setting::SETTING_DROPDASH || settingId == SharedDatabase::Setting::SETTING_SUPER_PEELOUT)
				{
					return (mSubMode == 0x11) ? 1 : 0;
				}
				else
				{
					return (settingId & 0xff);
				}
			}
		}
	}

	if (nullptr != setting)
	{
		const uint32 value = ConfigurationImpl::instance().mActiveGameSettings->getValue(settingId);

		// Special handling for Debug Mode setting in dev mode
		if (settingId == SharedDatabase::Setting::SETTING_DEBUG_MODE)
		{
			if (value == 0)
			{
				if (EngineMain::getDelegate().useDeveloperFeatures() && ConfigurationImpl::instance().mDevModeImpl.mEnforceDebugMode)
					return true;
			}
		}
		return value;
	}
	else
	{
		// Use default value
		return (settingId & 0xff);
	}
}

void Game::setSetting(uint32 settingId, uint32 value)
{
	uint32 forcedValue;
	if (Configuration::instance().mClassicMode && ClassicModeRules::forcedSetting(settingId, forcedValue))
		return;
	RMX_CHECK(nullptr != SharedDatabase::getSetting(settingId), "Setting not found", return);
	ConfigurationImpl::instance().mActiveGameSettings->setValue(settingId, value);
}

void Game::checkForUnlockedSecrets(bool saveIfAnyUnlocked)
{
	// Check for unlocked secrets
	uint32 achievementsCompleted = 0;
	for (const SharedDatabase::Achievement& achievement : SharedDatabase::getAchievements())
	{
		if (achievement.mType < retroachievements::ID_BASE && mPlayerProgress.mAchievements.getAchievementState(achievement.mType) > 0)
		{
			++achievementsCompleted;
		}
	}

	bool anyUnlocked = false;
	for (const SharedDatabase::Secret& secret : SharedDatabase::getSecrets())
	{
		if (secret.mUnlockedByAchievements && !mPlayerProgress.mUnlocks.isSecretUnlocked(secret.mType) && achievementsCompleted >= secret.mRequiredAchievements)
		{
			// Unlock secret now
			mPlayerProgress.mUnlocks.setSecretUnlocked(secret.mType);
			GameApp::instance().showUnlockedWindow(SecretUnlockedWindow::EntryType::SECRET, "Secret unlocked!", secret.mName);
			anyUnlocked = true;
		}
	}

	if (anyUnlocked && saveIfAnyUnlocked)
	{
		mPlayerProgress.save();
	}
}

void Game::unlockSecret(uint32 secretId)
{
	setSecretUnlocked(secretId);
}

void Game::startIntoTitleScreen()
{
	mMode = Mode::TITLE_SCREEN;

	Simulation& simulation = Application::instance().getSimulation();
	simulation.resetState();

	startIntoGameInternal();
}

void Game::startIntoDataSelect()
{
	mMode = Mode::NORMAL_GAME;

	Simulation& simulation = Application::instance().getSimulation();
	simulation.resetIntoGame("EntryFunctions.dataSelect");

	startIntoGameInternal();
}

void Game::startIntoActSelect()
{
	mMode = Mode::ACT_SELECT;

	Simulation& simulation = Application::instance().getSimulation();
	simulation.resetIntoGame("EntryFunctions.actSelectMenu");

	startIntoGameInternal();
}

void Game::startIntoLevel(Mode mode, uint32 submode, uint16 zoneAndAct, uint8 characters)
{
	mMode = mode;
	mSubMode = submode;

	Simulation& simulation = Application::instance().getSimulation();
	simulation.resetIntoGame("EntryFunctions.actSelect");

	startIntoGameInternal();
	mLastZoneAndAct = zoneAndAct;
	mLastCharacters = characters;

	mEmulatorInterface->writeMemory16(0xfffffe10, zoneAndAct);
	mEmulatorInterface->writeMemory16(0xffffff0a, characters);

	if (mMode == Mode::TIME_ATTACK)
	{
		mPlayerRecorder.setMaxGhosts(getSetting(SharedDatabase::Setting::SETTING_TIME_ATTACK_GHOSTS, true));

		const uint8 category = submode;
		std::wstring recBaseFilename;
		const std::wstring recordingsDir = TimeAttackData::getSavePath(zoneAndAct, category, &recBaseFilename);

		if (!recordingsDir.empty())
		{
			mPlayerRecorder.initForDirectory(recordingsDir, recBaseFilename, zoneAndAct, category);
		}
	}

	GameApp::instance().enableStillImageBlur(false);
}

void Game::restartLevel()
{
	Simulation& simulation = Application::instance().getSimulation();
	simulation.getCodeExec().getLemonScriptRuntime().callFunctionByName("restartLevel");
	simulation.setSpeed(simulation.getDefaultSpeed());		// Because it could be paused
}

void Game::restartAtCheckpoint()
{
	Simulation& simulation = Application::instance().getSimulation();
	simulation.getCodeExec().getLemonScriptRuntime().callFunctionByName("restartAtCheckpoint");
	simulation.setSpeed(simulation.getDefaultSpeed());		// Because it could be paused
}

void Game::restartTimeAttack(bool skipFadeout)
{
	if (skipFadeout)
	{
		startIntoLevel(mMode, mSubMode, mLastZoneAndAct, mLastCharacters);
	}
	else
	{
		if (!mRestartingTimeAttack)
		{
			mRestartingTimeAttack = true;
			Simulation& simulation = Application::instance().getSimulation();
			simulation.getCodeExec().getLemonScriptRuntime().callFunctionByName("restartTimeAttack");
			simulation.setSpeed(simulation.getDefaultSpeed());		// Because it could be paused
		}
	}
}

void Game::startIntoCompetitionMode()
{
	mMode = Mode::COMPETITION;

	Simulation& simulation = Application::instance().getSimulation();
	simulation.resetIntoGame("EntryFunctions.competitionMode");

	startIntoGameInternal();
}

void Game::startIntoBlueSphere()
{
	mMode = Mode::BLUE_SPHERE;

	Simulation& simulation = Application::instance().getSimulation();
	simulation.resetIntoGame("EntryFunctions.blueSphereGame");

	startIntoGameInternal();
}

void Game::startIntoLevelSelect()
{
	mMode = Mode::ACT_SELECT;

	Simulation& simulation = Application::instance().getSimulation();
	simulation.resetIntoGame("EntryFunctions.levelSelect");

	startIntoGameInternal();
	retroEvent(retroachievements::CHEAT_EVENT, 0, 0);
}

void Game::startIntoMainMenuBG()
{
	mMode = Mode::MAIN_MENU_BG;

	Simulation& simulation = Application::instance().getSimulation();
	simulation.resetIntoGame("EntryFunctions.mainMenuBG");

	startIntoGameInternal();
}

void Game::onPreUpdateFrame()
{
	if (mReplayStartPending && !mReplayPlaying)
	{
		mReplayStartPending = false;
		beginReplayRecording();
	}
}

void Game::onPostUpdateFrame()
{
	EmulatorInterface& emulatorInterface = *mEmulatorInterface;

	// If code execution stopped, return to the main menu
	if (!Application::instance().getSimulation().getCodeExec().isCodeExecutionPossible())
	{
		if (mMode != Mode::UNDEFINED && mMode != Mode::MAIN_MENU_BG)
		{
			GameApp::instance().returnToMenu();
		}
	}

	// Check for invalid game mode inside simulation
	{
		const uint8 gameMode = emulatorInterface.readMemory8(0xfffff600) & 0x7f;
		bool isValidGameMode = true;
		if (isInTimeAttackMode())
		{
			// Only allowed is the main game
			isValidGameMode = (gameMode <= 0x0c);
		}

		if (!isValidGameMode)
		{
			resetCurrentMode();
			Application::instance().getSimulation().setSpeed(0.0f);
			GameApp::instance().returnToMenu();
			return;
		}
	}

	if (mReplayFinishPending)
	{
		mReplayFinishPending = false;
		mReplayRecording = false;
		if (mReplayPlaying) returnToMainMenu();
		else if (!mReplay.frames.empty())
		{
			mReplayPromptPending = true;
			Application::instance().getSimulation().setSpeed(0.0f);
		}
	}
	if (mReplayPlaying && mReplayFrame >= mReplay.frames.size())
	{
		returnToMainMenu();
		Application::instance().getSimulation().setSpeed(0.0f);
	}

	// Update skippable cutscene
	if (mSkippableCutsceneFrames > 0)
	{
		--mSkippableCutsceneFrames;
		if (mSkippableCutsceneFrames == 0)
		{
			endSkippableCutscene();
		}
		else
		{
			Simulation& simulation = Application::instance().getSimulation();
			if (simulation.getSpeed() <= 0.0f) {} // Keep pause and replay prompts paused.
			else if (ConfigurationImpl::instance().mSpeedrunMode)
			{
				simulation.setSpeed(2.0f);
				mSpeedrunCutsceneActive = true;
			}
			else if (mButtonYPressedDuringSkippableCutscene)
			{
				if (simulation.getSpeed() == 1.0f)
				{
					simulation.setSpeed(CUTSCENE_SKIPPING_SPEED);
					GameApp::instance().showSkippableCutsceneWindow(true);
				}
			}
			else
			{
				if (mSpeedrunCutsceneActive || simulation.getSpeed() == CUTSCENE_SKIPPING_SPEED)
				{
					simulation.setSpeed(simulation.getDefaultSpeed());
					mSpeedrunCutsceneActive = false;
				}
				GameApp::instance().showSkippableCutsceneWindow(false);
			}
		}
	}

	// Run after every simulation frame, independently of the 30 FPS presentation cap.
	if (mMode == Mode::NORMAL_GAME || mMode == Mode::COMPETITION)
	{
		const uint8 gameMode = emulatorInterface.readMemory8(0xfffff600) & 0x7f;
		if (getSetting(SharedDatabase::Setting::SETTING_DEBUG_MODE, true) != 0 || gameMode == 0x1c)
			retroEvent(retroachievements::CHEAT_EVENT, 0, 0);
		retroEvent(retroachievements::FRAME, 0, 0);
	}

	// Update player recorder
	mPlayerRecorder.onPostUpdateFrame();

	// Update ghost sync
	GameClient::instance().getGhostSync().onPostUpdateFrame();

	if (mReceivedTimeAttackFinished)
	{
		int hundreds = 0;
		std::vector<int> otherTimes;
		if (getPlayerRecorder().onTimeAttackFinish(hundreds, otherTimes))
		{
			GameApp::instance().showTimeAttackResults(hundreds, otherTimes);
		}
	}
}

void Game::onUpdateControls()
{
	if (mReplayPlaying)
	{
		// Hardware pause remains available even though recorded input owns the gamepads.
		const auto& keys = InputManager::instance().getController(0);
		if (keys.Start.justPressed() || keys.Back.justPressed())
		{
			GameApp::instance().onGamePaused(false);
		}
		while (mReplaySettingEvent < mReplay.events.size() && mReplay.events[mReplaySettingEvent].frame == mReplayFrame)
		{
			const auto& event = mReplay.events[mReplaySettingEvent++];
			if (event.key == 0xffffffff) ConfigurationImpl::instance().mSpeedrunMode = event.value != 0;
			else ConfigurationImpl::instance().mActiveGameSettings->setValue(event.key, event.value);
		}
		if (mReplayFrame < mReplay.frames.size())
		{
			for (size_t p = 0; p < ControlsIn::NUM_GAMEPADS; ++p)
			{
				auto& pad = ControlsIn::instance().getGamepad(p);
				pad.mPreviousInput = mReplay.previousInputs[mReplayFrame][p];
				pad.mCurrentInput = mReplay.frames[mReplayFrame][p];
			}
			++mReplayFrame;
		}
		return;
	}
	if (mSkippableCutsceneFrames > 0 || mButtonYPressedDuringSkippableCutscene)	// Last check makes sure we'll ignore the press until it gets released
	{
		// Block input to the game
		mButtonYPressedDuringSkippableCutscene = ControlsIn::instance().getGamepad(0).isPressed(ControlsIn::Button::Y);
		if (mButtonYPressedDuringSkippableCutscene)
		{
			ControlsIn::instance().injectEmptyInputs();
		}
	}
	if (mReplayRecording)
	{
		if (mReplay.frames.size() < s3air::ReplayData::MAX_FRAMES)
		{
			for (auto& pair : mReplayCurrentSettings)
			{
				const uint32 value = pair.first == 0xffffffff ? uint32(ConfigurationImpl::instance().mSpeedrunMode) : getSetting(pair.first, false);
				if (value != pair.second)
				{
					mReplay.events.push_back({uint32(mReplay.frames.size()), pair.first, value});
					pair.second = value;
				}
			}
			s3air::ReplayData::Input input{};
			ControlsIn::instance().writeCurrentState(input.data());
			mReplay.frames.push_back(input);
			s3air::ReplayData::Input previous{};
			for (size_t p = 0; p < ControlsIn::NUM_GAMEPADS; ++p) previous[p] = ControlsIn::instance().getGamepad(p).mPreviousInput;
			mReplay.previousInputs.push_back(previous);
		}
		else { mReplayRecording = false; mReplay = {}; }
	}
}

void Game::updateSpecialInput(float timeElapsed)
{
	// In time attack mode: Restart if holding Y button for a while
	bool isCharging = false;
	if (isInTimeAttackMode())
	{
		if (Application::instance().getSimulation().getSpeed() > 0.0f)	// Not in pause
		{
			const InputManager::Control& buttonY = InputManager::instance().getController(0).Y;
			if (buttonY.isPressed())
			{
				if (buttonY.justPressed())
				{
					// Start charging
					isCharging = true;
					mTimeAttackRestartCharge = 0.0f;
				}
				else
				{
					// Keep charging if already start, otherwise ignore the button
					isCharging = mTimeAttackRestartCharging;
				}
			}
		}
	}

	if (isCharging)
	{
		if (ConfigurationImpl::instance().mInstantTimeAttackRestart)
		{
			// Restart now
			mTimeAttackRestartCharge = 0.0f;
			restartTimeAttack(false);
			isCharging = false;
		}
		else
		{
			mTimeAttackRestartCharge += timeElapsed / 0.5f;
			if (mTimeAttackRestartCharge >= 1.0f)
			{
				// Restart now
				mTimeAttackRestartCharge = 1.0f;
				restartTimeAttack(true);
				isCharging = false;
			}
		}
	}
	else if (mTimeAttackRestartCharge > 0.0f)
	{
		mTimeAttackRestartCharge = std::max(0.0f, mTimeAttackRestartCharge - timeElapsed / 0.2f);
	}
	mTimeAttackRestartCharging = isCharging;

	// Update white overlay
	Application::instance().getGameView().setWhiteOverlayAlpha(mTimeAttackRestartCharge);
}

void Game::onActiveModsChanged()
{
	checkActiveModsUsedFeatures();
	mDynamicSprites.updateSpriteRedirects();
}

bool Game::shouldPauseOnFocusLoss() const
{
	return (GameApp::hasInstance() && Application::instance().getSimulation().isRunning() && Application::instance().getSimulation().getSpeed() > 0.0f && !isInMainMenuMode());
}

void Game::fillDebugVisualization(Bitmap& bitmap, int& mode)
{
	mode = mode % 2;
	static Bitmap textBitmap;

	const Vec2i cameraPosition = VideoOut::instance().getInterpolatedWorldSpaceOffset();
	const int32 cameraX = cameraPosition.x;
	const int32 cameraY = cameraPosition.y;
	const int32 cameraTileX = cameraX / 16;
	const int32 cameraTileY = cameraY / 16;

	const int32 minX = cameraTileX;
	const int32 maxX = cameraTileX + bitmap.getWidth() / 16;
	const int32 minY = cameraTileY;
	const int32 maxY = cameraTileY + bitmap.getHeight() / 16;

	const uint16 filterByCharacterPath = (3 << mEmulatorInterface->readMemory8(0xffffb046));

	for (int32 y = minY; y <= maxY; ++y)
	{
		for (int32 x = minX; x <= maxX; ++x)
		{
			const int32 screenX = x*16 - cameraX;
			const int32 screenY = y*16 - cameraY;

			// Access the right chunk
			const uint16 chunkX = (x / 8);
			const uint16 chunkY = (y / 8) & 0x1f;
			uint32 address = mEmulatorInterface->readMemory16(0xffff8008 + chunkY * 4) + chunkX;
			const uint16 chunkType = mEmulatorInterface->readMemory8(0xffff0000 + address);

			// Access tile info
			address = mEmulatorInterface->readMemory16(0x00f02a + chunkType * 2);
			address += ((x % 8) + (y % 8) * 8) * 2;
			const uint16 tile = mEmulatorInterface->readMemory16(0xffff0000 + address);
			const uint8 tileForm = mEmulatorInterface->readMemory8(mEmulatorInterface->readMemory32(0xfffff796) + (tile & 0x03ff) * 2);

			if ((tile & 0xf000) != 0 && tileForm != 0)
			{
				Color baseColor;
				uint8 angle;
				if (mode == 1)	// Angle is only relevant in mode 1
				{
					angle = mEmulatorInterface->readMemory8(0x096000 + tileForm);
					if (tile & 0x0400)	// Flip horizontally
						angle = 0x100 - angle;
					if (tile & 0x0800)	// Flip vertically
						angle = 0x80 - angle;

					baseColor.setFromHSL(Vec3f((float)angle / 256.0f * 360.0f, 1.0f, 0.5f));
					baseColor.a = 0.7f;
				}

				for (int32 iy = 0; iy < 16; ++iy)
				{
					const int32 py = (screenY + iy);
					if (py < 0 || py >= bitmap.getHeight())
						continue;

					for (int32 ix = 0; ix < 16; ++ix)
					{
						const int32 px = (screenX + ix);
						if (px < 0 || px >= bitmap.getWidth())
							continue;

						bool pixelVisible = false;

					#if 1
						// Vertical
						const uint16 offset = (tile & 0x0400) ? (15 - ix) : ix;
						int8 indent = (int8)mEmulatorInterface->readMemory8(0x096100 + (tileForm * 0x10) + offset);
						if (indent != 0)
						{
							if (tile & 0x0800)
								indent = -indent;

							if (indent > 0)
							{
								pixelVisible = ((signed)(15 - iy) < indent);
							}
							else
							{
								pixelVisible = ((signed)iy < -indent);
							}
						}
					#else
						// Horizontal
						const uint16 offset = (tile & 0x0800) ? (15 - iy) : iy;
						int8 indent = (int8)mEmulatorInterface->readMemory8(0x097100 + (tileForm * 0x10) + offset);
						if (indent != 0)
						{
							if (tile & 0x0400)
								indent = -indent;

							if (indent > 0)
							{
								pixelVisible = ((signed)(15 - ix) < indent);
							}
							else
							{
								pixelVisible = ((signed)ix < -indent);
							}
						}
					#endif

						uint32& dst = *bitmap.getPixelPointer(px, py);
						switch (mode)
						{
							case 0:
							{
								if (pixelVisible)
								{
									const bool isSolidAbove = (tile & 0x5000) != 0;		// Solid from above
									const bool isSolidOther = (tile & 0xa000) != 0;		// Solid from left/right/below
									const bool isPath1 = (tile & 0x3000) != 0;
									const bool isPath2 = (tile & 0xc000) != 0;

									dst = ((tile & filterByCharacterPath) != 0) ? 0xc0000000 : 0x50000000;

									if (!isPath1 && (ix + iy) & 0x04)	dst = 0x40000000;
									if (!isPath2 && (ix - iy) & 0x04)	dst = 0x40000000;
									if (isSolidAbove)	dst += 0xff00ff;
									if (isSolidOther)	dst += 0x00ff00;
								}
								break;
							}

							case 1:
							{
								dst = baseColor.getABGR32();
								if (pixelVisible)
								{
									if (ix == 0x0f || iy == 0x0f)
										dst -= (dst & 0xf8f8f8) >> 3;
								}
								else
								{
									dst = ((dst & 0xfc000000) >> 2) | (dst & 0xffffff);
								}
								break;
							}
						}
					}
				}

				if (mode == 1)
				{
					Vec2i dummy;
					global::mSmallfont.printBitmap(textBitmap, dummy, Recti(0, 0, 16, 16), rmx::hexString(angle, 2, ""));

					Blitter::Options blitterOptions;
					blitterOptions.mBlendMode = BlendMode::ALPHA;

					static Blitter blitter;
					blitter.blitSprite(Blitter::OutputWrapper(bitmap), Blitter::SpriteWrapper(textBitmap, Vec2i()), Vec2i(screenX+4, screenY+5), blitterOptions);
				}
			}
		}
	}
}

void Game::onGameRecordingHeaderLoaded(const std::string& buildString, const std::vector<uint8>& buffer)
{
	// Switch to using the alternative set of settings
	ConfigurationImpl::instance().mActiveGameSettings = &ConfigurationImpl::instance().mAlternativeGameSettings;

	const std::unordered_map<uint32, SharedDatabase::Setting>& settings = SharedDatabase::getSettings();
	VectorBinarySerializer serializer(true, buffer);
	const size_t numSettings = serializer.read<uint32>();
	for (size_t i = 0; i < numSettings; ++i)
	{
		const uint32 settingId = serializer.read<uint32>();
		const uint32 value = serializer.read<uint32>();
		ConfigurationImpl::instance().mActiveGameSettings->setValue(settingId, value);
	}
}

void Game::onGameRecordingHeaderSave(std::vector<uint8>& buffer)
{
	std::vector<const SharedDatabase::Setting*> relevantSettings;
	{
		relevantSettings.reserve(32);
		const std::unordered_map<uint32, SharedDatabase::Setting>& settings = SharedDatabase::getSettings();
		for (const auto& pair : settings)
		{
			const SharedDatabase::Setting& setting = pair.second;
			if (!setting.mPurelyVisual)
				relevantSettings.push_back(&setting);
		}
	}

	VectorBinarySerializer serializer(false, buffer);
	serializer.writeAs<uint32>(relevantSettings.size());
	for (const SharedDatabase::Setting* setting : relevantSettings)
	{
		const uint32 value = ConfigurationImpl::instance().mLocalGameSettings.getValue(setting->mSettingId);
		serializer.writeAs<uint32>(setting->mSettingId);
		serializer.write(value);
	}
}

void Game::checkActiveModsUsedFeatures()
{
	// Update number of players depending on the three / four player mod feature
	const bool usesThreePlayers = ModManager::instance().anyActiveModUsesFeature(rmx::constMurmur2_64("ThreePlayers"));
	const bool usesFourPlayers = ModManager::instance().anyActiveModUsesFeature(rmx::constMurmur2_64("FourPlayers"));
	Configuration::instance().mNumPlayers = usesFourPlayers ? 4 : usesThreePlayers ? 3 : 2;
}

void Game::startIntoGameInternal()
{
	stopReplay();
	mReplay = {};
	mReplayRecording = mReplayStartPending = mReplayFinishPending = mReplayPromptPending = false;
	mSpeedrunCutsceneActive = false;
	mRetroAchievements = {};
	// Menus keep their native layout; every game/intro entry chooses a real field size.
	if (!isInMainMenuMode())
	{
		Configuration::instance().mClassicMenuActive = false;
		VideoOut::instance().setScreenSize(ClassicModeRules::gameSize(Configuration::instance().mClassicMode));
	}
	// Setup defaults -- these should not get used outside of Time Attack anyway
	mLastZoneAndAct = 0;
	mLastCharacters = 0;
	mReceivedTimeAttackFinished = false;
	mSkippableCutsceneFrames = 0;

	Simulation& simulation = Application::instance().getSimulation();
	simulation.setRunning(true);
	simulation.setSpeed(simulation.getDefaultSpeed());

	// Enforce fixed simulation frequency in Time Attack and for the Main Menu background
	if (isInTimeAttackMode() || isInMainMenuMode())
		simulation.setSimulationFrequencyOverride(60.0f);
	else
		simulation.disableSimulationFrequencyOverride();

	setSetting(SharedDatabase::Setting::SETTING_KNUCKLES_AND_TAILS, false);		// Not queried by scripts at all, but it can't hurt to set it to false nevertheless

	SharedDatabase::resetAchievementValues();

	if (!isInMainMenuMode())
	{
		AudioOut::instance().moveMenuMusicToIngame();	// Needed only for the data select music to continue from main menu to data select
		AudioOut::instance().resumeSoundContext(AudioOut::CONTEXT_INGAME + AudioOut::CONTEXT_MUSIC);
		AudioOut::instance().resumeSoundContext(AudioOut::CONTEXT_INGAME + AudioOut::CONTEXT_SOUND);
	}

	mPlayerRecorder.reset();
	GameApp::instance().enableStillImageBlur(false);

	mTimeoutUntilDiscordRefresh = 0.0f;
}

uint32 Game::useSetting(uint32 settingId)
{
	return getSetting(settingId, false);
}

int32 Game::getAchievementValue(uint32 achievementId)
{
	SharedDatabase::Achievement* achievement = SharedDatabase::getAchievement(achievementId);
	return (nullptr == achievement) ? 0 : achievement->mValue;
}

void Game::setAchievementValue(uint32 achievementId, int32 value)
{
	SharedDatabase::Achievement* achievement = SharedDatabase::getAchievement(achievementId);
	if (nullptr != achievement)
	{
		achievement->mValue = value;
	}
}

bool Game::isAchievementComplete(uint32 achievementId)
{
	return (mPlayerProgress.mAchievements.getAchievementState(achievementId) != 0);
}

void Game::setAchievementComplete(uint32 achievementId)
{
	if (mReplayPlaying) return;
	if (achievementId == SharedDatabase::Achievement::ACHIEVEMENT_LONGPLAY) completeReplayRecording(mReplay.zoneAct);
	if (achievementId == SharedDatabase::Achievement::ACHIEVEMENT_LONGPLAY)
		retroEvent(retroachievements::GAME_FINISH, 0, 0);
	else if (achievementId == SharedDatabase::Achievement::ACHIEVEMENT_SM_JACKPOT)
		retroEvent(retroachievements::JACKPOT_EVENT, 0, 0);
	else if (achievementId == SharedDatabase::Achievement::ACHIEVEMENT_GS_EXIT_TOP)
		retroEvent(retroachievements::GLOW_EXIT_EVENT, 0, 0);
	// Can't affect achievements in debug mode (except if dev mode is active)
	const bool hasDebugModeActive = getSetting(SharedDatabase::Setting::SETTING_DEBUG_MODE, true) != 0;
	if (hasDebugModeActive && achievementId != retroachievements::ID_BASE + retroachievements::CHEAT && !EngineMain::getDelegate().useDeveloperFeatures())
		return;

	if (mPlayerProgress.mAchievements.getAchievementState(achievementId) == 0)
	{
		mPlayerProgress.mAchievements.mAchievementStates[achievementId] = 1;
		SharedDatabase::Achievement* achievement = SharedDatabase::getAchievement(achievementId);
		if (nullptr != achievement)
		{
			GameApp::instance().showUnlockedWindow(SecretUnlockedWindow::EntryType::ACHIEVEMENT, "Achievement complete", achievement->mName);
		}
		else
		{
			RMX_ERROR("Achievement not found", );
		}

		checkForUnlockedSecrets(false);
		mPlayerProgress.save();
	}
}

bool Game::isSecretUnlocked(uint32 secretId)
{
	return mPlayerProgress.mUnlocks.isSecretUnlocked(secretId);
}

void Game::setSecretUnlocked(uint32 secretId)
{
	if (mReplayPlaying) return;
	SharedDatabase::Secret* secret = SharedDatabase::getSecret(secretId);
	RMX_CHECK(nullptr != secret, "Secret with ID " << secretId << " not found", return);

	if (!mPlayerProgress.mUnlocks.isSecretUnlocked(secretId))
	{
		mPlayerProgress.mUnlocks.setSecretUnlocked(secretId);
		const char* text = (secret->mType == SharedDatabase::Secret::SECRET_DOOMSDAY_ZONE) ? "Unlocked in Act Select" : secret->mHiddenUntilUnlocked ? "Found hidden secret!" : "Secret unlocked!";
		GameApp::instance().showUnlockedWindow(SecretUnlockedWindow::EntryType::SECRET, text, secret->mName);
		mPlayerProgress.save();
	}
}

void Game::triggerRestart()
{
	mRestartTriggered = true;
}

void Game::onGamePause(uint8 canRestart)
{
	if (mReplayPlaying) return; // Recorded pauses must not interrupt playback.
	retroEvent(retroachievements::PAUSE, 0, 0);
	GameApp::instance().onGamePaused(canRestart != 0);
}

void Game::allowRestartInGamePause(uint8 canRestart)
{
	mAllowRestartInGamePause = (canRestart != 0);
}

void Game::onLevelStart()
{
	mRestartingTimeAttack = false;
	if (!mReplayPlaying && ConfigurationImpl::instance().mReplayMode &&
		(!mReplayRecording || mEmulatorInterface->readMemory16(0xffffee4e) != mReplay.zoneAct) &&
		(mMode == Mode::NORMAL_GAME || mMode == Mode::ACT_SELECT)) mReplayStartPending = true;
}

void Game::onZoneActCompleted(uint16 zoneAndAct)
{
	completeReplayRecording(zoneAndAct);
	if (mReplayPlaying) return;
	const uint8 bitNumber = (zoneAndAct >> 7) + (zoneAndAct & 1);
	const uint32 bitValue = (1 << bitNumber);
	const uint8 character = clamp(mLastCharacters, 1, 3) - 1;

	mPlayerProgress.mUnlocks.mFinishedZoneAct |= bitValue;
	mPlayerProgress.mUnlocks.mFinishedZoneActByCharacter[character] |= bitValue;
	mPlayerProgress.save();
}

uint16 Game::onTriggerNextZone(uint16 zoneAndAct)
{
	// These exits do not always create a normal level-results object.
	const uint16 currentAct = mEmulatorInterface->readMemory16(0xffffee4e);
	const uint16 physicalAct = mEmulatorInterface->readMemory16(0xfffffe10);
	if (currentAct == 0x0a00 && zoneAndAct == 0x0b00)
		retroEvent(retroachievements::ACT_FINISH, 0x0a00, 0);
	if ((physicalAct == 0x1700 || currentAct == 0x0b01) && (zoneAndAct == 0x0c00 || zoneAndAct == 0x0d01))
		retroEvent(retroachievements::ACT_FINISH, 0x0b01, 0);
	completeReplayRecording(currentAct);
	return zoneAndAct;
}

uint16 Game::onFadedOutLoadingZone(uint16 zoneAndAct)
{
	return zoneAndAct;
}

bool Game::onCharacterDied(uint8 playerIndex)
{
	if (playerIndex == 0) retroEvent(retroachievements::DEATH, 0, 0);
	// Death in time attack means level restart
	if (isInTimeAttackMode())
	{
		if (mReceivedTimeAttackFinished)
		{
			// No death allowed!
			return false;
		}

		Application::instance().getSimulation().getCodeExec().getLemonScriptRuntime().callFunctionByName("restartTimeAttack");
	}
	return true;
}

void Game::returnToMainMenu()
{
	mReturnToMenuTriggered = true;

	// Do not restart data select music
	if (AudioOut::instance().isPlayingAudioKey(0x2f))
	{
		AudioOut::instance().moveIngameMusicToMenu();
	}
	else
	{
		// Do a quick fade-out of the music (especially for transition from Blue Sphere mode and Competition Mode)
		AudioOut::instance().fadeOutChannel(0, 0.15f);
	}

	mTimeoutUntilDiscordRefresh = 0.0f;
}

void Game::openOptionsMenu()
{
	GameApp::instance().openOptionsMenuInGame();
}

bool Game::onTimeAttackFinish()
{
	if (!isInTimeAttackMode() || mReceivedTimeAttackFinished)
		return false;

	mReceivedTimeAttackFinished = true;
	return true;
}

void Game::changePlanePatternRectAtex(uint16 px, uint16 py, uint16 width, uint16 height, uint8 planeIndex, uint8 atex)
{
	s3air::changePlanePatternRectAtex(*mEmulatorInterface, px, py, width, height, planeIndex, atex);
}

void Game::setupBlueSpheresGroundSprites()
{
	mBlueSpheresRendering.createSprites(VideoOut::instance().getScreenSize());
}

void Game::writeBlueSpheresData(uint32 targetAddress, uint32 sourceAddress, uint16 px, uint16 py, uint8 rotation)
{
	mBlueSpheresRendering.writeVisibleSpheresData(targetAddress, sourceAddress, px, py, rotation, *mEmulatorInterface);
}

void Game::startSkippableCutscene()
{
	mSkippableCutsceneFrames = 5 * 60 * 60;		// Limit cutscene length to five minutes
}

void Game::endSkippableCutscene()
{
	mSkippableCutsceneFrames = 0;
	mButtonYPressedDuringSkippableCutscene = false;

	// Back to normal
	Simulation& simulation = Application::instance().getSimulation();
	if (simulation.getSpeed() > 0.0f && (mSpeedrunCutsceneActive || simulation.getSpeed() == CUTSCENE_SKIPPING_SPEED))
		simulation.setSpeed(1.0f);
	mSpeedrunCutsceneActive = false;

	GameApp::instance().showSkippableCutsceneWindow(false);
}

bool Game::isInSkippableCutscene()
{
	return mSkippableCutsceneFrames > 0;
}


bool Game::isClassicMode()
{
	return Configuration::instance().mClassicMode;
}

bool Game::setClassicMode(bool enabled)
{
    Configuration& config = Configuration::instance();
    if (config.mClassicMode == enabled) return true;
    const std::wstring root = ClassicModeRules::saveRoot(config.mAppDataPath, enabled, config.mSharedModeSaves);
    if (!FTX::FileSystem->exists(root + L"storage/") && !FTX::FileSystem->createDirectory(root + L"storage/"))
    {
        PlatformFunctions::showMessageBox("Save profile", "Could not create the save folder. The mode was not changed.");
        return false;
    }
    // Complete all writes in the old profile before changing the active base path.
    mPlayerProgress.save(true);
    PersistentData::instance().updatePersistentData();
    config.mClassicMode = enabled;
    loadModeProfile();
    EngineMain::instance().setVSyncMode(config.mFrameSync);
    config.saveSettings();
    return true;
}

void Game::loadModeProfile()
{
    Configuration& config = Configuration::instance();
    const std::wstring root = ClassicModeRules::saveRoot(config.mAppDataPath, config.mClassicMode, config.mSharedModeSaves);
    config.mPersistentDataBasePath = root + L"storage/";
	RMX_LOG_INFO("Mode profile: " << (config.mClassicMode ? "Classic" : "Anniversary") << "; storage: " << WString(config.mPersistentDataBasePath).toStdString());
    config.mSaveStatesDir = (config.mClassicMode && !config.mSharedModeSaves) ? root + L"savestates/" : mAnniversarySaveStatesDir;
    config.mSaveStatesDirLocal = (config.mClassicMode && !config.mSharedModeSaves) ? root + L"savestates/" : mAnniversarySaveStatesDirLocal;
    PersistentData::instance().loadFromBasePath(config.mPersistentDataBasePath);
    mPlayerProgress.reset();
    mPlayerProgress.load();
    TimeAttackData::clearCache();
}

void Game::retroEvent(uint32 event, uint32 a, uint32 b)
{
    if (mReplayPlaying) return;
    if (!mEmulatorInterface || event > retroachievements::INVALIDATE) return;
    auto& memory = *mEmulatorInterface;
    if (event == retroachievements::LEVEL_START && memory.readMemory8(0xfffffe48) != 0) return;
    retroachievements::Snapshot s;
    s.normal = (mMode == Mode::NORMAL_GAME);
    s.competition = (mMode == Mode::COMPETITION);
    s.zoneAct = memory.readMemory16(0xffffee4e);
    const unsigned physicalZoneAct = memory.readMemory16(0xfffffe10);
    s.rings = memory.readMemory16(0xfffffe20);
    s.lives = memory.readMemory8(0xfffffe12);
    s.time = (memory.readMemory8(0xfffffe23) * 60 + memory.readMemory8(0xfffffe24)) * 60 + memory.readMemory8(0xfffffe25);
    s.character = memory.readMemory16(0xffffff0a);
    s.chaos = memory.readMemory8(0xffffffb0);
    s.superEmeralds = memory.readMemory8(0xffffffb1);
    s.shield = memory.readMemory8(0xffffb02b) & 0x71;
    s.checkpoint = memory.readMemory8(0xfffffe2a) & 0x7f;
    s.x = memory.readMemory16(0xffffb010);
    s.y = memory.readMemory16(0xffffb014);
    s.cameraY = memory.readMemory16(0xffffee7c);
    s.input = memory.readMemory8(0xfffff604);
    s.transformed = memory.readMemory8(0xfffffe19) != 0 || memory.readMemory8(0xfffff667) != 0;
    s.countersActive = memory.readMemory8(0xfffff600) == 0x0c;
    s.active = s.countersActive && memory.readMemory8(0xfffffe48) == 0;
    s.blueRemaining = memory.readMemory16(0xffffe438);
    s.ringsRemaining = memory.readMemory16(0xffffe442);
    // Exclude bonus-stage coordinates and counters from regular-act checks.
    s.active = s.active && physicalZoneAct < 0x0d00;
    if (event == retroachievements::COMPETITION_FINISH)
        s.character = memory.readMemory8(0xffffff8b);
    mRetroAchievements.process(static_cast<retroachievements::Event>(event), s, a, b,
        [this](uint32 id) { setAchievementComplete(id); });
}

bool Game::setSharedModeSaves(bool enabled)
{
    auto& config = Configuration::instance();
    if (config.mSharedModeSaves == enabled) return true;
    // Switching a live script's backing storage would mix profiles.
    if (mMode == Mode::NORMAL_GAME || mMode == Mode::ACT_SELECT || mReplayPlaying) return false;
    const auto root = ClassicModeRules::saveRoot(config.mAppDataPath, config.mClassicMode, enabled);
    if (!FTX::FileSystem->exists(root + L"storage/") && !FTX::FileSystem->createDirectory(root + L"storage/"))
    {
        PlatformFunctions::showMessageBox("Save profile", "Could not open the save folder. The profile was not changed.");
        return false;
    }
    mPlayerProgress.save(true);
    PersistentData::instance().updatePersistentData();
    config.mSharedModeSaves = enabled;
    loadModeProfile();
    config.saveSettings();
    return true;
}

std::string Game::replayCompatibility() const
{
    std::string result = "S3AIR-iOS-0.2.5-replay1:";
    result += EngineMain::getDelegate().getAppMetaData().mBuildVersionString;
    for (const auto* module : Application::instance().getSimulation().getCodeExec().getLemonScriptProgram().getModules())
        result += ":" + std::to_string(module->getCompiledCodeHash());
    for (const Mod* mod : ModManager::instance().getActiveMods())
    {
        result += "\n" + mod->mUniqueID + ":" + mod->mDirectoryName + ":" + mod->mModVersion;
        for (const auto& category : mod->mSettingCategories)
            for (const auto& setting : category.mSettings)
                result += "\n" + setting.mIdentifier + "=" + std::to_string(setting.mCurrentValue);
    }
    return result;
}

void Game::beginReplayRecording()
{
    if (!ConfigurationImpl::instance().mReplayMode || (mMode != Mode::NORMAL_GAME && mMode != Mode::ACT_SELECT)) return;
    mReplay = {};
    mReplay.zoneAct = mEmulatorInterface->readMemory16(0xffffee4e);
    if (!SharedDatabase::getZoneByInternalIndex(mReplay.zoneAct >> 8)) return;
    mReplay.classic = Configuration::instance().mClassicMode;
    mReplay.speedrun = ConfigurationImpl::instance().mSpeedrunMode;
    mReplay.normalGame = isInNormalGameMode();
    mReplay.frequency = uint32(Application::instance().getSimulation().getSimulationFrequency());
    mReplay.cutsceneFrames = mSkippableCutsceneFrames;
    mReplay.timestamp = uint64(std::time(nullptr));
    mReplay.compatibility = replayCompatibility();
    ControlsIn::instance().writeCurrentState(mReplay.initialInput.data());
    mReplayCurrentSettings.clear();
    for (const auto& pair : SharedDatabase::getSettings())
    {
        const uint32 value = getSetting(pair.first, false);
        mReplay.settings.emplace_back(pair.first, value);
        mReplayCurrentSettings[pair.first] = value;
    }
    mReplayCurrentSettings[0xffffffff] = uint32(mReplay.speedrun);
    for (const auto& pair : PersistentData::instance().getFiles())
        for (const auto& entry : pair.second.mEntries)
            mReplay.persistent.push_back({pair.second.mFilePath, entry.mKey, entry.mData});
    SaveStateSerializer serializer(Application::instance().getSimulation(), RenderParts::instance());
    mReplayRecording = serializer.saveState(mReplay.state);
}

void Game::completeReplayRecording(uint16 zoneAndAct)
{
    if (mReplayPlaying) return; // Playback ends at its recorded frame count, including the final frame.
    if (!mReplayRecording || mReplayFinishPending || zoneAndAct != mReplay.zoneAct) return;
    mReplay.actTime = (mEmulatorInterface->readMemory8(0xfffffe23) * 60 +
        mEmulatorInterface->readMemory8(0xfffffe24)) * 60 + mEmulatorInterface->readMemory8(0xfffffe25);
    mReplayFinishPending = true;
}

bool Game::finishReplayRecording(bool save)
{
    if (save)
    {
        const auto directory = Configuration::instance().mAppDataPath + L"replays/";
        const auto bytes = mReplay.encode();
        if (bytes.empty() || (!FTX::FileSystem->exists(directory) && !FTX::FileSystem->createDirectory(directory))) return false;
        std::wstring filename;
        for (uint32 index = 1; ; ++index)
        {
            filename = std::to_wstring(mReplay.timestamp) + L"_" + std::to_wstring(index);
            if (!FTX::FileSystem->exists(directory + filename + L".s3rp")) break;
        }
        // A temporary file is never listed; a failed save leaves the prompt open for retry.
        if (!FTX::FileSystem->saveFile(directory + filename + L".tmp", bytes) ||
            !FTX::FileSystem->renameFile(directory + filename + L".tmp", directory + filename + L".s3rp"))
        {
            PlatformFunctions::showMessageBox("Replay", "Could not save the replay. Try again or choose Do Not Save.");
            return false;
        }
    }
    mReplay = {};
    return true;
}

std::vector<Game::ReplayEntry> Game::listReplays()
{
    std::vector<ReplayEntry> result;
    std::vector<rmx::FileIO::FileEntry> files;
    FTX::FileSystem->listFilesByMask(Configuration::instance().mAppDataPath + L"replays/*.s3rp", false, files);
    for (const auto& file : files)
    {
        if (file.mSize > s3air::ReplayData::MAX_BYTES) continue;
        std::vector<uint8> bytes;
        ReplayEntry entry;
        entry.filename = file.mPath + file.mFilename;
        if (FTX::FileSystem->readFile(entry.filename, bytes) && s3air::ReplayData::decode(bytes, entry.data))
        {
            // The browser needs only metadata, not all recordings resident in memory.
            entry.data.frames.clear(); entry.data.previousInputs.clear(); entry.data.state.clear(); entry.data.persistent.clear(); entry.data.settings.clear(); entry.data.events.clear();
            result.push_back(std::move(entry));
        }
    }
    std::sort(result.begin(), result.end(), [](const ReplayEntry& a, const ReplayEntry& b) {
        return a.data.zoneAct != b.data.zoneAct ? a.data.zoneAct < b.data.zoneAct : a.data.timestamp > b.data.timestamp;
    });
    return result;
}

bool Game::deleteReplay(const std::wstring& filename)
{
    // Only files returned by the replay browser are accepted.
    const auto entries = listReplays();
    for (const auto& entry : entries) if (entry.filename == filename) return FTX::FileSystem->removeFile(filename);
    return false;
}

bool Game::playReplay(const std::wstring& filename)
{
    bool found = false;
    for (const auto& entry : listReplays()) if (entry.filename == filename) { found = true; break; }
    if (!found) return false;
    s3air::ReplayData replay;
    std::vector<uint8> bytes;
    if (!FTX::FileSystem->readFile(filename, bytes) || !s3air::ReplayData::decode(bytes, replay) ||
        replay.compatibility != replayCompatibility())
    {
        PlatformFunctions::showMessageBox("Replay", "This replay is damaged or needs the original game version and mods.");
        return false;
    }
    stopReplay();
    auto& config = ConfigurationImpl::instance();
    mPlayerProgress.save(true);
    PersistentData::instance().updatePersistentData();
    mMode = replay.normalGame ? Mode::NORMAL_GAME : Mode::ACT_SELECT;
    startIntoGameInternal();
    mReplay = std::move(replay);
    mReplayOldClassic = config.mClassicMode;
    mReplayOldSpeedrun = config.mSpeedrunMode;
    mReplayOldSettings = config.mActiveGameSettings;
    config.mClassicMode = mReplay.classic;
    config.mSpeedrunMode = mReplay.speedrun;
    config.mActiveGameSettings = &config.mAlternativeGameSettings;
    for (const auto& setting : mReplay.settings) config.mActiveGameSettings->setValue(setting.first, setting.second);
    auto& persistent = PersistentData::instance();
    persistent.setTransient(true);
    persistent.clear();
    for (const auto& entry : mReplay.persistent) persistent.setData(entry.file, entry.key, entry.bytes);
    mReplayPlaying = true;
    mPlayerProgress.reset(); mPlayerProgress.load();
    mReplayFrame = 0;
    mReplaySettingEvent = 0;
    mSkippableCutsceneFrames = mReplay.cutsceneFrames;
    auto& simulation = Application::instance().getSimulation();
    VideoOut::instance().reset();
    AudioOut::instance().reset();
    SaveStateSerializer serializer(simulation, RenderParts::instance());
    if (!serializer.loadState(mReplay.state))
    {
        stopReplay();
        PlatformFunctions::showMessageBox("Replay", "The recorded game state could not be loaded.");
        return false;
    }
    simulation.getGameRecorder().clear();
    simulation.getCodeExec().reinitRuntime(nullptr, CodeExec::CallStackInitPolicy::USE_EXISTING);
    simulation.setSimulationFrequencyOverride(float(mReplay.frequency));
    simulation.setSpeed(1.0f);
    VideoOut::instance().setScreenSize(ClassicModeRules::gameSize(config.mClassicMode));
    ControlsIn::instance().setAllIgnores();
    return true;
}

void Game::stopReplay()
{
    if (!mReplayPlaying) return;
    mReplayPlaying = false;
    auto& config = ConfigurationImpl::instance();
    config.mClassicMode = mReplayOldClassic;
    config.mSpeedrunMode = mReplayOldSpeedrun;
    config.mActiveGameSettings = mReplayOldSettings;
    auto& persistent = PersistentData::instance();
    persistent.clear();
    persistent.setTransient(false);
    persistent.loadFromBasePath(config.mPersistentDataBasePath);
    mPlayerProgress.reset(); mPlayerProgress.load();
    mReplay = {};
    endSkippableCutscene();
    Application::instance().getSimulation().disableSimulationFrequencyOverride();
}
