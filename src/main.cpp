#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>

using namespace geode::prelude;

class $modify(FillUI, EditorUI) {
    bool init(LevelEditorLayer* layer) {
        if (!EditorUI::init(layer)) return false;

        auto sprite = CCSprite::createWithSpriteFrameName("GJ_plusBtn_001.png");
        sprite->setScale(0.7f);
        auto btn = CCMenuItemSpriteExtra::create(
            sprite, this, menu_selector(FillUI::onFill)
        );
        btn->setID("fill-button"_spr);

        auto menu = CCMenu::create();
        menu->addChild(btn);
        auto win = CCDirector::get()->getWinSize();
        menu->setPosition({ win.width - 30.f, win.height / 2.f });
        menu->setID("fill-menu"_spr);
        this->addChild(menu);

        return true;
    }

    void onFill(CCObject*) {
        auto selected = this->getSelectedObjects();
        if (!selected || selected->count() != 2) {
            Notification::create("Select exactly 2 corner blocks", NotificationIcon::Warning)->show();
            return;
        }

        auto a = static_cast<GameObject*>(selected->objectAtIndex(0));
        auto b = static_cast<GameObject*>(selected->objectAtIndex(1));

        if (a->m_objectID != b->m_objectID) {
            Notification::create("Corners must be the same block", NotificationIcon::Warning)->show();
            return;
        }

        int id = a->m_objectID;
        auto pa = a->getPosition();
        auto pb = b->getPosition();

        float minX = std::min(pa.x, pb.x);
        float maxX = std::max(pa.x, pb.x);
        float minY = std::min(pa.y, pb.y);
        float maxY = std::max(pa.y, pb.y);

        int cols = static_cast<int>((maxX - minX) / 30.f + 0.5f) + 1;
        int rows = static_cast<int>((maxY - minY) / 30.f + 0.5f) + 1;
        if (cols * rows > 5000) {
            Notification::create("Area too big (max 5000 blocks)", NotificationIcon::Error)->show();
            return;
        }

        int placed = 0;
        for (int i = 0; i < cols; i++) {
            for (int j = 0; j < rows; j++) {
                CCPoint pos = { minX + i * 30.f, minY + j * 30.f };
                // skip the two corners, they already exist
                if (pos.equals(pa) || pos.equals(pb)) continue;
                auto obj = m_editorLayer->createObject(id, pos, false);
                if (obj) {
                    obj->setRotation(a->getRotation());
                    placed++;
                }
            }
        }

        Notification::create(fmt::format("Filled {} blocks", placed), NotificationIcon::Success)->show();
    }
};
