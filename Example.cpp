// https://github.com/defGameEngine/defGameEngine
#include "defGameEngine.hpp"

#define DEF_QUAD_TREE_IMPL
#include "QuadTree.hpp"

#define DGE_AFFINE_TRANSFORMS
#include "../Extensions/DGE_AffineTransforms.hpp"

class App : public def::GameEngine
{
public:
    App()
    {
        GetWindow()->SetTitle("Quad trees");
        UseOnlyTextures(true);
    }

    enum class PlantID
    {
        LargeTree,
        LargeBush,
        SmallTree,
        SmallBush
    };

    struct Object
    {
        def::rectf area;
        PlantID id;
    };

    QuadTreeContainer<Object> tree;

    def::AffineTransforms at;

    float worldSize = 25000.0f;
    float searchAreaSize = 100.0f;

    def::Graphic plants;

protected:
    bool OnUserCreate() override
    {
        tree.create({ {0.0f, 0.0f}, {worldSize, worldSize} });
        at.SetViewArea(GetWindow()->GetScreenSize());

        auto rand_float = [](float min, float max)
            {
                return min + (float)rand() / (float)RAND_MAX * (max - min);
            };

        for (size_t i = 0; i < 1000000; i++)
        {
            Object o;

            o.area.pos = { rand_float(0.0f, worldSize), rand_float(0.0f, worldSize) };
            o.id = PlantID(rand() % 4);

            o.area.size.x = 16.0f;
            
            if (o.id == PlantID::LargeTree || o.id == PlantID::LargeBush)
                o.area.size.y = 32.0f;
            else
                o.area.size.x = 16.0f;

            tree.insert(o, o.area);
        }

        plants.Load("plants.png");

        return true;
    }

    bool OnUserUpdate(float) override
    {
        auto i = GetInput();

        if (i->GetButtonState(def::Button::WHEEL).pressed)
            at.StartPan(GetInput()->GetMousePosition());

        if (i->GetButtonState(def::Button::WHEEL).held)
            at.UpdatePan(GetInput()->GetMousePosition());

        if (i->GetScrollDelta() > 0)
        {
            if (i->GetKeyState(def::Key::LEFT_CONTROL).held)
                searchAreaSize *= 0.9f;
            else
                at.Zoom(1.1f, i->GetMousePosition());
        }

        if (i->GetScrollDelta() < 0)
        {
            if (i->GetKeyState(def::Key::LEFT_CONTROL).held)
                searchAreaSize *= 1.1f;
            else
                at.Zoom(0.9f, i->GetMousePosition());
        }

        def::Vector2f mouse = at.ScreenToWorld(i->GetMousePosition());
        def::rectf selectedArea({ mouse.x - searchAreaSize * 0.5f, mouse.y - searchAreaSize * 0.5f }, { searchAreaSize, searchAreaSize });

        if (i->GetButtonState(def::Button::LEFT).held)
        {
            std::list<std::list<QuadTreeContainer<Object>::Item>::iterator> selected;
            tree.find(selectedArea, selected);

            for (auto& item : selected)
                tree.remove(item);
        }

        def::Vector2f origin = at.GetOrigin();
        def::Vector2f size = at.GetEnd() - origin;

        def::rectf searchArea = { { origin.x, origin.y }, { size.x, size.y } };

        std::list<std::list<QuadTreeContainer<Object>::Item>::iterator> objects;
        tree.find(searchArea, objects);

        ClearTexture(def::GREEN);

        for (const auto& obj : objects)
        {
            def::Vector2f pos = { obj->data.area.pos.x, obj->data.area.pos.y };

            switch (obj->data.id)
            {
            case PlantID::LargeTree: at.DrawPartialTexture(pos, plants.texture, { 0.0f, 0.0f }, { 16.0f, 32.0f }); break;
            case PlantID::LargeBush: at.DrawPartialTexture(pos, plants.texture, { 16.0f, 0.0f }, { 16.0f, 32.0f }); break;
            case PlantID::SmallTree: at.DrawPartialTexture(pos, plants.texture, { 32.0f, 7.0f }, { 16.0f, 25.0f }); break;
            case PlantID::SmallBush: at.DrawPartialTexture(pos, plants.texture, { 48.0f, 16.0f }, { 16.0f, 16.0f }); break;
            }
        }

        DrawTextureString({ 0, 0 }, std::to_string(objects.size()));
        at.FillTextureRectangle(
            { selectedArea.pos.x, selectedArea.pos.y },
            { selectedArea.size.x, selectedArea.size.y },
            def::Pixel(255, 255, 255, 100));

        return true;
    }
};

int main()
{
    App app;

    if (app.Construct(1280, 960, 1, 1, false, true))
        app.Run();

    return 0;
}
