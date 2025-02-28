#include "DbContainer.h"
#include "SQLiteWrapper/Bind.h"
#include "SQLiteWrapper/Get.h"

#include <vector>
#include <optional>

#include "Awl/IntRange.h"
#include "Awl/StdConsole.h"

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

        builder.SetColumnConstraint(&Price::dt, "INTEGER NOT NULL PRIMARY KEY");

        db.Exec(builder.Create());
    }

    template <class Price = PricePair>
    Statement MakeInsertStatement(Database & db, std::optional<size_t> i = {})
    {
        return Statement(db, BuildParameterizedInsertQuery<Price>(MakeTableName(i)));
    }

    size_t GetCount(Database & db, std::optional<size_t> i = {})
    {
        Statement s(db, (awl::aformat() << "SELECT count(*) FROM " << MakeTableName(i) << ";"));
        int count;
        sqlite::SelectScalar(s, count);
        return static_cast<size_t>(count);
    }

    inline void CheckCount(Database & db, size_t expected_count, std::optional<size_t> i = {})
    {
        AWL_ASSERT_EQUAL(expected_count, GetCount(db, i));
    }

    void PrintStat(const awl::testing::TestContext & context, const awl::StopWatch & sw, size_t batch_index, size_t batch_size)
    {
        const float seconds = sw.GetElapsedSeconds<float>();

        context.out << batch_size << _T(" / ") << (batch_index + 1) * batch_size << _T(" rows have been inserted within ") <<
            std::fixed << std::setprecision(2) << seconds <<
            _T(" seconds, speed: ") <<
            std::fixed << std::setprecision(2) << batch_size / seconds <<
            _T(" rows per second.") << std::endl;
    }
}

//--output all --filter InsertPrice_Test --batch_count 1000000 --batch_size 1000000 --transaction --synchronous FULL --journal_mode TRUNCATE
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

                        Bind(s, 0, MakePricePair(i));

                        s.Select();
                        
                        //if (!s.TryExec())
                        //{
                        //    context.out << _T("Insertion error: ") << awl::FromACString(db.GetLastError()) << std::endl;
                        //}

                        s.Reset();
                    }
                };

                if (transaction)
                {
                    db.Try(func);
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

//--output all --filter InsertMarketPrice_Test --batch_count 1000000 --batch_size 1000000 --transaction --use_index --synchronous FULL --journal_mode TRUNCATE
//--output all --filter InsertMarketPrice_Test --batch_count 1000 --batch_size 1000000 --transaction
//--output all --filter InsertMarketPrice_Test --batch_count 1000 --batch_size 1000 --transaction --use_index
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

        const bool index_exists = db.IndexExists("i_exchange");

        context.out << _T("The number of rows: ") << GetCount(db) << std::endl;

        if (use_index)
        {
            if (index_exists)
            {
                context.out << _T("The indices already exist.") << std::endl;
            }
            else
            {
                awl::StopWatch sw;

                db.Exec("CREATE INDEX i_exchange ON prices(exchangeId)");
                db.Exec("CREATE INDEX i_market ON prices(marketId)");

                context.out << _T("The indices have been create within ") <<
                    std::fixed << std::setprecision(2) << sw.GetElapsedSeconds<float>()
                    << _T(" seconds.") << std::endl;
            }
        }
        else
        {
            if (index_exists)
            {
                db.Exec("DROP INDEX i_exchange");
                db.Exec("DROP INDEX i_market");

                context.out << _T("The indices have been dropped.") << std::endl;
            }
            else
            {
                context.out << _T("The indices do not exist.") << std::endl;
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

                        Bind(s, 0, MakeMarketPricePair(i));

                        s.Select();

                        //if (!s.TryExec())
                        //{
                        //    context.out << _T("Insertion error: ") << awl::FromACString(db.GetLastError()) << std::endl;
                        //}

                        s.Reset();
                    }
                };

                if (transaction)
                {
                    db.Try(func);
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

    db.Exec(awl::aformat() << "PRAGMA synchronous = " << awl::ToAString(synchronous) << ";");
    db.Exec(awl::aformat() << "PRAGMA journal_mode = " << awl::ToAString(journal_mode) << ";");

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

                Bind(s, 0, MakePricePair(i));
                s.Select();
                s.Reset();
            }

            PrintStat(context, sw, batch_index, batch_size);
        }
    }

    CheckCount(db, batch_size * batch_count);
}

//--output all --filter MarsMt_Test --batch_count 1000000 --batch_size 1000 --transaction --synchronous OFF  --journal_mode TRUNCATE
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
                    Bind(s, 0, MakePricePair(i));
                    s.Select();
                    s.Reset();
                }
                catch (const SQLiteException & e)
                {
                    context.out << e.What() << std::endl;
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

        builder.SetColumnConstraint(&MarketInfo::id, "INTEGER NOT NULL PRIMARY KEY");

        const std::string query = builder.Create();

        context.out << awl::FromAString(query) << std::endl;;

        db.Exec(query);
    }

    Precision precision_sample{1, 2, 3, 4 };
    Precision precision_result{ 5, 6, 7, 8 };

    MarketInfo mi_sample{ "abc", precision_sample };
    MarketInfo mi_result{ mi_sample.id, precision_result };

    const IndexFilter key_filter{ 0 };
    const IndexFilter value_filter{ 1, 2, 3, 4 };

    {
        const std::string query = sqlite::BuildParameterizedInsertQuery<MarketInfo>(table_name);
        
        context.out << awl::FromAString(query) << std::endl;;

        sqlite::Statement insert_statement = sqlite::Statement(db, query);

        sqlite::Bind(insert_statement, 0, mi_sample);

        insert_statement.Select();
        //insert_statement.Reset();
    }

    {
        const std::string query = sqlite::BuildTrivialSelectQuery<MarketInfo>(table_name);

        context.out << awl::FromAString(query) << std::endl;

        sqlite::Statement select_statement = sqlite::Statement(db, query);

        AWL_ASSERT(select_statement.Next());

        MarketInfo mi_actual;

        sqlite::Get(select_statement, 0, mi_actual);

        AWL_ASSERT(mi_actual == mi_sample);
    }

    {
        const std::string query = BuildParameterizedUpdateQuery<MarketInfo>(table_name, value_filter, key_filter);

        context.out << awl::FromAString(query) << std::endl;

        sqlite::Statement update_statement = sqlite::Statement(db, query);

        context.out << awl::FromAString(query) << std::endl;

        if (whole_record)
        {
            sqlite::Bind(update_statement, 0, mi_result);
        }
        else
        {
            sqlite::Bind(update_statement, 1, precision_result);

            sqlite::Bind(update_statement, 0, mi_sample.id);
        }

        update_statement.Select();
    }

    {
        const std::string query = sqlite::BuildParameterizedSelectQuery<MarketInfo>(table_name, value_filter, key_filter);

        context.out << awl::FromAString(query) << std::endl;

        sqlite::Statement select_statement = sqlite::Statement(db, query);

        sqlite::Bind(select_statement, 0, mi_sample.id);

        AWL_ASSERT(select_statement.Next());

        Precision actual;

        sqlite::Get(select_statement, 0, actual);

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
            std::string aline = awl::ToAString(line);

            awl::StopWatch sw;

            Statement s(db, aline);
            s.Next();

            context.out << _T("The query has taken ") <<
                std::fixed << std::setprecision(6) << sw.GetElapsedSeconds<float>()
                << _T(" seconds.") << std::endl;
        }
        catch (const SQLiteException & e)
        {
            context.out << e.What() << _T(" [") << _T("]") << std::endl;
        }
    }
}
