#pragma once

#include "SQLiteWrapper/Statement.h"
#include "SQLiteWrapper/Get.h"

#include <cassert>

namespace sqlite
{
    class HeterogeneousIterator
    {
    public:

        // Iterator needs it to be default constructible.
        HeterogeneousIterator() = default;
        
        HeterogeneousIterator(Statement& s) : _s(&s) {}

        HeterogeneousIterator(const HeterogeneousIterator&) = delete;

        HeterogeneousIterator& operator = (const HeterogeneousIterator&) = delete;

        HeterogeneousIterator(HeterogeneousIterator&& other) noexcept : _s(std::move(other._s))
        {
            other._s = nullptr;
        }

        HeterogeneousIterator& operator = (HeterogeneousIterator&& other) noexcept
        {
            close();

            _s = std::move(other._s);

            other._s = nullptr;

            return *this;
        }

        bool operator== (const HeterogeneousIterator& other) const = delete;
        bool operator!= (const HeterogeneousIterator& other) const = delete;

        bool next()
        {
            return _s->next();
        }

        template <class T>
        void get(size_t col, T& val)
        {
            sqlite::get(*_s, col, val);
        }

        template <class T>
        void get(T& val)
        {
            get(0, val);
        }

        ~HeterogeneousIterator()
        {
            close();
        }

    private:

        void close()
        {
            //Allow the statement to be reused with new parameters, for example.
            if (_s != nullptr)
            {
                _s->clearBindings();
                _s->reset();

                _s = nullptr;
            }
        }

        Statement* _s = nullptr;
    };
}
