#pragma once

#include <vector>

namespace sqlite
{
    //We need to construct it from a tuple of field pointers.
    class IndexFilter
    {
    public:

        using iterator = std::vector<size_t>::const_iterator;

        IndexFilter() = default;

        IndexFilter(std::initializer_list<size_t> init) : m_v(init) {}

        iterator begin() const { return m_v.begin(); }

        iterator end() const { return m_v.end(); }

        bool contains(size_t val) const
        {
            return std::find(m_v.begin(), m_v.end(), val) != m_v.end();
        }

        void insert(size_t val)
        {
            if (contains(val))
            {
                throw std::runtime_error("Duplicated index.");
            }

            m_v.push_back(val);
        }

        bool erase(size_t val)
        {
            auto i = std::find(m_v.begin(), m_v.end(), val);

            if (i != m_v.end())
            {
                m_v.erase(i);

                return true;
            }

            return false;
        }

        size_t size() const { return m_v.size(); }

        bool empty() const
        {
            return m_v.empty();
        }

        size_t index_of(size_t val) const
        {
            auto i = std::find(m_v.begin(), m_v.end(), val);

            if (i == m_v.end())
            {
                throw std::runtime_error("Element not found.");
            }

            return i - m_v.begin();
        }

    private:

        // The vector should not be sorted.
        // The elements should stay in the order we added them in, see Set::BindKey.
        std::vector<size_t> m_v;
    };
}
