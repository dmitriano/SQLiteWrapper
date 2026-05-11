#include "DbContainer.h"
#include "Tests/TableHelper.h"

#include "SQLiteWrapper/Bind.h"
#include "SQLiteWrapper/Get.h"
#include "SQLiteWrapper/QueryBuilder.h"
#include "SQLiteWrapper/AutoincrementSet.h"

#include "Awl/IntRange.h"

#include <string>

using namespace swtest;

namespace
{
    struct Bot
    {
        sqlite::RowId botId;
        std::string name;
        std::vector<uint8_t> state;

        AWL_REFLECT(name, botId, state)
    };

    AWL_MEMBERWISE_EQUATABLE(Bot);

    static_assert(std::input_iterator<sqlite::Iterator<Bot>>);

    std::vector<Bot> bots =
    {
        { 0, "BTC_USDT", {} },
        { 0, "XRP_USDT", {1u, 2u, 3u, 4u, 5u, 6u} },
        { 0, "ETH_USDT", {7u, 8u, 9u} }
    };

    Bot bot1{ 0, "DASH_USDT", {1u, 2u, 3u, 4u, 5u, 6u} };
    Bot bot2{ 0, "XRP_USDT", {1u, 2u} };
}

AWL_TEST(RowIdRawQueries)
{
    const std::string table_name = "bots";

    DbContainer c(context);

    Database& db = c.db();

    {
        sqlite::TableBuilder<Bot> builder(table_name);

        builder.addColumns();

        builder.setPrimaryKey(&Bot::botId);

        const std::string query = builder.build();

        db.exec(query);
    }
    
    {
        sqlite::Statement st(db, awl::aformat() << "INSERT INTO " << table_name << " (name, state) VALUES (?2, ?3);");

        sqlite::bind(st, 1, bots[1].name);
        sqlite::bind(st, 2, bots[2].state);

        st.exec();
    }

    {
        sqlite::Statement st(db, awl::aformat() << "SELECT rowId, name, state FROM " << table_name << ";");

        while (st.next())
        {
            Bot bot;

            sqlite::get(st, 0, bot.botId);
            sqlite::get(st, 1, bot.name);
            sqlite::get(st, 2, bot.state);

            context.logger->debug(_T("{}, {}, {}"), bot.botId, bot.name, bot.state.size());
        }
    }
}

AWL_TEST(RowIdRaw)
{
    DbContainer c(context);

    c._db->exec("CREATE TABLE raw_bots (name TEXT NOT NULL COLLATE NOCASE, state BLOB);");

    sqlite::Statement insert_statement(*c._db, "INSERT INTO raw_bots (name, state) VALUES (?1, ?3);");

    insert_statement.bindText(0, "BTCUSDT");
    insert_statement.bindBlob(2, { 0, 1, 3 });

    insert_statement.exec();
}

AWL_TEST(RowIdSet)
{
    const std::string table_name = "bots";
    
    DbContainer c(context);

    auto set = MakeAutoincrementSet(c._db, table_name, &Bot::botId);

    for (Bot& bot : bots)
    {
        set.insert(bot);

        // bot.botId = c.db().lastRowId();
    }

    {
        Bot b1 = *set.begin();

        AWL_ASSERT(b1 == bots[0]);
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

    //Algo tests

    AWL_ASSERT(std::ranges::equal(bots, set));

    {
        //auto r = std::ranges::common_view{ set };
    }

    //Find/Update tests

    for (const Bot& bot : bots)
    {
        Bot actual;

        AWL_ASSERT(set.find(bot.botId, actual));

        AWL_ASSERT(actual == bot);
    }

    bot1.botId = bots[0].botId;
    
    {
        set.update(bot1);

        Bot actual;

        AWL_ASSERT(set.find(bot1.botId, actual));

        AWL_ASSERT(actual == bot1);
    }

    bot2.botId = bots[1].botId;

    {
        sqlite::Updater up = set.createUpdater(std::make_tuple(&Bot::state));

        {
            up.update(bot2);

            Bot actual;

            AWL_ASSERT(set.find(bot2.botId, actual));

            AWL_ASSERT(actual == bot2);
        }

        {
            const std::vector<uint8_t> state = { 33u, 35u };

            up.update(std::make_tuple(bot2.botId), std::make_tuple(state));

            bot2.state = state;

            Bot actual;

            AWL_ASSERT(set.find(bot2.botId, actual));

            AWL_ASSERT(actual == bot2);
        }
    }
}

AWL_TEST(RowIdSequence)
{
    const std::string table_name = "bots";

    DbContainer c(context);

    auto set = MakeAutoincrementSet(c._db, table_name, &Bot::botId);

    for (size_t i = 0; i < bots.size(); ++i)
    {
        set.insert(bots[i]);

        AWL_ASSERT_EQUAL(static_cast<sqlite::RowId>(i) * 2 + 1, bots[i].botId);

        set.deleteElement(bots[i]);

        set.insert(bots[i]);

        AWL_ASSERT_EQUAL(static_cast<sqlite::RowId>(i) * 2 + 2, bots[i].botId);
    }
}

AWL_TEST(RowIdSetClear)
{
    const std::string table_name = "bots";

    DbContainer c(context);

    auto set = MakeAutoincrementSet(c._db, table_name, &Bot::botId);

    for (Bot& bot : bots)
    {
        set.insert(bot);
    }

    AWL_ASSERT_EQUAL(static_cast<std::ranges::range_difference_t<decltype(set)>>(bots.size()), std::ranges::distance(set));

    set.clear();

    AWL_ASSERT_EQUAL(0u, std::ranges::distance(set));

    for (const Bot& bot : bots)
    {
        Bot actual;

        AWL_ASSERT(!set.find(bot.botId, actual));
    }
}
