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
        
        HeterogeneousIterator(Statement& s) : m_s(&s) {}

        HeterogeneousIterator(const HeterogeneousIterator&) = delete;

        HeterogeneousIterator& operator = (const HeterogeneousIterator&) = delete;

        HeterogeneousIterator(HeterogeneousIterator&& other) noexcept : m_s(std::move(other.m_s))
        {
            other.m_s = nullptr;
        }

        HeterogeneousIterator& operator = (HeterogeneousIterator&& other) noexcept
        {
            close();

            m_s = std::move(other.m_s);

            other.m_s = nullptr;

            return *this;
        }

        bool operator== (const HeterogeneousIterator& other) const = delete;
        bool operator!= (const HeterogeneousIterator& other) const = delete;

        bool Next()
        {
            return m_s->Next();
        }

        template <class T>
        void get(size_t col, T& val)
        {
            sqlite::get(*m_s, col, val);
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
            if (m_s != nullptr)
            {
                m_s->clearBindings();
                m_s->reset();

                m_s = nullptr;
            }
        }

        Statement* m_s = nullptr;
    };
}
