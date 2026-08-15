#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>

using namespace geode::prelude;

static bool g_botEnabled = false;
static bool g_jumpHeld   = false;

static cocos2d::CCNode* getObjectLayer(PlayLayer* pl) {
    return pl->getChildByTag(9);
}

static bool hazardAhead(cocos2d::CCNode* objectLayer,
                        const cocos2d::CCRect& dangerZone) {
    if (!objectLayer) return false;
    auto& children = *objectLayer->getChildren();
    for (unsigned int i = 0; i < children.count(); ++i) {
        auto obj = dynamic_cast<GameObject*>(children.objectAtIndex(i));
        if (!obj) continue;
        if (obj->m_objectType != GameObjectType::Hazard &&
            obj->m_objectType != GameObjectType::AnimatedHazard)
            continue;
        if (obj->boundingBox().intersectsRect(dangerZone))
            return true;
    }
    return false;
}

class $modify(AIPauseLayer, PauseLayer) {

    static PauseLayer* create(bool p0) {
        auto* layer = PauseLayer::create(p0);
        if (!layer) return layer;

        auto winSize = CCDirector::sharedDirector()->getWinSize();
        auto* self   = static_cast<AIPauseLayer*>(layer);

        auto* bg = CCScale9Sprite::create("GJ_button_01.png");
        bg->setContentSize({ 160.f, 40.f });

        auto* lbl = CCLabelBMFont::create(
            g_botEnabled ? "AI Bot: ON" : "AI Bot: OFF",
            "bigFont.fnt"
        );
        lbl->setScale(0.55f);
        lbl->setColor(g_botEnabled ? ccColor3B{0,255,80} : ccColor3B{255,80,80});
        lbl->setPosition({ 80.f, 20.f });
        bg->addChild(lbl, 1);
        bg->setID("ai-bot-label-bg");

        auto* btn = CCMenuItemSpriteExtra::create(bg, self, menu_selector(AIPauseLayer::onBotToggle));
        btn->setID("ai-bot-toggle-btn");

        auto* menu = CCMenu::create();
        menu->setID("ai-bot-menu");
        menu->addChild(btn);
        menu->setPosition(winSize.width / 2.f, winSize.height * 0.12f);
        layer->addChild(menu, 100);

        log::info("[GD AI Bot] Pause button added. winSize={},{}", winSize.width, winSize.height);
        return layer;
    }

    void onBotToggle(CCObject*) {
        g_botEnabled = !g_botEnabled;
        g_jumpHeld   = false;

        if (auto* menu = this->getChildByID("ai-bot-menu")) {
            if (auto* btn = static_cast<CCMenu*>(menu)->getChildByID("ai-bot-toggle-btn")) {
                auto* item = static_cast<CCMenuItemSpriteExtra*>(btn);
                auto* bg   = static_cast<CCScale9Sprite*>(item->getNormalImage());
                if (bg) {
                    auto* lbl = static_cast<CCLabelBMFont*>(bg->getChildren()->objectAtIndex(0));
                    if (lbl) {
                        lbl->setString(g_botEnabled ? "AI Bot: ON" : "AI Bot: OFF");
                        lbl->setColor(g_botEnabled ? ccColor3B{0,255,80} : ccColor3B{255,80,80});
                    }
                }
            }
        }
        log::info("[GD AI Bot] Toggled: {}", g_botEnabled ? "ON" : "OFF");
    }
};

class $modify(AIPlayLayer, PlayLayer) {
    void update(float dt) {
        PlayLayer::update(dt);
        if (!g_botEnabled) return;
        if (!m_player1 || m_player1->m_isDead) return;
        const float LOOKAHEAD = 120.f;
        const float HEIGHT    = 80.f;
        auto playerBox = m_player1->boundingBox();
        cocos2d::CCRect dangerZone(playerBox.getMaxX(), playerBox.getMinY() - 20.f, LOOKAHEAD, HEIGHT);
        auto* objLayer = getObjectLayer(this);
        bool  danger   = hazardAhead(objLayer, dangerZone);
        if (danger && !g_jumpHeld) { handleButton(true, 1, false); g_jumpHeld = true; }
        else if (!danger && g_jumpHeld) { handleButton(false, 1, false); g_jumpHeld = false; }
    }
    void resetLevel() { g_jumpHeld = false; PlayLayer::resetLevel(); }
};

$on_mod(Loaded) { log::info("[GD AI Bot] Loaded. Pause to toggle."); }
