#include "DbContainer.h"
#include "SQLiteWrapper/Bind.h"
#include "SQLiteWrapper/Get.h"
#include "SQLiteWrapper/QueryBuilder.h"
#include "SQLiteWrapper/Scalar.h"

#include "Awl/IntRange.h"

#include <string>

using namespace swtest;

namespace
{
    using Clock = std::chrono::system_clock;

    using TimePoint = std::chrono::time_point<Clock>;

    struct Log
    {
        TimePoint dt;
        std::string category;
        std::string message;

        AWL_REFLECT(dt, category, message)
    };

    AWL_MEMBERWISE_EQUATABLE(Log);

    Log logSample{ Clock::now(), "info", "abc" };

    const std::string tableName = "log_messages";

    void CreateTable(Database & db)
    {
        sqlite::TableBuilder<Log> builder(tableName, true);

        builder.SetColumnConstraint(&Log::dt, "INTEGER NOT NULL PRIMARY KEY");

        db.Exec(builder.Create());
    }

    Statement MakeInsertStatement(Database & db)
    {
        return Statement(db, BuildParameterizedInsertQuery<Log>(tableName));
    }

    void firstchar(sqlite3_context* context, int argc, sqlite3_value** argv)
    {
        if (argc == 1)
        {
            const char * text = reinterpret_cast<const char *>(sqlite3_value_text(argv[0]));

            if (text && text[0])
            {
                char result[2];
                result[0] = text[0];
                result[1] = '\0';
                
                sqlite3_result_text(context, result, -1, SQLITE_TRANSIENT);
            }
            else
            {
                sqlite3_result_null(context);
            }
        }
        else
        {
            //We never reach this point, because it does not compile with a wrong number of the arguments.
            sqlite3_result_error(context, "wrong number of arguments passed to firstchar function", SQLITE_ERROR);
        }
    }

    std::string filterCategory;
    
    void filter(sqlite3_context* context, int argc, sqlite3_value** argv)
    {
        if (argc == 1)
        {
            const char* text = reinterpret_cast<const char*>(sqlite3_value_text(argv[0]));

            const int result = filterCategory == text ? 1 : 0;
            
            sqlite3_result_int(context, result);
        }
        else
        {
            //We never reach this point, because it does not compile with a wrong number of the arguments.
            sqlite3_result_error(context, "wrong number of arguments passed to filter function", SQLITE_ERROR);
        }
    }

    void CheckCount(Database& db, int expected = 1)
    {
        sqlite::Statement s(db, awl::aformat() << "SELECT COUNT(*) FROM " << tableName << ";");

        int count;
        sqlite::SelectScalar(s, count);
        AWL_ASSERT(count == expected);
    }

    void CheckSample(sqlite::Statement & s)
    {
        AWL_ASSERT(s.Next());

        Log log;
        sqlite::Get(s, 0, log);

        AWL_ASSERT(log == logSample);

        AWL_ASSERT(!s.Next());
    }
    
    void CheckSample(Database& db)
    {
        QueryBuilder<Log> builder;

        builder.StartSelect(tableName);

        builder.AddTerminator();

        const std::string query = builder.str();

        sqlite::Statement s(db, query);

        CheckSample(s);
    }

    void InsertSamples(Database& db, const std::vector<Log> & samples)
    {
        {
            Statement s = MakeInsertStatement(db);

            for (auto& log : samples)
            {
                sqlite::Bind(s, 0, log);

                s.Exec();
            }
        }

        CheckCount(db, static_cast<int>(samples.size()));
    }

    void InsertSample(Database& db)
    {
        {
            Statement s = MakeInsertStatement(db);

            //It can't be a temporary.
            sqlite::Bind(s, 0, logSample);

            s.Select();
        }

        CheckCount(db);

        CheckSample(db);
    }
}

AWL_TEST(TrivialFunction)
{
    DbContainer c(context);
    Database & db = c.db();

    db.CreateFunction("firstchar", 1, &firstchar);

    {
        sqlite::Statement s(db, "SELECT firstchar('abc');");

        AWL_ASSERT(s.Next());
        AWL_ASSERT(std::strcmp(s.GetText(0), "a") == 0);
        AWL_ASSERT(!s.Next());
    }

    {
        sqlite::Statement s(db, "SELECT firstchar('');");

        AWL_ASSERT(s.Next());
        AWL_ASSERT(s.IsNull(0));
        AWL_ASSERT(!s.Next());
    }

    try
    {
        //It knows the number of the arguments.
        sqlite::Statement s(db, "SELECT firstchar('abc', 'xyz');");
        AWL_FAIL;
    }
    catch (const sqlite::SQLiteException&)
    {
    }
}

AWL_TEST(TableFunction)
{
    DbContainer c(context);
    Database& db = c.db();

    CreateTable(db);

    InsertSample(db);

    db.CreateFunction("firstchar", 1, &firstchar);

    {
        sqlite::Statement s(db, awl::aformat() << "SELECT firstchar(""message"") FROM " << tableName << ";");

        AWL_ASSERT(s.Next());
        AWL_ASSERT(std::strcmp(s.GetText(0), "a") == 0);
        AWL_ASSERT(!s.Next());
    }

    {
        QueryBuilder<Log> builder;

        builder.StartSelect(tableName);

        builder << " WHERE firstchar(""message"") = 'a'";

        builder.AddTerminator();

        const std::string query = builder.str();

        sqlite::Statement s(db, query);

        CheckSample(s);
    }
}

AWL_TEST(ViewFunction)
{
    DbContainer c(context);
    Database& db = c.db();

    CreateTable(db);

    db.CreateFunction("filter", 1, &filter);

    auto now = Clock::now();

    Clock::duration d = std::chrono::duration_cast<Clock::duration>(std::chrono::seconds(1));
    
    const std::vector<Log> samples =
    {
        Log{now, "info", "first"},
        Log{now + d, "debug", "second"},
        Log{now + d * 2, "debug", "third"}
    };

    InsertSamples(db, samples);

    const std::string view_name = "log_view";

    {
        QueryBuilder<Log> builder;

        builder.CreateView(view_name);
        
        builder.StartSelect(tableName);

        builder << " WHERE filter(""category"")";

        builder.AddTerminator();

        const std::string query = builder.str();

        db.Exec(query);
    }

    sqlite::Statement count_statement(db, awl::aformat() << "SELECT COUNT(*) FROM " << view_name << ";");

    auto check_category = [&](std::string category, int expected)
    {
        filterCategory = category;

        int count;
        sqlite::SelectScalar(count_statement, count);
        AWL_ASSERT(count == expected);
        count_statement.Reset();
    };

    check_category("debug", 2);
    check_category("info", 1);
    check_category("other", 0);
}
