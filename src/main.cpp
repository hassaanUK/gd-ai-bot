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
        auto* self = static_cast<AIPauseLayer*>(layer);
        auto label = CCLabelBMFont::create(
            g_botEnabled ? "AI Bot: ON" : "AI Bot: OFF",
            "bigFont.fnt"
        );
        label->setScale(0.55f);
        label->setColor(g_botEnabled ? ccColor3B{0,255,100} : ccColor3B{255,80,80});
        auto btn = CCMenuItemSpriteExtra::create(label, self, menu_selector(AIPauseLayer::onBotToggle));
        btn->setID("ai-bot-toggle-btn");
        auto menu = CCMenu::create();
        menu->setID("ai-bot-menu");
        menu->addChild(btn);
        menu->setPosition(CCDirector::sharedDirector()->getWinSize().width / 2.f, 38.f);
        layer->addChild(menu, 10);
        return layer;
    }

    void onBotToggle(CCObject*) {
        g_botEnabled = !g_botEnabled;
        g_jumpHeld = false;
        if (auto* menu = this->getChildByID("ai-bot-menu")) {
            if (auto* btn = static_cast<CCMenu*>(menu)->getChildByID("ai-bot-toggle-btn")) {
                auto* item = static_cast<CCMenuItemSpriteExtra*>(btn);
                auto* label = static_cast<CCLabelBMFont*>(item->getNormalImage());
                label->setString(g_botEnabled ? "AI Bot: ON" : "AI Bot: OFF");
                label->setColor(g_botEnabled ? ccColor3B{0,255,100} : ccColor3B{255,80,80});
            }
        }
        log::info("GD AI Bot: {}", g_botEnabled ? "ENABLED" : "DISABLED");
    }
};

class $modify(AIPlayLayer, PlayLayer) {
    void update(float dt) {
        PlayLayer::update(dt);
        if (!g_botEnabled) return;
        if (!m_player1 || m_player1->m_isDead) return;
        const float LOOKAHEAD = 120.f;
        const float HEIGHT = 80.f;
        auto playerBox = m_player1->boundingBox();
        cocos2d::CCRect dangerZone(playerBox.getMaxX(), playerBox.getMinY() - 20.f, LOOKAHEAD, HEIGHT);
        auto* objLayer = getObjectLayer(this);
        bool danger = hazardAhead(objLayer, dangerZone);
        if (danger && !g_jumpHeld) { handleButton(true, 1, false); g_jumpHeld = true; }
        else if (!danger && g_jumpHeld) { handleButton(false, 1, false); g_jumpHeld = false; }
    }
    void resetLevel() { g_jumpHeld = false; PlayLayer::resetLevel(); }
};

$on_mod(Loaded) { log::info("GD AI Bot loaded! Toggle via the pause menu."); }
