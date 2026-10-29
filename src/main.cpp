#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>

using namespace geode::prelude;

// If undo removes blocks one at a time after a fill,
// flip this to the opposite value.
static constexpr bool CREATE_FLAG = true;

class $modify(FillUI, EditorUI) {
    void createMoveMenu() {
        EditorUI::createMoveMenu();

        auto btn = this->getSpriteButton(
            "GJ_plusBtn_001.png",
            menu_selector(FillUI::onFill),
            nullptr,
            0.9f
        );
        btn->setID("fill-button"_spr);

        m_editButtonBar->m_buttonArray->addObject(btn);

        auto rows = GameManager::get()->getIntGameVariable("0049");
        auto cols = GameManager::get()->getIntGameVariable("0050");
        m_editButtonBar->reloadItems(rows, cols);
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

        auto placedObjs = CCArray::create();
        for (int i = 0; i < cols; i++) {
            for (int j = 0; j < rows; j++) {
                CCPoint pos = { minX + i * 30.f, minY + j * 30.f };
                if (pos.equals(pa) || pos.equals(pb)) continue;
                auto obj = m_editorLayer->createObject(id, pos, CREATE_FLAG);
                if (obj) {
                    obj->setRotation(a->getRotation());
                    placedObjs->addObject(obj);
                }
            }
        }

        if (placedObjs->count() > 0) {
            auto undo = UndoObject::createWithArray(placedObjs, UndoCommand::Paste);
            m_editorLayer->addToUndoList(undo, false);
        }

        Notification::create(
            fmt::format("Filled {} blocks", placedObjs->count()),
            NotificationIcon::Success
        )->show();
    }
};
