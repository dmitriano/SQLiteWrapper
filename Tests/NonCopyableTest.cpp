#include "Tests/DbContainer.h"
#include "Tests/TableHelper.h"

#include "SQLiteWrapper/Bind.h"
#include "SQLiteWrapper/Get.h"
#include "SQLiteWrapper/QueryBuilder.h"
#include "SQLiteWrapper/Set.h"

#include "Awl/IntRange.h"

#include <string>

using namespace swtest;

namespace
{
    class NonCopyable
    {
    public:

        NonCopyable() : _a(0) {}
        
        explicit NonCopyable(int a) : _a(a)
        {
            ++count;
        }

        ~NonCopyable()
        {
            --count;
        }

        NonCopyable(NonCopyable const&) = delete;

        NonCopyable(NonCopyable&& other) : NonCopyable(other._a)
        {
            other._moved = true;
        }

        NonCopyable& operator = (const NonCopyable&) = delete;

        NonCopyable& operator = (NonCopyable&& other)
        {
            _a = other._a;
            other._moved = true;

            return *this;
        }

        bool operator == (const NonCopyable& other) const
        {
            return _a == other._a;
        }

        bool operator != (const NonCopyable& other) const
        {
            return !operator==(other);
        }

        bool operator < (const NonCopyable& other) const
        {
            return _a < other._a;
        }

        static inline int count = 0;

        int value() const
        {
            return _a;
        }

    private:

        bool _moved = false;
        int _a;
    };

    using Nc = NonCopyable;

    struct Bot
    {
        sqlite::RowId botId;
        std::string name;
        std::vector<uint8_t> state;
        Nc nc;

        Bot() = default;
        
        Bot(sqlite::RowId rid, std::string n, std::vector<uint8_t> s, Nc c) :
            botId(std::move(rid)), name(std::move(n)), state(std::move(s)), nc(std::move(c))
        {}

        Bot(const Bot&) = delete;
        Bot& operator = (const Bot&) = delete;

        Bot(Bot&&) = default;
        Bot& operator = (Bot&&) = default;

        AWL_REFLECT(name, botId, state, nc)
    };

    AWL_MEMBERWISE_EQUATABLE(Bot);

    static_assert(std::input_iterator<sqlite::Iterator<NonCopyable>>);

    static_assert(std::input_iterator<sqlite::Iterator<Bot>>);
}

namespace sqlite
{
    inline void bind(Statement & st, size_t col, const Nc& val)
    {
        sqlite::bind(st, col, val.value());
    }
}

AWL_TEST(NonCopyable)
{
    AWL_UNUSED_CONTEXT;
    
    //Check if helper functions do not require copy constructor and copy assignment.
    {
        Bot bot;
        
        awl::for_each(bot.as_tuple(), [](auto& field) { static_cast<void>(field);});

        sqlite::helpers::forEachFieldValue(bot, [](auto& field, auto fieldIndex)
        {
            static_cast<void>(field);
            static_cast<void>(fieldIndex);
        });
    }

    {
        const std::string table_name = "nc_bots";
        
        DbContainer c(context);

        auto set = makeSet(c._db, table_name, std::make_tuple(&Bot::botId));

        set.insert(Bot{ 0, "BTC_USDT", {}, Nc(0) });
        
        //bot.rowId = c.db().lastRowId();
    }
}
