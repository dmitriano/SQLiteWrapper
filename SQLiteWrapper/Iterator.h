#pragma once

#include "HeterogeneousIterator.h"

#include <optional>
#include <ranges>

namespace sqlite
{
    template <class T>
    struct IteratorSentinel {};

    template <class T>
    class Iterator
    {
    public:

        using iterator_category = std::input_iterator_tag;

        using value_type = T;

        using difference_type = std::ptrdiff_t;

        using pointer = value_type*;

        using reference = value_type&;

        Iterator() = default;
        
        Iterator(Statement& s) : _i(s)
        {
            try_next();
        }

        Iterator(const Iterator&) = delete;

        Iterator& operator = (const Iterator&) = delete;

        Iterator(Iterator&& other) = default;

        Iterator& operator = (Iterator&& other) = default;

        T* operator-> () { return cur(); }

        T& operator* () const { return cur_ref(); }

        bool operator== (const IteratorSentinel<T>&) const noexcept
        {
            return !_v;
        }

        Iterator& operator++ ()
        {
            this->try_next();

            return *this;
        }

        void operator++ (int)
        {
            return ++(*this);
        }

    private:

        bool try_next()
        {
            if (_i.next())
            {
                T val;
                _i.get(val);

                _v = std::move(val);

                return true;
            }

            _v = {};

            return false;
        }

        T* cur() const
        {
            T& val = *_v;

            return &val;
        }

        T& cur_ref() const
        {
            return *_v;
        }

        HeterogeneousIterator _i;

        mutable std::optional<T> _v;
    };

    template <class T>
    auto make_range(Statement& s)
    {
        return std::ranges::subrange(Iterator<T>(s), IteratorSentinel<T>{});
    }
}
