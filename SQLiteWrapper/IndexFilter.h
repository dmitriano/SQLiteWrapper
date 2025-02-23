#pragma once

#include <set>

namespace sqlite
{
    //We need to construct it from a tuple of field pointers.
    class IndexFilter
    {
    public:

        using iterator = std::set<size_t>::const_iterator;

        IndexFilter() = default;

        IndexFilter(std::initializer_list<size_t> init) : m_set(init) {}

        iterator begin() const { return m_set.begin(); }

        iterator end() const { return m_set.end(); }

        bool contains(size_t val) const
        {
            return m_set.contains(val);
        }

        void insert(size_t val)
        {
            if (!m_set.emplace(val).second)
            {
                throw std::runtime_error("Duplicated index.");
            }
        }

        size_t size() const { return m_set.size(); }

    private:

        std::set<size_t> m_set;
    };
}
