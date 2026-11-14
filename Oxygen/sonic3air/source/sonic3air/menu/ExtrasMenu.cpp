/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "sonic3air/pch.h"
#include "sonic3air/menu/ExtrasMenu.h"
#include "sonic3air/menu/GameApp.h"
#include "sonic3air/menu/MenuBackground.h"
#include "sonic3air/menu/SharedResources.h"
#include "sonic3air/Game.h"
#include "sonic3air/audio/AudioOut.h"
#include "sonic3air/data/SharedDatabase.h"

#include "oxygen/application/Configuration.h"
#include "oxygen/application/gameview/GameView.h"
#include "oxygen/application/input/InputManager.h"
#include "oxygen/engine/EngineMain.h"
#include "oxygen/helper/FileHelper.h"
#include "oxygen/helper/Utils.h"
#include "oxygen/platform/PlatformFunctions.h"
#include "sonic3air/data/TimeAttackData.h"
#include <ctime>


namespace
{
	static constexpr int BACK = 0xffff;
	static constexpr int REPLAYS = 0xfffe;

	bool showExtrasMenuTabContent(int tabIndex)
	{
		if (tabIndex == 2)
		{
			// Achievements are hidden if debug mode is active (but not for dev mode)
			if (!EngineMain::getDelegate().useDeveloperFeatures())
			{
				return (Game::instance().getSetting(SharedDatabase::Setting::SETTING_DEBUG_MODE, true) == 0);
			}
		}
		return true;
	}
}


ExtrasMenu::ExtrasMenu(MenuBackground& menuBackground) :
	mMenuBackground(&menuBackground)
{
	mScrolling.setVisibleAreaHeight(224 - 30);	// Do not count the 30 pixels of the tab title as scrolling area

	// Build up tab menu entries
	mTabMenuEntries.addEntry("", 0)
					.addOption("EXTRAS",	   Tab::Id::EXTRAS)
					.addOption("SECRETS",	   Tab::Id::SECRETS)
					.addOption("ACHIEVEMENTS", Tab::Id::ACHIEVEMENTS);

	for (int i = 0; i < 3; ++i)
	{
		GameMenuEntries& entries = mTabs[i].mMenuEntries;
		entries.reserve(20);
		entries.addEntry();		// Dummy entry representing the title in menu navigation
	}

	// All tabs will be rebuilt in each "initialize"
}

ExtrasMenu::~ExtrasMenu()
{
}

GameMenuBase::BaseState ExtrasMenu::getBaseState() const
{
	switch (mState)
	{
		case State::APPEAR:		   return BaseState::FADE_IN;
		case State::SHOW:		   return BaseState::SHOW;
		case State::FADE_TO_MENU:  return BaseState::FADE_OUT;
		case State::FADE_TO_GAME:  return BaseState::FADE_OUT;
		default:				   return BaseState::INACTIVE;
	}
}

void ExtrasMenu::setBaseState(BaseState baseState)
{
	switch (baseState)
	{
		case BaseState::INACTIVE: mState = State::INACTIVE;  break;
		case BaseState::FADE_IN:  mState = State::APPEAR;  break;
		case BaseState::SHOW:	  mState = State::SHOW;  break;
		case BaseState::FADE_OUT: mState = State::FADE_TO_MENU;  break;
	}
}

void ExtrasMenu::onFadeIn()
{
	mBrowsingReplays = false;
	mReplayLaunchFilename.clear();
	mReplayDeleteIndex = -1;
	mDescriptionLinesCache.clear();
	mState = State::APPEAR;

	// (Re)count completed achievements
	mAchievementsCompleted = 0;
	for (const SharedDatabase::Achievement& achievement : SharedDatabase::getAchievements())
	{
		if (PlayerProgress::instance().mAchievements.getAchievementState(achievement.mType) != 0)
			++mAchievementsCompleted;
	}

	// Rebuild extras, because extras from hidden secrets may have been added
	{
		GameMenuEntries& entries = mTabs[0].mMenuEntries;
		entries.resize(1);

		for (const SharedDatabase::Secret& secret : SharedDatabase::getSecrets())
		{
			if (!secret.mShownInMenu)
				continue;

			// Only certain secrets produce an extra
			if (secret.mType == SharedDatabase::Secret::SECRET_COMPETITION_MODE ||
				secret.mType == SharedDatabase::Secret::SECRET_BLUE_SPHERE ||
				secret.mType == SharedDatabase::Secret::SECRET_LEVELSELECT)
			{
				if (PlayerProgress::instance().mUnlocks.isSecretUnlocked(secret.mType))
				{
					entries.addEntry(secret.mName, secret.mType);
				}
			}
		}

		entries.addEntry("Replays", REPLAYS);
		entries.addEntry("Back", BACK);
	}

	// Rebuild secrets, because hidden secrets may have been added
	{
		GameMenuEntries& entries = mTabs[1].mMenuEntries;
		entries.resize(1);

		for (const SharedDatabase::Secret& secret : SharedDatabase::getSecrets())
		{
			if (!secret.mShownInMenu)
				continue;
			if (!secret.mSerialized)	// Excludes the Competition Mode secret, which is not a secret at all actually
				continue;
			if (secret.mHiddenUntilUnlocked && !PlayerProgress::instance().mUnlocks.isSecretUnlocked(secret.mType))
				continue;

			entries.addEntry(secret.mName, secret.mType);
		}

		entries.addEntry("Back", BACK);
	}

	// Rebuild achievements, because their order (but not their number) can change over time
	{
		GameMenuEntries& entries = mTabs[2].mMenuEntries;
		entries.resize(1);

		for (int pass = 0; pass < 2; ++pass)
		{
			for (const SharedDatabase::Achievement& achievement : SharedDatabase::getAchievements())
			{
				const bool isComplete = (PlayerProgress::instance().mAchievements.getAchievementState(achievement.mType) != 0);
				if (isComplete == (pass == 1))
				{
					entries.addEntry(achievement.mName, achievement.mType);
				}
			}
		}

		entries.addEntry("Back", BACK);
	}

	mMenuBackground->showPreview(false, false);
	mMenuBackground->startTransition(MenuBackground::Target::BLUE);

	for (size_t k = 0; k < 3; ++k)
	{
		for (size_t i = 0; i < mTabs[k].mMenuEntries.size(); ++i)
		{
			mTabs[k].mMenuEntries[i].mAnimation.mHighlight = 0.0f;
		}
	}

	AudioOut::instance().setMenuMusic(0x2f);
}

bool ExtrasMenu::canBeRemoved()
{
	return (mState == State::INACTIVE && mVisibility <= 0.0f);
}

void ExtrasMenu::initialize()
{
}

void ExtrasMenu::deinitialize()
{
}

void ExtrasMenu::keyboard(const rmx::KeyboardEvent& ev)
{
}

void ExtrasMenu::update(float timeElapsed)
{
	mScrolling.setVisibleAreaHeight(std::max(1, mRect.height - 30));
	GameMenuBase::update(timeElapsed);

	mActiveTabAnimated += clamp((float)mActiveTab - mActiveTabAnimated, -timeElapsed * 4.0f, timeElapsed * 4.0f);

	// Don't react to input during transitions
	if (mState == State::SHOW && mBrowsingReplays) updateReplayBrowser();
	else if (mState == State::SHOW)
	{
		const InputManager::ControllerScheme& keys = InputManager::instance().getController(0);

		if (mActiveMenu == &mTabMenuEntries && (keys.Down.justPressedOrRepeat() || keys.Up.justPressedOrRepeat()) && showExtrasMenuTabContent((int)mActiveTab))
		{
			// Switch from title to tab content
			mActiveMenu = &mTabs[mActiveTab].mMenuEntries;
			mActiveMenu->mSelectedEntryIndex = 0;
		}

		// Update menu entries
		const GameMenuEntries::UpdateResult result = mActiveMenu->update();
		if (result != GameMenuEntries::UpdateResult::NONE)
		{
			mRightPressCounter = 0;
			if (result == GameMenuEntries::UpdateResult::OPTION_CHANGED && mActiveMenu == &mTabMenuEntries)
			{
				mActiveTab = mTabMenuEntries[0].mSelectedIndex;
				playMenuSound(0xb7);
			}
			else
			{
				playMenuSound(0x5b);

				if (result == GameMenuEntries::UpdateResult::ENTRY_CHANGED && mActiveMenu != &mTabMenuEntries && mActiveMenu->mSelectedEntryIndex == 0)
				{
					// Switch from tab content to title
					mActiveMenu = &mTabMenuEntries;
				}
			}
		}

		enum class ButtonEffect
		{
			NONE,
			ACCEPT,
			BACK,
			RIGHT
		};
		const ButtonEffect buttonEffect = (keys.Start.justPressed() || keys.A.justPressed() || keys.X.justPressed()) ? ButtonEffect::ACCEPT :
										  (keys.Back.justPressed() || keys.B.justPressed()) ? ButtonEffect::BACK :
										   keys.Right.justPressed() ? ButtonEffect::RIGHT : ButtonEffect::NONE;

		if (buttonEffect != ButtonEffect::NONE)
		{
			if (buttonEffect == ButtonEffect::RIGHT)
			{
				if (mActiveTab == 1)
				{
					++mRightPressCounter;
					if (mRightPressCounter == 5)
					{
						const GameMenuEntry& selectedEntry = mTabs[mActiveTab].mMenuEntries.selected();
						if (!PlayerProgress::instance().mUnlocks.isSecretUnlocked(selectedEntry.mData))
						{
							Game::instance().unlockSecret(selectedEntry.mData);
						}
					}
				}
			}
			else if (buttonEffect == ButtonEffect::BACK || mActiveMenu == &mTabMenuEntries)
			{
				goBack();
			}
			else
			{
				Tab& tab = mTabs[mActiveTab];
				const GameMenuEntry& selectedEntry = tab.mMenuEntries.selected();
				if (mActiveTab == 0 && selectedEntry.mData == REPLAYS)
				{
					mBrowsingReplays = true;
					mReplayZone = -1;
					mReplays = Game::instance().listReplays();
					rebuildReplayEntries();
				}
				else if (mActiveTab == 0 &&
					PlayerProgress::instance().mUnlocks.isSecretUnlocked(selectedEntry.mData) &&
					(selectedEntry.mData == SharedDatabase::Secret::SECRET_COMPETITION_MODE ||
					 selectedEntry.mData == SharedDatabase::Secret::SECRET_BLUE_SPHERE ||
					 selectedEntry.mData == SharedDatabase::Secret::SECRET_LEVELSELECT))
				{
					playMenuSound(0xaf);
					GameApp::instance().getGameView().startFadingOut();
					mState = State::FADE_TO_GAME;
				}
				else
				{
					goBack();
				}
			}
		}
	}

	// Update animation
	for (size_t k = 0; k < 3; ++k)
	{
		const float maxStep = timeElapsed * 10.0f;
		for (size_t i = 0; i < mTabs[k].mMenuEntries.size(); ++i)
		{
			const float target = ((int)i == mTabs[k].mMenuEntries.mSelectedEntryIndex && k == mActiveTab) ? 1.0f : 0.0f;
			mTabs[k].mMenuEntries[i].mAnimation.mHighlight += clamp(target - mTabs[k].mMenuEntries[i].mAnimation.mHighlight, -maxStep, maxStep);
		}
	}

	// Scrolling
	mScrolling.update(timeElapsed);

	// Fading in/out
	if (mState == State::APPEAR)
	{
		if (updateFadeIn(timeElapsed * 6.0f))
		{
			mState = State::SHOW;
		}
	}
	else if (mState > State::SHOW)
	{
		if (updateFadeOut(timeElapsed * 4.0f))
		{
			if (!mReplayLaunchFilename.empty())
			{
				const auto filename = mReplayLaunchFilename;
				mReplayLaunchFilename.clear();
				if (Game::instance().playReplay(filename))
				{
					GameApp::instance().onStartGame();
					mMenuBackground->setGameStartedMenu();
				}
				else
				{
					mState = State::SHOW;
					mVisibility = 1.0f;
					GameApp::instance().getGameView().setFadedIn();
					return;
				}
			}
			else if (mState == State::FADE_TO_GAME && mActiveTab == 0)
			{
				const GameMenuEntry& selectedEntry = mTabs[mActiveTab].mMenuEntries.selected();
				if (PlayerProgress::instance().mUnlocks.isSecretUnlocked(selectedEntry.mData))
				{
					switch (selectedEntry.mData)
					{
						case SharedDatabase::Secret::SECRET_COMPETITION_MODE:
							startCompetitionMode();
							break;

						case SharedDatabase::Secret::SECRET_BLUE_SPHERE:
							startBlueSphere();
							break;

						case SharedDatabase::Secret::SECRET_LEVELSELECT:
							startLevelSelect();
							break;
					}
				}
			}
			mState = State::INACTIVE;
		}
	}
}

void ExtrasMenu::render()
{
	if (mBrowsingReplays) { renderReplayBrowser(); return; }
	GuiBase::render();

	Drawer& drawer = EngineMain::instance().getDrawer();

	int anchorX = mRect.width / 2;
	const int cardHalfWidth = std::min(185, (mRect.width - 24) / 2);
	float alpha = 1.0f;
	if (mState != State::SHOW && mState != State::FADE_TO_GAME)
	{
		anchorX += roundToInt((1.0f - mVisibility) * 300.0f);
		alpha = mVisibility;
	}

	const int startY = 30 - mScrolling.getScrollOffsetYInt();

	drawer.pushScissor(Recti(0, 30, (int)mRect.width, (int)mRect.height - 30));

	const int minTabIndex = (int)std::floor(mActiveTabAnimated);
	const int maxTabIndex = (int)std::ceil(mActiveTabAnimated);

	for (int tabIndex = minTabIndex; tabIndex <= maxTabIndex; ++tabIndex)
	{
		const Tab& tab = mTabs[tabIndex];

		const float tabAlpha = alpha * (1.0f - std::fabs(tabIndex - mActiveTabAnimated));
		const int baseX = anchorX + roundToInt((tabIndex - mActiveTabAnimated) * 250);
		int py = startY + 12;
		int section = 0;

		if (!showExtrasMenuTabContent(tabIndex))
		{
			const char* text2 = EngineMain::getDelegate().useDeveloperFeatures() ? "because Dev Mode is active" : "because Debug Mode is active";
			drawer.printText(global::mSonicFontB, Vec2i(baseX, py + 22), "Achievements are locked", 5, Color(1.0f, 0.75f, 0.5f, alpha));
			drawer.printText(global::mSonicFontB, Vec2i(baseX, py + 35), text2, 5, Color(1.0f, 0.75f, 0.5f, alpha));

			if (mTailsYawning.getWidth() == 0)
			{
				FileHelper::loadTexture(mTailsYawning, L"data/images/menu/tails_yawning.png");
			}
			drawer.drawRect(Recti(baseX - mTailsYawning.getWidth() / 2, py + 50, mTailsYawning.getWidth(), mTailsYawning.getHeight()), mTailsYawning, Color(1.0f, 1.0f, 1.0f, alpha));

			mTabs[tabIndex].mMenuEntries.mSelectedEntryIndex = 0;
			continue;
		}

		for (size_t line = 1; line < tab.mMenuEntries.size(); ++line)
		{
			const auto& entry = tab.mMenuEntries[line];
			const bool isBack = (entry.mData == BACK);

			const bool isSelected = (mActiveMenu == &tab.mMenuEntries && (int)line == tab.mMenuEntries.mSelectedEntryIndex);
			const Color color = isSelected ? Color(1.0f, 1.0f, 0.0f, tabAlpha) : Color(1.0f, 1.0f, 1.0f, tabAlpha);

			int currentAbsoluteY1 = py - startY;

			if (isBack)
			{
				py += (tabIndex < 2) ? 22 : 16;
				drawer.printText(global::mOxyfontRegular, Recti(baseX, py, 0, 10), entry.mText, 5, color);
				py += 16;
			}
			else if (tabIndex == Tab::Id::EXTRAS && entry.mData == REPLAYS)
			{
				py += 18;
				drawer.printText(global::mSonicFontC, Recti(baseX, py, 0, 20), "REPLAYS", 5, color);
				py += 26;
			}
			else if (tabIndex == Tab::Id::EXTRAS)
			{
				// Extras
				const SharedDatabase::Secret* secret = SharedDatabase::getSecret(entry.mData);
				RMX_ASSERT(nullptr != secret, "Invalid secret ID");

				py += 8;
				const int localStartY = py;
				const int px = baseX + roundToInt(interpolate(12.0f, 0.0f, entry.mAnimation.mHighlight));

				// Background
				DrawerTexture& frameTexture = global::mAchievementsFrame;
				DrawerTexture& secretImage = global::mSecretImage[secret->mType];
				drawer.drawRect(Recti(px - cardHalfWidth, py, std::min((int)frameTexture.getWidth(), mRect.width - 24), frameTexture.getHeight()), frameTexture, Color(1.0f, 1.0f, 1.0f, tabAlpha * 0.5f));
				drawer.drawRect(Recti(px - cardHalfWidth, py, secretImage.getWidth() + 4, secretImage.getHeight() + 4), secretImage, Color(0.12f, 0.12f, 0.12f, tabAlpha));
				drawer.drawRect(Recti(px - cardHalfWidth + 2, py + 2, secretImage.getWidth(), secretImage.getHeight()), secretImage, Color(1.0f, 1.0f, 1.0f, tabAlpha));
				py += 7;

				// Title
				String name(secret->mName);
				name.upperCase();
				drawer.printText(global::mSonicFontC, Recti(px - cardHalfWidth + 82, py + 12, 0, 10), name, 1, color);

				py = localStartY + 56;
				if (isSelected)
				{
					const Color color3(1.0f, 1.0f - std::fabs(std::fmod(FTX::getTime(), 1.0f) - 0.5f) * 2.0f, 0.0f, entry.mAnimation.mHighlight);
					drawer.printText(global::mOxyfontSmall, Recti(px + roundToInt(interpolate(50.0f, 0.0f, entry.mAnimation.mHighlight)), py - 11, 0, 10), "Press Enter to start", 2, color3);
				}
				py -= 8;
			}
			else if (tabIndex == Tab::Id::SECRETS)
			{
				// Secrets
				if (line == 1)
				{
					py += 15;
					drawer.printText(global::mSonicFontB, Recti(baseX, py, 0, 10), String(0, "%d of %d achievements completed", mAchievementsCompleted, (int)SharedDatabase::getAchievements().size()), 5, Color(0.6f, 0.8f, 1.0f, alpha));
					py += 16;
				}

				const SharedDatabase::Secret* secret = SharedDatabase::getSecret(entry.mData);
				RMX_ASSERT(nullptr != secret, "Invalid secret ID");
				const bool isUnlocked = PlayerProgress::instance().mUnlocks.isSecretUnlocked(entry.mData);

				py += 8;
				const int localStartY = py;
				const int px = baseX + roundToInt(interpolate(12.0f, 0.0f, entry.mAnimation.mHighlight));

				// Background
				DrawerTexture& frameTexture = global::mAchievementsFrame;
				DrawerTexture& secretImage = isUnlocked ? global::mSecretImage[secret->mType] : global::mSecretImage[secret->mType | 0x80000000];
				drawer.drawRect(Recti(px - cardHalfWidth, py, std::min((int)frameTexture.getWidth(), mRect.width - 24), frameTexture.getHeight()), frameTexture, Color(1.0f, 1.0f, 1.0f, tabAlpha));
				drawer.drawRect(Recti(px - cardHalfWidth + 2, py + 2, secretImage.getWidth(), secretImage.getHeight()), secretImage, Color(1.0f, 1.0f, 1.0f, tabAlpha));
				py += 7;

				// Title
				String name(isUnlocked ? secret->mName : "???");
				name.upperCase();
				drawer.printText(global::mOxyfontRegular, Recti(px - cardHalfWidth + 75, py, 0, 10), name, 1, color);

				// Description
				const Color color2 = isSelected ? Color(1.0f, 1.0f, 0.6f, tabAlpha) : Color(0.9f, 0.9f, 0.9f, tabAlpha);
				if (isUnlocked)
				{
					std::vector<std::string_view>* textLines = nullptr;
					{
						const auto it = mDescriptionLinesCache.find(0x1000 + entry.mData);
						if (it == mDescriptionLinesCache.end())
						{
							textLines = &mDescriptionLinesCache[0x1000 + entry.mData];
							utils::splitTextIntoLines(*textLines, secret->mDescription, global::mOxyfontTiny, 260);
						}
						else
						{
							textLines = &it->second;
						}
					}

					py += (textLines->size() == 1) ? 9 : 5;

					for (const std::string_view& textLine : *textLines)
					{
						py += 10;
						drawer.printText(global::mOxyfontTiny, Recti(px - 105, py, 0, 10), textLine, 1, color2);
					}

					py = localStartY + 56;
				}
				else
				{
					String text(0, "Complete %d achievements to unlock", secret->mRequiredAchievements);
					py += 19;
					drawer.printText(global::mOxyfontTiny, Recti(px - 105, py, 0, 10), text, 1, color2);

					py = localStartY + 56;
				}
			}
			else if (tabIndex == Tab::Id::ACHIEVEMENTS)
			{
				// Achievements
				const SharedDatabase::Achievement* achievement = SharedDatabase::getAchievement(entry.mData);
				RMX_ASSERT(nullptr != achievement, "Invalid achievement ID");
				const bool isComplete = (PlayerProgress::instance().mAchievements.getAchievementState(entry.mData) != 0);

				const int newSection = isComplete ? 2 : 1;
				if (section != newSection)
				{
					py += (line == 1) ? 15 : 30;
					drawer.printText(global::mSonicFontB, Recti(baseX, py, 0, 10), isComplete ? "* Completed achievements *" : "* Open achievements *", 5, Color(0.6f, 0.8f, 1.0f, tabAlpha));
					py += 16;
					section = newSection;

					if (line > 1)
					{
						// Refresh position for scrolling once after text, except if it was the first
						currentAbsoluteY1 = py - startY;
					}
				}

				py += 8;
				const int localStartY = py;
				const int px = baseX + roundToInt(interpolate(12.0f, 0.0f, entry.mAnimation.mHighlight));

				// Allow long challenge descriptions to wrap inside either screen width.
				auto& textLines = mDescriptionLinesCache[entry.mData];
				if (textLines.empty())
					utils::splitTextIntoLines(textLines, achievement->mDescription, global::mOxyfontTiny,
						std::min(260, cardHalfWidth * 2 - 90));
				const int cardHeight = std::max(entry.mData >= retroachievements::ID_BASE ? 61 : 52, 27 + (int)textLines.size() * 10);
				const int textX = px - cardHalfWidth + 75;

				DrawerTexture& frameTexture = global::mAchievementsFrame;
				DrawerTexture& achievementImage = global::mAchievementImage[achievement->mType | ((isSelected || isComplete) ? 0 : 0x80000000)];
				drawer.drawRect(Recti(px - cardHalfWidth, py, std::min((int)frameTexture.getWidth(), mRect.width - 24), cardHeight), frameTexture, Color(1.0f, 1.0f, 1.0f, tabAlpha));
				drawer.drawRect(Recti(px - cardHalfWidth + 2, py + 2, achievementImage.getWidth(), achievementImage.getHeight()), achievementImage, Color(1.0f, 1.0f, 1.0f, tabAlpha));
				py += 7;

				String name(achievement->mName);
				name.upperCase();
				if (entry.mData >= retroachievements::ID_BASE && entry.mData < retroachievements::ID_BASE + retroachievements::COUNT)
				{
					const auto& imported = retroachievements::CATALOG[entry.mData - retroachievements::ID_BASE];
					drawer.printText(global::mOxyfontTiny, Recti(px - cardHalfWidth + 34, localStartY + cardHeight - 9, 0, 8), std::to_string(imported.points) + " pts", 5, color);
				}
				drawer.printText(global::mOxyfontRegular, Recti(textX, py, 0, 10), name, 1, color);

				py += (textLines.size() == 1) ? 9 : 5;
				const Color color2 = isSelected ? Color(1.0f, 1.0f, 0.6f, tabAlpha) : Color(0.9f, 0.9f, 0.9f, tabAlpha);
				for (const std::string_view& textLine : textLines)
				{
					py += 10;
					drawer.printText(global::mOxyfontTiny, Recti(textX, py, 0, 10), textLine, 1, color2);
				}
				py = localStartY + cardHeight;
			}
			else
			{
				drawer.printText(global::mOxyfontRegular, Recti(baseX, py, 0, 10), entry.mText, 5, color);
				py += 16;
			}

			if (isSelected)
			{
				const int currentAbsoluteY2 = py - startY;
				mScrolling.setCurrentSelection(currentAbsoluteY1 - 28, currentAbsoluteY2 + 34);
			}
		}
	}

	drawer.popScissor();

	// Tab titles
	{
		// Title background
		drawer.drawRect(Recti(anchorX - 200, -6, 400, 48), global::mOptionsTopBar, Color(1.0f, 1.0f, 1.0f, alpha));

		int py = 4;
		const auto& entry = mTabMenuEntries[0];
		const bool isSelected = (mActiveMenu == &mTabMenuEntries);
		const Color color = isSelected ? Color(1.0f, 1.0f, 0.0f, alpha) : Color(1.0f, 1.0f, 1.0f, alpha);

		const bool canGoLeft  = (entry.mSelectedIndex > 0);
		const bool canGoRight = (entry.mSelectedIndex < entry.mOptions.size() - 1);

		const int center = anchorX;
		int arrowDistance = 100;
		if (isSelected)
		{
			const int offset = (int)std::fmod(FTX::getTime() * 6.0f, 6.0f);
			arrowDistance += ((offset > 3) ? (6 - offset) : offset);
		}

		// Show all tab titles
		for (size_t k = 0; k < entry.mOptions.size(); ++k)
		{
			const Color color2 = (k == entry.mSelectedIndex) ? color : Color(0.9f, 0.9f, 0.9f, alpha * 0.8f);
			const std::string& text = entry.mOptions[k].mText;
			const int px = roundToInt(((float)k - mActiveTabAnimated) * 220.0f) + center - 80;
			drawer.printText(global::mSonicFontC, Recti(px, py, 160, 20), text, 5, color2);
		}

		if (canGoLeft)
			drawer.printText(global::mOxyfontRegular, Recti(center - arrowDistance, py + 6, 0, 10), "<", 5, color);
		if (canGoRight)
			drawer.printText(global::mOxyfontRegular, Recti(center + arrowDistance, py + 6, 0, 10), ">", 5, color);

		py += 36;

		if (isSelected)
		{
			mScrolling.setCurrentSelection(0, py);
		}
	}

	// TEST
	//drawer.drawRect(Recti(0, startY + mCurrentSelectionY1, 400, mCurrentSelectionY2 - mCurrentSelectionY1), Color(0x80000000));

	drawer.performRendering();
}

void ExtrasMenu::startCompetitionMode()
{
	// Init simulation
	Game::instance().startIntoCompetitionMode();
	GameApp::instance().onStartGame();
	mMenuBackground->setGameStartedMenu();
}

void ExtrasMenu::startBlueSphere()
{
	// Init simulation
	Game::instance().startIntoBlueSphere();
	GameApp::instance().onStartGame();
	mMenuBackground->setGameStartedMenu();
}

void ExtrasMenu::startLevelSelect()
{
	// Init simulation
	Game::instance().startIntoLevelSelect();
	GameApp::instance().onStartGame();
	mMenuBackground->setGameStartedMenu();
}

void ExtrasMenu::goBack()
{
	playMenuSound(0xad);
	mMenuBackground->openMainMenu();
	mState = State::FADE_TO_MENU;
}


void ExtrasMenu::rebuildReplayEntries()
{
    mReplayDeleteIndex = -1;
    mReplayEntries.clear();
    if (mReplayZone < 0)
    {
        int previous = -1;
        for (const auto& entry : mReplays)
        {
            const int zoneIndex = entry.data.zoneAct >> 8;
            if (zoneIndex == previous) continue;
            previous = zoneIndex;
            const auto* zone = SharedDatabase::getZoneByInternalIndex(uint8(zoneIndex));
            mReplayEntries.addEntry(zone ? zone->mDisplayName : "Unknown Zone", uint32(zoneIndex));
        }
    }
    else
    {
        for (size_t i = 0; i < mReplays.size(); ++i)
        {
            const auto& entry = mReplays[i];
            if ((entry.data.zoneAct >> 8) != mReplayZone) continue;
            const std::string title = (entry.data.scope == s3air::ReplayScope::FULL_ZONE ? std::string("Full Zone") : "Act " + std::to_string((entry.data.zoneAct & 0xff) + 1)) + "  " +
                TimeAttackData::getTimeString(entry.data.actTime) + (entry.data.classic ? "  Classic" : "  Anniversary");
            mReplayEntries.addEntry(title, uint32(i));
        }
    }
    mReplayEntries.addEntry("Back", BACK);
    mReplayEntries.mSelectedEntryIndex = 0;
}

void ExtrasMenu::updateReplayBrowser()
{
    const auto& keys = InputManager::instance().getController(0);
    const bool accept = keys.A.justPressed() || keys.X.justPressed() || keys.Start.justPressed();
    const bool back = keys.B.justPressed() || keys.Back.justPressed();
    if (mReplayDeleteIndex >= 0)
    {
        mReplayDeleteEntries.update();
        if (back) mReplayDeleteIndex = -1;
        else if (accept)
        {
            if (mReplayDeleteEntries.selected().mData == 1)
            {
                if (!Game::instance().deleteReplay(mReplays[size_t(mReplayDeleteIndex)].filename))
                    PlatformFunctions::showMessageBox("Replay", "Could not delete this replay.");
                mReplays = Game::instance().listReplays();
                rebuildReplayEntries();
            }
            mReplayDeleteIndex = -1;
        }
        return;
    }
    mReplayEntries.update();
    const uint32 selected = mReplayEntries.selected().mData;
    if (back || (accept && selected == BACK))
    {
        if (mReplayZone >= 0) { mReplayZone = -1; rebuildReplayEntries(); }
        else mBrowsingReplays = false;
        return;
    }
    if (selected == BACK) return;
    if (mReplayZone >= 0 && keys.Y.justPressed())
    {
        mReplayDeleteIndex = int(selected);
        mReplayDeleteEntries.clear();
        mReplayDeleteEntries.addEntry("Cancel", 0);
        mReplayDeleteEntries.addEntry("Delete replay", 1);
        mReplayDeleteEntries.mSelectedEntryIndex = 0;
    }
    else if (accept)
    {
        if (mReplayZone < 0) { mReplayZone = int(selected); rebuildReplayEntries(); }
        else
        {
            mReplayLaunchFilename = mReplays[selected].filename;
            GameApp::instance().getGameView().startFadingOut();
            mState = State::FADE_TO_GAME;
        }
    }
}

void ExtrasMenu::renderReplayBrowser()
{
    Drawer& drawer = EngineMain::instance().getDrawer();
    const int center = mRect.width / 2;
    drawer.drawRect(Recti(8, 4, mRect.width - 16, mRect.height - 8), Color(0.02f, 0.05f, 0.14f, mVisibility));
    std::string title = "REPLAYS";
    if (mReplayZone >= 0)
    {
        const auto* zone = SharedDatabase::getZoneByInternalIndex(uint8(mReplayZone));
        if (zone) title = zone->mShortName + " - Replays";
    }
    drawer.printText(global::mOxyfontRegular, Recti(center, 12, 0, 16), title, 5, Color::WHITE);
    const int rowHeight = mReplayZone < 0 ? 24 : 36;
    const int visible = std::max(1, (mRect.height - 76) / rowHeight);
    const int selected = mReplayEntries.mSelectedEntryIndex;
    const int first = std::max(0, selected - visible + 1);
    if (mReplayEntries.size() == 1)
        drawer.printText(global::mOxyfontSmall, Recti(center, 38, 0, 12), "No saved replays", 5, Color::WHITE);
    for (int row = first; row < std::min(int(mReplayEntries.size()), first + visible); ++row)
    {
        const auto& entry = mReplayEntries[size_t(row)];
        const int y = 53 + (row - first) * rowHeight;
        const auto color = row == selected ? Color::YELLOW : Color::WHITE;
        drawer.printText(global::mOxyfontSmall, Recti(center, y, 0, 14), entry.mText, 5, color);
        if (mReplayZone >= 0 && entry.mData != BACK)
        {
            const std::time_t timestamp = std::time_t(mReplays[entry.mData].data.timestamp);
            char text[40] = {};
            const auto* time = std::localtime(&timestamp);
            if (time) std::strftime(text, sizeof(text), "%Y-%m-%d %H:%M:%S", time);
            drawer.printText(global::mOxyfontTiny, Recti(center, y + 12, 0, 12), text, 5, color);
            const auto& data = mReplays[entry.mData].data;
            if (data.scope == s3air::ReplayScope::FULL_ZONE)
            {
                const auto times = "Act 1: " + TimeAttackData::getTimeString(data.actTimes[0]) +
                    "   Act 2: " + TimeAttackData::getTimeString(data.actTimes[1]);
                drawer.printText(global::mOxyfontTiny, Recti(center, y + 23, 0, 10), times, 5, color);
            }
        }
    }
    drawer.printText(global::mOxyfontTiny, Recti(center, mRect.height - 17, 0, 12),
        mReplayZone < 0 ? "A: Choose zone   B: Back" : "A: Play   Y: Delete   B: Back", 5, Color::WHITE);
    if (mReplayDeleteIndex >= 0)
    {
        drawer.drawRect(Recti(20, 65, mRect.width - 40, 96), Color(0.05f, 0.05f, 0.1f, 1.0f));
        drawer.printText(global::mOxyfontRegular, Recti(center, 76, 0, 14), "Delete selected replay?", 5, Color::WHITE);
        for (size_t i = 0; i < mReplayDeleteEntries.size(); ++i)
            drawer.printText(global::mOxyfontSmall, Recti(center, 108 + int(i)*22, 0, 12), mReplayDeleteEntries[i].mText,
                5, int(i) == mReplayDeleteEntries.mSelectedEntryIndex ? Color::YELLOW : Color::WHITE);
    }
    drawer.performRendering();
}
