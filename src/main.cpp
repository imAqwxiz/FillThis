
#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/TextInput.hpp>
#include <Geode/modify/EditorUI.hpp>

using namespace geode::prelude;

static constexpr bool CREATE_FLAG = true;

class FillOptionsPopup : public geode::Popup {
protected:
    bool init() {
        if (!Popup::init(260.f, 180.f)) return false;
        this->setTitle("Fill Options");

        auto mod = Mod::get();

        // Hollow toggle
        auto hollowToggle = CCMenuItemToggler::createWithStandardSprites(
            this, menu_selector(FillOptionsPopup::onHollow), 0.7f
        );
        hollowToggle->toggle(mod->getSavedValue<bool>("fill-hollow", false));
        m_buttonMenu->addChildAtPosition(hollowToggle, Anchor::Center, { -95.f, 30.f });

        auto hollowLabel = CCLabelBMFont::create("Hollow", "bigFont.fnt");
        hollowLabel->setScale(0.6f);
        hollowLabel->setAnchorPoint({ 0.f, 0.5f });
        m_mainLayer->addChildAtPosition(hollowLabel, Anchor::Center, { -70.f, 30.f });

        // Checkerboard toggle
        auto checkToggle = CCMenuItemToggler::createWithStandardSprites(
            this, menu_selector(FillOptionsPopup::onChecker), 0.7f
        );
        checkToggle->toggle(mod->getSavedValue<bool>("fill-checker", false));
        m_buttonMenu->addChildAtPosition(checkToggle, Anchor::Center, { -95.f, -5.f });

        auto checkLabel = CCLabelBMFont::create("Checkerboard", "bigFont.fnt");
        checkLabel->setScale(0.6f);
        checkLabel->setAnchorPoint({ 0.f, 0.5f });
        m_mainLayer->addChildAtPosition(checkLabel, Anchor::Center, { -70.f, -5.f });

        // Block step
        auto stepLabel = CCLabelBMFont::create("Block step", "bigFont.fnt");
        stepLabel->setScale(0.6f);
        stepLabel->setAnchorPoint({ 0.f, 0.5f });
        m_mainLayer->addChildAtPosition(stepLabel, Anchor::Center, { -110.f, -45.f });

        auto stepInput = TextInput::create(80.f, "30", "bigFont.fnt");
        stepInput->setFilter("0123456789.");
        stepInput->setMaxCharCount(6);
        stepInput->setString(fmt::format("{}", mod->getSavedValue<double>("fill-step", 30.0)));
        stepInput->setCallback([](std::string const& str) {
            double v = std::strtod(str.c_str(), nullptr);
            if (v >= 1.0 && v <= 300.0) {
                Mod::get()->setSavedValue<double>("fill-step", v);
            }
        });
        m_mainLayer->addChildAtPosition(stepInput, Anchor::Center, { 55.f, -45.f });

        return true;
    }

    void onHollow(CCObject* sender) {
        auto t = static_cast<CCMenuItemToggler*>(sender);
        Mod::get()->setSavedValue<bool>("fill-hollow", !t->isToggled());
    }

    void onChecker(CCObject* sender) {
        auto t = static_cast<CCMenuItemToggler*>(sender);
        Mod::get()->setSavedValue<bool>("fill-checker", !t->isToggled());
    }

public:
    static FillOptionsPopup* create() {
        auto ret = new FillOptionsPopup();
        if (ret->init()) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }
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

        auto optBtn = this->getSpriteButton(
            "GJ_optionsBtn_001.png",
            menu_selector(FillUI::onOptions),
            nullptr,
            0.9f
        );
        optBtn->setID("fill-options-button"_spr);

        m_editButtonBar->m_buttonArray->addObject(fillBtn);
        m_editButtonBar->m_buttonArray->addObject(optBtn);

        auto rows = GameManager::get()->getIntGameVariable("0049");
        auto cols = GameManager::get()->getIntGameVariable("0050");
        m_editButtonBar->reloadItems(rows, cols);
    }

    void onOptions(CCObject*) {
        FillOptionsPopup::create()->show();
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

        bool hollow = Mod::get()->getSavedValue<bool>("fill-hollow", false);
        bool checker = Mod::get()->getSavedValue<bool>("fill-checker", false);
        float step = static_cast<float>(Mod::get()->getSavedValue<double>("fill-step", 30.0));
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
            fmt::format("Filled {} blocks", placedObjs->count()),
            NotificationIcon::Success
        )->show();
    }
};
