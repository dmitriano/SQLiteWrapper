#include "Tests/DbContainer.h"
#include "Tests/TableHelper.h"

#include "SQLiteWrapper/Bind.h"
#include "SQLiteWrapper/Get.h"
#include "SQLiteWrapper/QueryBuilder.h"
#include "SQLiteWrapper/Set.h"

#include "Awl/IntRange.h"

#include <cstddef>
#include <string>

using namespace swtest;

namespace
{
    struct Bot
    {
        int id;
        std::string name;
        std::vector<std::byte> state;

        AWL_REFLECT(id, name, state)
    };

    AWL_MEMBERWISE_EQUATABLE(Bot);

    static_assert(std::input_iterator<sqlite::Iterator<Bot>>);

    const std::vector<Bot> bots =
    {
        { 1, "BTC_USDT", {} },
        { 2, "XRP_USDT", { std::byte{ 1 }, std::byte{ 2 }, std::byte{ 3 }, std::byte{ 4 }, std::byte{ 5 }, std::byte{ 6 } } },
        { 3, "ETH_USDT", { std::byte{ 7 }, std::byte{ 8 }, std::byte{ 9 } } }
    };

    const Bot bot1{ 1, "DASH_USDT", { std::byte{ 1 }, std::byte{ 2 }, std::byte{ 3 }, std::byte{ 4 }, std::byte{ 5 }, std::byte{ 6 } } };
    const Bot bot2{ 2, "XRP_USDT", { std::byte{ 1 }, std::byte{ 2 } } };
}

AWL_TEST(Blob)
{
    const std::string table_name = "bots";
    
    DbContainer c(context);

    auto set = makeSet(c._db, table_name, std::make_tuple(&Bot::id));

    for (const Bot& bot : bots)
    {
        set.insert(bot);
    }

    {
        Bot b0 = *set.begin();

        AWL_ASSERT(b0 == bots[0]);
    }

    //Range-based loop test
    {
        std::vector<Bot> actual_bots;

        for (const Bot& bot : set)
        {
            actual_bots.push_back(bot);
        }

        AWL_ASSERT(std::ranges::equal(bots, actual_bots));
    }

    //std::ranges tests

    AWL_ASSERT(std::ranges::equal(bots, set));

    //Find/Update tests

    for (const Bot& bot : bots)
    {
        Bot actual;

        AWL_ASSERT(set.find(bot.id, actual));

        AWL_ASSERT(actual == bot);
    }

    {
        set.update(bot1);

        Bot actual;

        AWL_ASSERT(set.find(bot1.id, actual));

        AWL_ASSERT(actual == bot1);
    }

    {
        sqlite::Updater up = set.createUpdater(std::make_tuple(&Bot::state));

        up.update(bot2);

        Bot actual;

        AWL_ASSERT(set.find(bot2.id, actual));

        AWL_ASSERT(actual == bot2);
    }
}
