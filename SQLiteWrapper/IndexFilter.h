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

        IndexFilter(std::initializer_list<size_t> init) : _v(init) {}

        iterator begin() const { return _v.begin(); }

        iterator end() const { return _v.end(); }

        bool contains(size_t val) const
        {
            return std::find(_v.begin(), _v.end(), val) != _v.end();
        }

        void insert(size_t val)
        {
            if (contains(val))
            {
                throw std::runtime_error("Duplicated index.");
            }

            _v.push_back(val);
        }

        bool erase(size_t val)
        {
            auto i = std::find(_v.begin(), _v.end(), val);

            if (i != _v.end())
            {
                _v.erase(i);

                return true;
            }

            return false;
        }

        size_t size() const { return _v.size(); }

        bool empty() const
        {
            return _v.empty();
        }

        size_t index_of(size_t val) const
        {
            auto i = std::find(_v.begin(), _v.end(), val);

            if (i == _v.end())
            {
                throw std::runtime_error("Element not found.");
            }

            return i - _v.begin();
        }

    private:

        // The vector should not be sorted.
        // The elements should stay in the order we added them in, see Set::bindKey.
        std::vector<size_t> _v;
    };
}
