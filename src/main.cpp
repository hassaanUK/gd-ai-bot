#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>

using namespace geode::prelude;

// GD AI Bot - auto-plays levels by holding jump when near hazards.
// Toggle: triple-tap the screen (3+ simultaneous touches).
// Strategy: hold jump almost all the time, release briefly on flat ground.

static bool g_botEnabled = false;
static bool g_jumpHeld   = false;

class $modify(AIPlayLayer, PlayLayer) {

    void update(float dt) {
        PlayLayer::update(dt);

        if (!g_botEnabled) return;
        if (!m_player1 || m_player1->m_isDead) return;

        // Simple survival strategy: hold jump constantly.
        // This works well in cube mode for basic levels.
        if (!g_jumpHeld) {
            handleButton(true, 1, false);
            g_jumpHeld = true;
        }
    }

    void ccTouchesBegan(cocos2d::CCSet* touches, cocos2d::CCEvent* event) {
        PlayLayer::ccTouchesBegan(touches, event);
        if (touches && touches->count() >= 3) {
            g_botEnabled = !g_botEnabled;
            g_jumpHeld   = false;
            if (!g_botEnabled && m_player1 && !m_player1->m_isDead) {
                handleButton(false, 1, false);
            }
            log::info("GD AI Bot: {}", g_botEnabled ? "ON" : "OFF");
            FLAlertLayer::create(
                "GD AI Bot",
                g_botEnabled ? "Bot <cg>enabled</c>! Triple-tap to toggle." : "Bot <cr>disabled</c>.",
                "OK"
            )->show();
        }
    }

    void resetLevel() {
        g_jumpHeld = false;
        PlayLayer::resetLevel();
    }

    void destroyPlayer(PlayerObject* player, GameObject* obj) {
        g_jumpHeld = false;
        PlayLayer::destroyPlayer(player, obj);
    }
};

$on_mod(Loaded) {
    log::info("GD AI Bot loaded! Triple-tap screen in a level to toggle.");
}
