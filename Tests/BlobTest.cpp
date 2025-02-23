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
    struct Bot
    {
        int id;
        std::string name;
        std::vector<uint8_t> state;

        AWL_REFLECT(id, name, state)
    };

    AWL_MEMBERWISE_EQUATABLE(Bot);

    static_assert(std::input_iterator<sqlite::Iterator<Bot>>);

    const std::vector<Bot> bots =
    {
        { 1, "BTC_USDT", {} },
        { 2, "XRP_USDT", {1u, 2u, 3u, 4u, 5u, 6u} },
        { 3, "ETH_USDT", {7u, 8u, 9u} }
    };

    const Bot bot1{ 1, "DASH_USDT", {1u, 2u, 3u, 4u, 5u, 6u} };
    const Bot bot2{ 2, "XRP_USDT", {1u, 2u} };
}

AWL_TEST(Blob)
{
    const std::string table_name = "bots";
    
    DbContainer c(context);

    sqlite::SetStorage set(c.m_db, table_name, std::make_tuple(&Bot::id));
    set.Create();
    set.Prepare();

    for (const Bot& bot : bots)
    {
        set.Insert(bot);
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

        AWL_ASSERT(set.Find(bot.id, actual));

        AWL_ASSERT(actual == bot);
    }

    {
        set.Update(bot1);

        Bot actual;

        AWL_ASSERT(set.Find(bot1.id, actual));

        AWL_ASSERT(actual == bot1);
    }

    {
        sqlite::Updater up = set.CreateUpdater(std::make_tuple(&Bot::state));

        up.Update(bot2);

        Bot actual;

        AWL_ASSERT(set.Find(bot2.id, actual));

        AWL_ASSERT(actual == bot2);
    }
}
