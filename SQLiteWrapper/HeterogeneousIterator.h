#pragma once

#include "SQLiteWrapper/Statement.h"
#include "SQLiteWrapper/Get.h"

#include <cassert>

namespace sqlite
{
    class HeterogeneousIterator
    {
    public:

        HeterogeneousIterator() = default;
        
        HeterogeneousIterator(Statement& s) : m_s(&s)
        {
        }

        HeterogeneousIterator(const HeterogeneousIterator&) = delete;

        HeterogeneousIterator& operator = (const HeterogeneousIterator&) = delete;

        HeterogeneousIterator(HeterogeneousIterator&& other) noexcept : m_s(std::move(other.m_s))
        {
            other.m_s = nullptr;
        }

        HeterogeneousIterator& operator = (HeterogeneousIterator&& other) noexcept
        {
            Close();

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
        void Get(size_t col, T & val)
        {
            sqlite::Get(*m_s, col, val);
        }

        template <class T>
        void Get(T & val)
        {
            Get(0, val);
        }

        ~HeterogeneousIterator()
        {
            Close();
        }

    private:

        void Close()
        {
            //Allow the statement to be reused with new parameters, for example.
            if (m_s != nullptr)
            {
                m_s->ClearBindings();
                m_s->Reset();

                m_s = nullptr;
            }
        }

        Statement* m_s;
    };
}
