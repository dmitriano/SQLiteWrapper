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
        
        Iterator(Statement& s) : m_i(s)
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
            return !m_v;
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
            if (m_i.Next())
            {
                T val;
                m_i.Get(val);

                m_v = std::move(val);

                return true;
            }

            m_v = {};

            return false;
        }

        T* cur() const
        {
            T& val = *m_v;

            return &val;
        }

        T& cur_ref() const
        {
            return *m_v;
        }

        HeterogeneousIterator m_i;

        mutable std::optional<T> m_v;
    };

    template <class T>
    auto make_range(Statement& s)
    {
        return std::ranges::subrange(Iterator<T>(s), IteratorSentinel<T>{});
    }
}
