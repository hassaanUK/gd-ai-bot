#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/CCKeyboardDispatcher.hpp>

using namespace geode::prelude;

static bool g_botEnabled = false;
static bool g_jumpHeld   = false;

static float getLookahead() { return (float)Mod::get()->getSettingValue<double>("lookahead"); }
static float getHeight()    { return (float)Mod::get()->getSettingValue<double>("height"); }

static bool hazardAhead(PlayLayer* pl) {
    auto* objLayer = pl->getChildByTag(9);
    if (!objLayer) return false;
    auto* player = pl->m_player1;
    if (!player) return false;
    CCPoint playerWorld = player->getParent()
        ? player->getParent()->convertToWorldSpace(player->getPosition())
        : player->getPosition();
    float lookahead = getLookahead();
    float height    = getHeight();
    float playerHW = player->getContentWidth() * player->getScaleX() * 0.5f;
    CCRect zone(
        playerWorld.x + playerHW,
        playerWorld.y - height * 0.5f,
        lookahead,
        height
    );
    auto& children = *objLayer->getChildren();
    for (unsigned int i = 0; i < children.count(); ++i) {
        auto* obj = dynamic_cast<GameObject*>(children.objectAtIndex(i));
        if (!obj || !obj->isVisible()) continue;
        if (obj->m_objectType != GameObjectType::Hazard &&
            obj->m_objectType != GameObjectType::AnimatedHazard)
            continue;
        CCPoint objWorld = objLayer->convertToWorldSpace(obj->getPosition());
        float hw = obj->getContentWidth()  * obj->getScaleX() * 0.5f;
        float hh = obj->getContentHeight() * obj->getScaleY() * 0.5f;
        CCRect objRect(objWorld.x - hw, objWorld.y - hh, hw * 2.f, hh * 2.f);
        if (zone.intersectsRect(objRect))
            return true;
    }
    return false;
}

static void refreshPauseBtn(CCNode* layer) {
    auto* menu = static_cast<CCMenu*>(layer->getChildByID("ai-bot-menu"));
    if (!menu) return;
    auto* btn = static_cast<CCMenuItemSpriteExtra*>(menu->getChildByID("ai-bot-toggle-btn"));
    if (!btn) return;
    auto* bg = static_cast<CCScale9Sprite*>(btn->getNormalImage());
    if (!bg) return;
    auto* lbl = static_cast<CCLabelBMFont*>(bg->getChildren()->objectAtIndex(0));
    if (!lbl) return;
    lbl->setString(g_botEnabled ? "AI Bot: ON" : "AI Bot: OFF");
    lbl->setColor(g_botEnabled ? ccColor3B{50,255,100} : ccColor3B{255,80,80});
}

class $modify(AIPauseLayer, PauseLayer) {
    static PauseLayer* create(bool p0) {
        auto* layer = PauseLayer::create(p0);
        if (!layer) return layer;
        auto* self   = static_cast<AIPauseLayer*>(layer);
        auto winSize = CCDirector::sharedDirector()->getWinSize();
        auto* bg = CCScale9Sprite::create("GJ_button_01.png");
        bg->setContentSize({ 140.f, 36.f });
        auto* lbl = CCLabelBMFont::create(
            g_botEnabled ? "AI Bot: ON" : "AI Bot: OFF",
            "bigFont.fnt"
        );
        lbl->setScale(0.5f);
        lbl->setColor(g_botEnabled ? ccColor3B{50,255,100} : ccColor3B{255,80,80});
        lbl->setPosition({ 70.f, 18.f });
        bg->addChild(lbl, 1);
        auto* btn = CCMenuItemSpriteExtra::create(bg, self, menu_selector(AIPauseLayer::onBotToggle));
        btn->setID("ai-bot-toggle-btn");
        auto* menu = CCMenu::create();
        menu->setID("ai-bot-menu");
        menu->setTouchPriority(-500);
        menu->addChild(btn);
        menu->setPosition(winSize.width / 2.f, 30.f);
        layer->addChild(menu, 500);
        log::info("[GD AI Bot] Pause btn added. winSize=({},{})", winSize.width, winSize.height);
        return layer;
    }
    void onBotToggle(CCObject*) {
        g_botEnabled = !g_botEnabled;
        g_jumpHeld   = false;
        if (!g_botEnabled) {
            auto* pl = PlayLayer::get();
            if (pl && pl->m_player1 && !pl->m_player1->m_isDead)
                pl->handleButton(false, 1, false);
        }
        refreshPauseBtn(this);
        log::info("[GD AI Bot] Pause toggle: {}", g_botEnabled ? "ON" : "OFF");
    }
};

class $modify(AIKeyboard, CCKeyboardDispatcher) {
    bool dispatchKeyboardMSG(enumKeyCodes key, bool isKeyDown, bool isKeyRepeat) {
        bool result = CCKeyboardDispatcher::dispatchKeyboardMSG(key, isKeyDown, isKeyRepeat);
        if (key == enumKeyCodes::KEY_B && isKeyDown && !isKeyRepeat) {
            auto* pl = PlayLayer::get();
            if (pl && !pl->m_isPaused) {
                g_botEnabled = !g_botEnabled;
                g_jumpHeld   = false;
                if (!g_botEnabled && pl->m_player1 && !pl->m_player1->m_isDead)
                    pl->handleButton(false, 1, false);
                log::info("[GD AI Bot] Key toggle: {}", g_botEnabled ? "ON" : "OFF");
            }
        }
        return result;
    }
};

class $modify(AIPlayLayer, PlayLayer) {
    void update(float dt) {
        PlayLayer::update(dt);
        if (!g_botEnabled) return;
        if (!m_player1 || m_player1->m_isDead) return;
        bool danger = hazardAhead(this);
        if (danger && !g_jumpHeld) { handleButton(true, 1, false); g_jumpHeld = true; }
        else if (!danger && g_jumpHeld) { handleButton(false, 1, false); g_jumpHeld = false; }
    }
    void resetLevel() { g_jumpHeld = false; PlayLayer::resetLevel(); }
};

$on_mod(Loaded) {
    log::info("[GD AI Bot] v1.1 loaded. Lookahead={}px", getLookahead());
}
