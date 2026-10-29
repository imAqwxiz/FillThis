#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>

using namespace geode::prelude;

static constexpr bool CREATE_FLAG = true;

static const char* MODE_NAMES[] = {
    "Solid", "Hollow", "Checkerboard", "Hollow checkerboard"
};

class $modify(FillUI, EditorUI) {
    void createMoveMenu() {
        EditorUI::createMoveMenu();

        auto fillBtn = this->getSpriteButton(
            "GJ_plusBtn_001.png",
            menu_selector(FillUI::onFill),
            nullptr,
            0.9f
        );
        fillBtn->setID("fill-button"_spr);

        auto modeBtn = this->getSpriteButton(
            "GJ_optionsBtn_001.png",
            menu_selector(FillUI::onMode),
            nullptr,
            0.9f
        );
        modeBtn->setID("fill-mode-button"_spr);

        m_editButtonBar->m_buttonArray->addObject(fillBtn);
        m_editButtonBar->m_buttonArray->addObject(modeBtn);

        auto rows = GameManager::get()->getIntGameVariable("0049");
        auto cols = GameManager::get()->getIntGameVariable("0050");
        m_editButtonBar->reloadItems(rows, cols);
    }

    void onMode(CCObject*) {
        int mode = (Mod::get()->getSavedValue<int>("fill-mode", 0) + 1) % 4;
        Mod::get()->setSavedValue<int>("fill-mode", mode);
        Notification::create(
            fmt::format("Fill mode: {}", MODE_NAMES[mode]),
            NotificationIcon::Info
        )->show();
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

        int mode = Mod::get()->getSavedValue<int>("fill-mode", 0);
        bool hollow = (mode & 1) != 0;
        bool checker = (mode & 2) != 0;
        float step = static_cast<float>(Mod::get()->getSettingValue<double>("step"));
        if (step < 1.f) step = 30.f;

        int id = a->m_objectID;
        auto pa = a->getPosition();
        auto pb = b->getPosition();

        float minX = std::min(pa.x, pb.x);
        float maxX = std::max(pa.x, pb.x);
        float minY = std::min(pa.y, pb.y);
        float maxY = std::max(pa.y, pb.y);

        int cols = static_cast<int>((maxX - minX) / step + 0.5f) + 1;
        int rows = static_cast<int>((maxY - minY) / step + 0.5f) + 1;
        if (cols * rows > 5000) {
            Notification::create("Area too big (max 5000 blocks)", NotificationIcon::Error)->show();
            return;
        }

        auto isCorner = [&](CCPoint p) {
            return (std::abs(p.x - pa.x) < 0.5f && std::abs(p.y - pa.y) < 0.5f)
                || (std::abs(p.x - pb.x) < 0.5f && std::abs(p.y - pb.y) < 0.5f);
        };

        auto placedObjs = CCArray::create();
        for (int i = 0; i < cols; i++) {
            for (int j = 0; j < rows; j++) {
                if (hollow && !(i == 0 || j == 0 || i == cols - 1 || j == rows - 1)) continue;
                if (checker && ((i + j) % 2 != 0)) continue;

                CCPoint pos = { minX + i * step, minY + j * step };
                if (isCorner(pos)) continue;

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
            fmt::format("Filled {} blocks ({})", placedObjs->count(), MODE_NAMES[mode]),
            NotificationIcon::Success
        )->show();
    }
};
