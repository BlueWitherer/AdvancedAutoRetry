#include <Geode/Geode.hpp>

#include <Geode/ui/GeodeUI.hpp>

#include <Geode/utils/terminate.hpp>

#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>

#include <Geode/binding/PlayLayer.hpp>
#include <Geode/binding/PauseLayer.hpp>
#include <Geode/binding/GameManager.hpp>
#include <Geode/binding/PlayerObject.hpp>

using namespace geode::prelude;

// it's modding time :3
auto getThisMod = getMod();

class $modify(MyPlayLayer, PlayLayer) {
    // modify destroyPlayer to make the mod work
    void destroyPlayer(PlayerObject * player, GameObject * object) {
        auto gameManager = GameManager::get();
        bool enableMod = getThisMod->getSettingValue<bool>("enable");

        // check for auto retry in vanilla settings
        bool isAutoRetry = gameManager->getGameVariable("0026");

        if (isAutoRetry && enableMod) {
            // percent threshold
            int normalPercent = as<int>(getThisMod->getSettingValue<int64_t>("normal-mode"));

            // conditions
            bool newBestEnabled = getThisMod->getSettingValue<bool>("new-best"); // only disable auto retry on new best
            bool startPosEnabled = getThisMod->getSettingValue<bool>("start-pos"); // allow disable auto retry on testmode

            int currentPercent = this->getCurrentPercentInt();
            bool shouldDisableAutoRetry = false;

            // check for classic
            if (player->m_isPlatformer) {
                log::error("Level is a platformer");
            } else {
                log::debug("Checking to disable auto retry...");

                // if the player wants no auto retry on startpos
                auto startPosIsOk = !player->m_isStartPos || (player->m_isStartPos && startPosEnabled);

                // check testmode
                if (startPosIsOk) {
                    if (m_isPracticeMode) {
                        log::error("Level is in practice mode");
                    } else {
                        // detect new best
                        if (newBestEnabled) {
                            int bestPercent = m_level->m_normalPercent.value(); // vanilla best percent
                            if (currentPercent > bestPercent) shouldDisableAutoRetry = true;
                            if (shouldDisableAutoRetry) log::info("Player reached a new best at {}%", currentPercent);
                        } else {
                            // manual percentage
                            log::debug("Player crashed at {}%", currentPercent);
                            if (currentPercent >= normalPercent) shouldDisableAutoRetry = true;
                        };
                    };
                } else {
                    log::error("Player is playing from a startpos");
                };
            };

            // handle new auto retry
            if (shouldDisableAutoRetry) {
                log::debug("Disabling auto retry...");

                // quickly disable and re-enable auto retry
                gameManager->setGameVariable("0026", false);
                PlayLayer::destroyPlayer(player, object);
                gameManager->setGameVariable("0026", isAutoRetry);

                log::info("Auto-retry re-enabled");
            } else {
                log::debug("Skipping auto retry function");
                PlayLayer::destroyPlayer(player, object);
            };
        } else {
            log::error("Player has auto retry disabled in vanilla or mod settings");
            PlayLayer::destroyPlayer(player, object);
        };
    };
};

class $modify(MyPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();

        // set up the advanced auto retry button
        if (PlayLayer::get()->m_levelSettings->m_platformerMode) {
            log::error("Level is a platformer");
        } else {
            auto rightMenu = this->getChildByID("right-button-menu");

            // mod settings button sprite
            auto autoRetryModBtnSprite = CCSprite::createWithSpriteFrameName("GJ_getSongInfoBtn_001.png");
            autoRetryModBtnSprite->setScale(0.75f);

            // create mod settings button
            auto autoRetryModBtn = CCMenuItemSpriteExtra::create(
                autoRetryModBtnSprite,
                this,
                menu_selector(MyPauseLayer::onAdvAutoRetry)
            );
            autoRetryModBtn->setID("settings-button"_spr);

            rightMenu->addChild(autoRetryModBtn);
            rightMenu->updateLayout();
        };
    };

    void onAdvAutoRetry(CCObject*) {
        openSettingsPopup(getThisMod);
    };
};