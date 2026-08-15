#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>

using namespace geode::prelude;

static float s_lookahead = 200.f;
static float s_vTol      = 80.f;
static bool  s_holding   = false;

static const std::unordered_set<int> HAZARDS = {
    8, 39, 103, 172, 180, 185, 755, 1055, 1705
};

static bool isEnabled() {
    return Mod::get()->getSettingValue<bool>("enabled");
}

static bool shouldJump(PlayerObject* player, PlayLayer* pl) {
    if (!player || !pl) return false;
    if (player->m_isShip || player->m_isBird || player->m_isDart) return true;

    auto pPos = player->getPosition();
    auto* objects = pl->m_objects;
    if (!objects) return false;

    CCObject* raw;
    CCARRAY_FOREACH(objects, raw) {
        auto* obj = static_cast<GameObject*>(raw);
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
        if (!isEnabled()) {
            if (s_holding) {
                releaseButton(1, false);
                s_holding = false;
            }
            return;
        }

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
        if (s_holding) {
            releaseButton(1, false);
            s_holding = false;
        }
    }

    void onQuit() {
        PlayLayer::onQuit();
        if (s_holding) {
            releaseButton(1, false);
            s_holding = false;
        }
    }
};

$on_mod(Loaded) {
    s_lookahead = Mod::get()->getSettingValue<double>("lookahead");
    s_vTol      = Mod::get()->getSettingValue<double>("vertical-tolerance");
    log::info("GD AI Bot loaded. Lookahead={}px vTol={}px. Toggle in mod settings.",
              s_lookahead, s_vTol);
}
