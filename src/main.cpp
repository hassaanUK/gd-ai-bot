/**
 * GD AI Bot - Geode Mod (Android, GD 2.2074)
 * Scans objects ahead of the player every frame.
 * If a hazard is in range: hold jump.
 * If path is clear: release jump.
 * Toggle: tap screen with 3 fingers.
 */

#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>

using namespace geode::prelude;

static bool  s_enabled   = false;
static float s_lookahead = 200.f;
static float s_vTol      = 80.f;
static bool  s_holding   = false;

static const std::unordered_set<int> HAZARDS = {
    8, 39, 103, 172, 180, 185, 755, 1055, 1705
};

static bool shouldJump(PlayerObject* p, PlayLayer* pl) {
    if (!p || !pl) return false;
    if (p->m_isShip || p->m_isBird || p->m_isDart) return true;

    auto pPos = p->getPosition();
    CCObject* raw;
    CCARRAY_FOREACH(pl->m_objects, raw) {
        auto obj = static_cast<GameObject*>(raw);
        if (!obj || !obj->isVisible()) continue;
        auto oPos = obj->getPosition();
        float dx = oPos.x - pPos.x;
        float dy = oPos.y - pPos.y;
        if (dx < 5.f || dx > s_lookahead) continue;
        if (std::abs(dy) > s_vTol) continue;
        if (HAZARDS.count(obj->m_objectID)) return true;
        if (obj->m_objectID <= 4 && dy > -10.f && dy < 70.f && dx < 90.f) return true;
    }
    return false;
}

class $modify(BotPlayLayer, PlayLayer) {
    void update(float dt) {
        PlayLayer::update(dt);
        if (!s_enabled) return;
        auto* p = m_player1;
        if (!p || p->m_isDead) return;
        bool jump = shouldJump(p, this);
        if (jump && !s_holding) {
            pushButton(1, false);
            s_holding = true;
        } else if (!jump && s_holding) {
            releaseButton(1, false);
            s_holding = false;
        }
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        s_holding = false;
    }

    void onQuit() {
        PlayLayer::onQuit();
        s_enabled = false;
        s_holding = false;
    }

    void ccTouchesEnded(CCSet* touches, CCEvent* event) {
        PlayLayer::ccTouchesEnded(touches, event);
        if (touches && touches->count() >= 3) {
            s_enabled = !s_enabled;
            s_holding = false;
            if (!s_enabled) releaseButton(1, false);
            auto msg = s_enabled ? "AI Bot: ON" : "AI Bot: OFF";
            Notification::create(msg, NotificationIcon::Success, 2.0f)->show();
            log::info("{}", msg);
        }
    }
};

$on_mod(Loaded) {
    s_lookahead = Mod::get()->getSettingValue<double>("lookahead");
    s_vTol      = Mod::get()->getSettingValue<double>("vertical-tolerance");
    log::info("GD AI Bot loaded. Lookahead={}px vTol={}px. 3-finger tap to toggle.",
              s_lookahead, s_vTol);
}
