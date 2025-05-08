#ifndef QUAD_TREE_HPP
#define QUAD_TREE_HPP

#include <array>

// https://github.com/defini7/defGeometry2D
#define DEF_GEOMETRY2D_IMPL
#include "defGeometry2D.hpp"

bool overlaps(const def::rectf& r1, const def::rectf& r2);

template <class T>
class QuadTree
{
public:
    using Storage = std::list<std::pair<T, def::rectf>>;

    struct ItemLocation
    {
        typename Storage::iterator iterator;
        Storage* container = nullptr;
    };

public:
    QuadTree(const def::rectf& area = { {0.0f, 0.0f}, {128.0f, 128.0f} }, size_t level = 0);

    void create(const def::rectf& area, size_t level = 0);
    void resize(const def::rectf& area);
    void clear();

    size_t size() const;

    ItemLocation insert(const T& item, const def::rectf& area);
    void find(const def::rectf& area, std::list<T>& data);
    bool remove(const T& item);

    void collect_items(std::list<T>& items);
    void collect_areas(std::list<def::rectf>& areas);

private:
    size_t m_Level;

    def::rectf m_Area;

    // All 4 children of a current quad
    std::array<std::shared_ptr<QuadTree<T>>, 4> m_Children;

    // Cached areas of each item from the m_Children array
    std::array<def::rectf, 4> m_ChildrenAreas;

    // Items in the current quad
    Storage m_Items;
};

template <class T>
class QuadTreeContainer
{
public:
    struct Item
    {
        T data;
        typename QuadTree<typename std::list<Item>::iterator>::ItemLocation location;
    };

    using Storage = std::list<Item>;

    QuadTreeContainer(const def::rectf& area = { {0.0f, 0.0f}, {128.0f, 128.0f} }, size_t level = 0);

    void create(const def::rectf& area = { {0.0f, 0.0f}, {128.0f, 128.0f} }, size_t level = 0);
    void clear();

    size_t size() const;

    void insert(const T& item, const def::rectf& area);
    void find(const def::rectf& area, std::list<typename Storage::iterator>& data);
    void remove(typename Storage::iterator item);

    void collect_items(Storage& items);
    void collect_areas(std::list<def::rectf>& areas);

private:
    Storage m_Items;
    QuadTree<typename Storage::iterator> m_Root;

};

#ifdef DEF_QUAD_TREE_IMPL
#undef DEF_QUAD_TREE_IMPL

bool overlaps(const def::rectf& r1, const def::rectf& r2)
{
    return r1.pos < r2.pos + r2.size && r1.pos + r1.size >= r2.pos;
}

template <class T>
QuadTree<T>::QuadTree(const def::rectf& area, size_t level)
{
    create(area, level);
}

template <class T>
void QuadTree<T>::create(const def::rectf& area, size_t level)
{
    m_Level = level;
    resize(area);
}

template <class T>
void QuadTree<T>::resize(const def::rectf& area)
{
    clear();

    m_Area = area;
    def::vf2d childSize = area.size * 0.5f;

    m_ChildrenAreas =
    {
        def::rectf(area.pos, childSize),
        def::rectf({ area.pos.x + childSize.x, area.pos.y }, childSize),
        def::rectf({ area.pos.x, area.pos.y + childSize.y }, childSize),
        def::rectf(area.pos + childSize, childSize)
    };
}

template <class T>
void QuadTree<T>::clear()
{
    m_Items.clear();

    for (auto& child : m_Children)
    {
        if (child) child->clear();
        child.reset();
    }
}

template <class T>
size_t QuadTree<T>::size() const
{
    size_t count = m_Items.size();

    for (auto& child : m_Children)
    {
        if (child)
            count += child.size();
    }

    return count;
}

template <class T>
QuadTree<T>::ItemLocation QuadTree<T>::insert(const T& item, const def::rectf& area)
{
    for (size_t i = 0; i < 4; i++)
    {
        if (def::contains(m_ChildrenAreas[i], area))
        {
            if (!m_Children[i])
                m_Children[i] = std::make_shared<QuadTree<T>>(m_ChildrenAreas[i], m_Level + 1);

            return m_Children[i]->insert(item, area);
        }
    }

    // It fits within the area of the current child
    // but it doesn't fit within any area of the children
    // so stop here
    m_Items.push_back({ item, area });

    return { std::prev(m_Items.end()), &m_Items };
}

template <class T>
void QuadTree<T>::find(const def::rectf& area, std::list<T>& data)
{
    for (const auto& item : m_Items)
    {
        if (overlaps(area, item.second))
            data.push_back(item.first);
    }

    for (size_t i = 0; i < 4; i++)
    {
        if (m_Children[i])
        {
            if (def::contains(area, m_ChildrenAreas[i]))
                m_Children[i]->collect_items(data);

            else if (overlaps(m_ChildrenAreas[i], area))
                m_Children[i]->find(area, data);
        }
    }
}

template <class T>
bool QuadTree<T>::remove(const T& item)
{
    auto it = std::find_if(m_Items.begin(), m_Items.end(),
                           [&](const std::pair<T, def::rectf>& i) { return i.first == item; });

    if (it != m_Items.end())
    {
        m_Items.erase(it);
        return true;
    }

    for (size_t i = 0; i < 4; i++)
    {
        if (m_Children[i])
        {
            if (m_Children[i]->remove(item))
                return true;
        }
    }

    return false;
}

template <class T>
void QuadTree<T>::collect_items(std::list<T>& items)
{
    for (const auto& item : m_Items)
        items.push_back(item.first);

    for (auto& child : m_Children)
    {
        if (child)
            child->collect_items(items);
    }
}

template <class T>
void QuadTree<T>::collect_areas(std::list<def::rectf>& areas)
{
    areas.push_back(m_Area);

    for (size_t i = 0; i < 4; i++)
    {
        if (m_Children[i])
            m_Children[i]->collect_areas(areas);
    }
}

template <class T>
QuadTreeContainer<T>::QuadTreeContainer(const def::rectf& area, size_t level)
{
    create(area, level);
}

template <class T>
void QuadTreeContainer<T>::create(const def::rectf& area, size_t level)
{
    m_Root.create(area, level);
}

template <class T>
void QuadTreeContainer<T>::clear()
{
    m_Root.clear();
}

template <class T>
size_t QuadTreeContainer<T>::size() const
{
    return m_Root.size();
}

template <class T>
void QuadTreeContainer<T>::insert(const T& item, const def::rectf& area)
{
    m_Items.push_back({ item });
    m_Items.back().location = m_Root.insert(std::prev(m_Items.end()), area);
}

template <class T>
void QuadTreeContainer<T>::find(const def::rectf& area, std::list<typename Storage::iterator>& data)
{
    m_Root.find(area, data);
}

template <class T>
void QuadTreeContainer<T>::remove(typename Storage::iterator item)
{
    item->location.container->erase(item->location.iterator);
    m_Items.erase(item);
}

template <class T>
void QuadTreeContainer<T>::collect_items(Storage& items)
{
    m_Root.collect_items(items);
}

template <class T>
void QuadTreeContainer<T>::collect_areas(std::list<def::rectf>& areas)
{
    m_Root.collect_areas(areas);
}

#endif

#endif
