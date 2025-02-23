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
        sqlite::RowId rowId;
        std::string name;
        std::vector<uint8_t> state;

        AWL_REFLECT(name, rowId, state)
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

AWL_UNSTABLE_TEST(RowIdRawQueries)
{
    const std::string table_name = "bots";

    DbContainer c(context);

    Database& db = c.db();

    {
        sqlite::TableBuilder<Bot> builder(table_name);

        builder.AddColumns();

        const std::string query = builder.Build();

        db.Exec(query);
    }
    
    {
        sqlite::Statement st(db, awl::aformat() << "INSERT INTO " << table_name << " (name, state) VALUES (?2, ?3);");

        sqlite::Bind(st, 1, bots[1].name);
        sqlite::Bind(st, 2, bots[2].state);

        st.Exec();
    }

    {
        sqlite::Statement st(db, awl::aformat() << "SELECT rowId, name, state FROM " << table_name << ";");

        while (st.Next())
        {
            Bot bot;

            sqlite::Get(st, 0, bot.rowId);
            sqlite::Get(st, 1, bot.name);
            sqlite::Get(st, 2, bot.state);

            context.out << bot.rowId << ", " << awl::FromAString(bot.name) << ", " << bot.state.size() << std::endl;
        }
    }
}

AWL_UNSTABLE_TEST(RowIdRaw)
{
    DbContainer c(context);

    c.m_db->Exec("CREATE TABLE raw_bots (name TEXT NOT NULL COLLATE NOCASE, state BLOB);");

    sqlite::Statement insert_statement(*c.m_db, "INSERT INTO raw_bots (name, state) VALUES (?1, ?3);");

    insert_statement.BindText(0, "BTCUSDT");
    insert_statement.BindBlob(2, { 0, 1, 3 });

    insert_statement.Exec();
}

AWL_UNSTABLE_TEST(RowIdSet)
{
    const std::string table_name = "bots";
    
    DbContainer c(context);

    sqlite::SetStorage set(c.m_db, table_name, std::make_tuple(&Bot::rowId));
    set.Create();
    set.Prepare();

    for (Bot& bot : bots)
    {
        set.Insert(bot);

        bot.rowId = c.db().GetLastRowId();
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

        AWL_ASSERT(set.Find(bot.rowId, actual));

        AWL_ASSERT(actual == bot);
    }

    bot1.rowId = bots[0].rowId;
    
    {
        set.Update(bot1);

        Bot actual;

        AWL_ASSERT(set.Find(bot1.rowId, actual));

        AWL_ASSERT(actual == bot1);
    }

    bot2.rowId = bots[1].rowId;

    {
        sqlite::Updater up = set.CreateUpdater(std::make_tuple(&Bot::state));

        {
            up.Update(bot2);

            Bot actual;

            AWL_ASSERT(set.Find(bot2.rowId, actual));

            AWL_ASSERT(actual == bot2);
        }

        {
            const std::vector<uint8_t> state = { 33u, 35u };

            up.Update(std::make_tuple(bot2.rowId), std::make_tuple(state));

            bot2.state = state;

            Bot actual;

            AWL_ASSERT(set.Find(bot2.rowId, actual));

            AWL_ASSERT(actual == bot2);
        }
    }
}
