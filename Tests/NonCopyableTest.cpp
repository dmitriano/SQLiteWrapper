#include "DbContainer.h"
#include "SQLiteWrapper/Bind.h"
#include "SQLiteWrapper/Get.h"
#include "SQLiteWrapper/QueryBuilder.h"
#include "SQLiteWrapper/SetStorage.h"

#include "Awl/IntRange.h"

#include <string>

using namespace swtest;

namespace
{
    class NonCopyable
    {
    public:

        NonCopyable() : m_a(0) {}
        
        explicit NonCopyable(int a) : m_a(a)
        {
            ++count;
        }

        ~NonCopyable()
        {
            --count;
        }

        NonCopyable(NonCopyable const&) = delete;

        NonCopyable(NonCopyable&& other) : NonCopyable(other.m_a)
        {
            other.m_moved = true;
        }

        NonCopyable& operator = (const NonCopyable&) = delete;

        NonCopyable& operator = (NonCopyable&& other)
        {
            m_a = other.m_a;
            other.m_moved = true;

            return *this;
        }

        bool operator == (const NonCopyable& other) const
        {
            return m_a == other.m_a;
        }

        bool operator != (const NonCopyable& other) const
        {
            return !operator==(other);
        }

        bool operator < (const NonCopyable& other) const
        {
            return m_a < other.m_a;
        }

        static inline int count = 0;

        int value() const
        {
            return m_a;
        }

    private:

        bool m_moved = false;
        int m_a;
    };

    using Nc = NonCopyable;

    struct Bot
    {
        sqlite::RowId rowId;
        std::string name;
        std::vector<uint8_t> state;
        Nc nc;

        Bot() = default;
        
        Bot(sqlite::RowId rid, std::string n, std::vector<uint8_t> s, Nc c) :
            rowId(std::move(rid)), name(std::move(n)), state(std::move(s)), nc(std::move(c))
        {}

        Bot(const Bot&) = delete;
        Bot& operator = (const Bot&) = delete;

        Bot(Bot&&) = default;
        Bot& operator = (Bot&&) = default;

        AWL_STRINGIZABLE(name, rowId, state, nc)
    };

    AWL_MEMBERWISE_EQUATABLE(Bot);

    static_assert(std::input_iterator<sqlite::Iterator<NonCopyable>>);

    static_assert(std::input_iterator<sqlite::Iterator<Bot>>);
}

namespace sqlite
{
    inline void Bind(Statement & st, size_t col, const Nc& val)
    {
        sqlite::Bind(st, col, val.value());
    }
}

AWT_TEST(NonCopyable)
{
    AWT_UNUSED_CONTEXT;
    
    //Check if helper functions do not require copy constructor and copy assignment.
    {
        Bot bot;
        
        awl::for_each(bot.as_tuple(), [](auto& field) { static_cast<void>(field);});

        sqlite::helpers::ForEachFieldValue(bot, [](auto& field, auto fieldIndex)
        {
            static_cast<void>(field);
            static_cast<void>(fieldIndex);
        });
    }

    {
        const std::string table_name = "nc_bots";
        
        DbContainer c(context);

        sqlite::SetStorage set(c.m_db, table_name, std::make_tuple(&Bot::rowId));
        set.Create();
        set.Prepare();

        set.Insert(Bot{ 0, "BTC_USDT", {}, Nc(0) });
        
        //bot.rowId = c.db().GetLastRowId();
    }
    
}
