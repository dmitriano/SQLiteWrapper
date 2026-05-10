#include "DbContainer.h"

#include "SQLiteWrapper/Bind.h"
#include "SQLiteWrapper/Get.h"
#include "SQLiteWrapper/Scalar.h"

#include "Awl/IntRange.h"
#include "Awl/StdConsole.h"

#include <vector>
#include <optional>

using namespace swtest;

namespace
{
    using Clock = std::chrono::system_clock;

    using TimePoint = std::chrono::time_point<Clock>;

    struct PricePair
    {
        TimePoint dt;
        double buy;
        double sell;

        AWL_REFLECT(dt, buy, sell)
    };

    struct MarketPricePair
    {
        TimePoint dt;
        int64_t exchangeId;
        int64_t marketId;
        double buy;
        double sell;

        AWL_REFLECT(dt, exchangeId, marketId, buy, sell)
    };

    PricePair MakePricePair(size_t i)
    {
        return PricePair{ Clock::now(), static_cast<double>(i), static_cast<double>(i) * 0.99 };
    }

    MarketPricePair MakeMarketPricePair(size_t i)
    {
        return MarketPricePair{ Clock::now(), static_cast<int64_t>(i % 100), static_cast<int64_t>(i % 1000),
            static_cast<double>(i), static_cast<double>(i) * 0.99};
    }

    const std::string tableName = "prices";

    std::string MakeTableName(std::optional<size_t> i)
    {
        std::ostringstream out;

        out << tableName;

        if (i)
        {
            out << *i;
        }

        return out.str();
    }

    template <class Price = PricePair>
    void CreateTable(Database & db, std::optional<size_t> i = {})
    {
        sqlite::TableBuilder<Price> builder(MakeTableName(i), true);

        builder.setColumnConstraint(&Price::dt, "INTEGER NOT NULL PRIMARY KEY");

        db.exec(builder.create());
    }

    template <class Price = PricePair>
    Statement MakeInsertStatement(Database & db, std::optional<size_t> i = {})
    {
        return Statement(db, buildParameterizedInsertQuery<Price>(MakeTableName(i)));
    }

    size_t GetCount(Database & db, std::optional<size_t> i = {})
    {
        Statement s(db, (awl::aformat() << "SELECT count(*) FROM " << MakeTableName(i) << ";"));
        int count;
        sqlite::selectScalar(s, count);
        return static_cast<size_t>(count);
    }

    inline void CheckCount(Database & db, size_t expected_count, std::optional<size_t> i = {})
    {
        AWL_ASSERT_EQUAL(expected_count, GetCount(db, i));
    }

    void PrintStat(const awl::testing::TestContext & context, const awl::StopWatch & sw, size_t batch_index, size_t batch_size)
    {
        const float seconds = sw.elapsedSeconds<float>();

        context.logger->debug(
            _T("{} / {} rows have been inserted within {:.2f} seconds, speed: {:.2f} rows per second."),
            batch_size,
            (batch_index + 1) * batch_size,
            seconds,
            batch_size / seconds);
    }
}

//--output=all --filter=InsertPrice_Test --batch_count=1000000 --batch_size=1000000 --transaction --synchronous=FULL --journal_mode=TRUNCATE
AWL_TEST(InsertPrice)
{
    AWL_ATTRIBUTE(size_t, batch_count, 10);
    AWL_ATTRIBUTE(size_t, batch_size, 10);
    AWL_FLAG(transaction);

    DbContainer c(context);
    Database & db = c.db();

    {
        CreateTable(db);

        Statement s = MakeInsertStatement(db);

        for (size_t batch_index = 0; batch_index < batch_count; ++batch_index)
        {
            awl::StopWatch sw;

            {
                auto func = [&s, batch_size, batch_index]()
                {
                    for (size_t local_index = 0; local_index < batch_size; ++local_index)
                    {
                        const size_t i = batch_index * batch_size + local_index;

                        sqlite::bind(s, 0, MakePricePair(i));

                        s.select();
                        
                        //if (!s.tryExec())
                        //{
                        //    context.logger->debug(_T("Insertion error: {}"), awl::FromACString(db.GetLastError()));
                        //}

                        s.reset();
                    }
                };

                if (transaction)
                {
                    db.tryRun(func);
                }
                else
                {
                    func();
                }
            }

            PrintStat(context, sw, batch_index, batch_size);
        }
    }

    CheckCount(db, batch_size * batch_count);
}

//--output=all --filter=InsertMarketPrice_Test --batch_count=1000000 --batch_size=1000000 --transaction --use_index --synchronous=FULL --journal_mode=TRUNCATE
//--output=all --filter=InsertMarketPrice_Test --batch_count=1000 --batch_size=1000000 --transaction
//--output=all --filter=InsertMarketPrice_Test --batch_count=1000 --batch_size=1000 --transaction --use_index
AWL_TEST(InsertMarketPrice)
{
    AWL_ATTRIBUTE(size_t, batch_count, 10);
    AWL_ATTRIBUTE(size_t, batch_size, 10);
    AWL_FLAG(transaction);
    AWL_FLAG(use_index);

    DbContainer c(context);
    Database & db = c.db();

    {
        CreateTable<MarketPricePair>(db);

        const bool index_exists = db.indexExists("i_exchange");

        context.logger->debug(_T("The number of rows: {}"), GetCount(db));

        if (use_index)
        {
            if (index_exists)
            {
                context.logger->debug(_T("The indices already exist."));
            }
            else
            {
                awl::StopWatch sw;

                db.exec("CREATE INDEX i_exchange ON prices(exchangeId)");
                db.exec("CREATE INDEX i_market ON prices(marketId)");

                context.logger->debug(_T("The indices have been create within {:.2f} seconds."), sw.elapsedSeconds<float>());
            }
        }
        else
        {
            if (index_exists)
            {
                db.exec("DROP INDEX i_exchange");
                db.exec("DROP INDEX i_market");

                context.logger->debug(_T("The indices have been dropped."));
            }
            else
            {
                context.logger->debug(_T("The indices do not exist."));
            }
        }

        Statement s = MakeInsertStatement<MarketPricePair>(db);

        for (size_t batch_index = 0; batch_index < batch_count; ++batch_index)
        {
            awl::StopWatch sw;

            {
                auto func = [&s, batch_size, batch_index]()
                {
                    for (size_t local_index = 0; local_index < batch_size; ++local_index)
                    {
                        const size_t i = batch_index * batch_size + local_index;

                        sqlite::bind(s, 0, MakeMarketPricePair(i));

                        s.select();

                        //if (!s.tryExec())
                        //{
                        //    context.logger->debug(_T("Insertion error: {}"), awl::FromACString(db.GetLastError()));
                        //}

                        s.reset();
                    }
                };

                if (transaction)
                {
                    db.tryRun(func);
                }
                else
                {
                    func();
                }
            }

            PrintStat(context, sw, batch_index, batch_size);
        }
    }

    CheckCount(db, batch_size * batch_count);
}

AWL_TEST(Mars)
{
    AWL_ATTRIBUTE(size_t, batch_count, 10);
    AWL_ATTRIBUTE(size_t, batch_size, 10);
    AWL_ATTRIBUTE(awl::String, synchronous, _T("FULL"));
    AWL_ATTRIBUTE(awl::String, journal_mode, _T("DELETE"));

    DbContainer c(context);
    Database & db = c.db();

    db.exec(awl::aformat() << "PRAGMA synchronous = " << awl::toAString(synchronous) << ";");
    db.exec(awl::aformat() << "PRAGMA journal_mode = " << awl::toAString(journal_mode) << ";");

    {
        CreateTable(db);

        std::vector<Statement> v;
        v.reserve(batch_size);

        for (size_t local_index = 0; local_index < batch_size; ++local_index)
        {
            static_cast<void>(local_index);

            v.push_back(MakeInsertStatement(db));
        }

        for (size_t batch_index = 0; batch_index < batch_count; ++batch_index)
        {
            awl::StopWatch sw;

            for (size_t local_index = 0; local_index < batch_size; ++local_index)
            {
                Statement & s = v[local_index];

                const size_t i = batch_index * batch_size + local_index;

                sqlite::bind(s, 0, MakePricePair(i));
                s.select();
                s.reset();
            }

            PrintStat(context, sw, batch_index, batch_size);
        }
    }

    CheckCount(db, batch_size * batch_count);
}

//--output=all --filter=MarsMt_Test --batch_count=1000000 --batch_size=1000 --transaction --synchronous=OFF  --journal_mode=TRUNCATE
AWL_TEST(MarsMt)
{
    AWL_ATTRIBUTE(size_t, batch_count, 10);
    AWL_ATTRIBUTE(size_t, batch_size, 10);

    DbContainer c(context);
    Database & db = c.db();

    {
        //Create the tables first to prevent "database schema has changed" error.
        for (size_t i : awl::make_count(batch_size))
        {
            CreateTable(db, i);
        }

        std::vector<Statement> v;
        v.reserve(batch_size);

        for (size_t i : awl::make_count(batch_size))
        {
            v.push_back(MakeInsertStatement(db, i));
        }

        for (size_t batch_index : awl::make_count(batch_count))
        {
            awl::StopWatch sw;

            for (size_t local_index = 0; local_index < batch_size; ++local_index)
            {
                Statement & s = v[local_index];

                const size_t i = batch_index * batch_size + local_index;

                try
                {
                    sqlite::bind(s, 0, MakePricePair(i));
                    s.select();
                    s.reset();
                }
                catch (const SQLiteException & e)
                {
                    context.logger->debug(e.message());
                    AWL_FAIL;
                }
            }

            PrintStat(context, sw, batch_index, batch_size);
        }
    }

    for (size_t local_index = 0; local_index < batch_size; ++local_index)
    {
        CheckCount(db, batch_count, local_index);
    }
}

namespace
{
    struct Precision
    {
        uint8_t base;
        uint8_t quote;
        uint8_t amount;
        uint8_t price;

        AWL_REFLECT(base, quote, amount, price)
    };

    AWL_MEMBERWISE_EQUATABLE(Precision);

    struct MarketInfo
    {
        std::string id;

        Precision precision;

        AWL_REFLECT(id, precision)
    };

    AWL_MEMBERWISE_EQUATABLE(MarketInfo);
}

AWL_TEST(MarketInfo)
{
    AWL_FLAG(whole_record);

    DbContainer c(context);
    Database& db = c.db();

    const char table_name[] = "market_info";

    {
        sqlite::TableBuilder<MarketInfo> builder(table_name, true);

        builder.setColumnConstraint(&MarketInfo::id, "INTEGER NOT NULL PRIMARY KEY");

        const std::string query = builder.create();

        context.logger->debug(_T("{}"), query);

        db.exec(query);
    }

    Precision precision_sample{1, 2, 3, 4 };
    Precision precision_result{ 5, 6, 7, 8 };

    MarketInfo mi_sample{ "abc", precision_sample };
    MarketInfo mi_result{ mi_sample.id, precision_result };

    const IndexFilter key_filter{ 0 };
    const IndexFilter value_filter{ 1, 2, 3, 4 };

    {
        const std::string query = sqlite::buildParameterizedInsertQuery<MarketInfo>(table_name);
        
        context.logger->debug(_T("{}"), query);

        sqlite::Statement insert_statement = sqlite::Statement(db, query);

        sqlite::bind(insert_statement, 0, mi_sample);

        insert_statement.select();
        //insert_statement.reset();
    }

    {
        const std::string query = sqlite::buildTrivialSelectQuery<MarketInfo>(table_name);

        context.logger->debug(_T("{}"), query);

        sqlite::Statement select_statement = sqlite::Statement(db, query);

        AWL_ASSERT(select_statement.next());

        MarketInfo mi_actual;

        sqlite::get(select_statement, 0, mi_actual);

        AWL_ASSERT(mi_actual == mi_sample);
    }

    {
        const std::string query = buildParameterizedUpdateQuery<MarketInfo>(table_name, value_filter, key_filter);

        context.logger->debug(_T("{}"), query);

        sqlite::Statement update_statement = sqlite::Statement(db, query);

        context.logger->debug(_T("{}"), query);

        if (whole_record)
        {
            sqlite::bind(update_statement, 0, mi_result);
        }
        else
        {
            sqlite::bind(update_statement, 1, precision_result);

            sqlite::bind(update_statement, 0, mi_sample.id);
        }

        update_statement.select();
    }

    {
        const std::string query = sqlite::buildParameterizedSelectQuery<MarketInfo>(table_name, value_filter, key_filter);

        context.logger->debug(_T("{}"), query);

        sqlite::Statement select_statement = sqlite::Statement(db, query);

        sqlite::bind(select_statement, 0, mi_sample.id);

        AWL_ASSERT(select_statement.next());

        Precision actual;

        sqlite::get(select_statement, 0, actual);

        AWL_ASSERT(actual == precision_result);
    }
}
        
AWL_EXAMPLE(Console)
{
    DbContainer c(context);
    Database & db = c.db();

    while (true)
    {
        awl::String line;

        std::getline(awl::cin(), line);

        if (line == _T("exit"))
        {
            break;
        }

        try        {
            std::string aline = awl::toAString(line);

            awl::StopWatch sw;

            Statement s(db, aline);
            s.next();

        context.logger->debug(_T("The query has taken {:.6f} seconds."), sw.elapsedSeconds<float>());
        }
        catch (const SQLiteException & e)
        {
            context.logger->debug(_T("{} []"), e.message());
        }
    }
}
