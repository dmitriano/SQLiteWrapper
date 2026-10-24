#include "SQLiteWrapper/SequenceTable.h"
#include "SQLiteWrapper/Database.h"
#include "SQLiteWrapper/Exception.h"
#include "SQLiteWrapper/TransactionGuard.h"

#include "Awl/Testing/UnitTest.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <exception>
#include <filesystem>
#include <format>
#include <functional>
#include <limits>
#include <memory>
#include <string>
#include <system_error>
#include <thread>
#include <vector>

namespace
{
    std::shared_ptr<sqlite::Database> createDatabase(const std::shared_ptr<awl::ILogger>& logger,
        const std::string& filename = ":memory:")
    {
        std::shared_ptr<sqlite::Database> db = std::make_shared<sqlite::Database>(filename.c_str(), logger);
        db->exec("PRAGMA busy_timeout=5000;");
        sqlite::SequenceTable::createScheme().create(*db);
        return db;
    }

    class TemporaryDatabase
    {
    public:

        TemporaryDatabase()
        {
            static std::atomic<unsigned> counter{0};
            _path = std::filesystem::temp_directory_path() / std::format("sqlite-sequences-{}-{}.db",
                std::chrono::system_clock::now().time_since_epoch().count(), counter.fetch_add(1));
        }

        ~TemporaryDatabase()
        {
            std::error_code ignored;
            std::filesystem::remove(_path, ignored);
        }

        std::string filename() const { return _path.string(); }

    private:

        std::filesystem::path _path;
    };
}

AWL_TEST(SequenceTableNamedCounters)
{
    const std::shared_ptr<sqlite::Database> db = createDatabase(context.logger);
    sqlite::SequenceTable sequences(db);
    sequences.createSequence("orders");
    sequences.createSequence("bots", 100);
    AWL_ASSERT(sequences.nextId("orders") == 1);
    AWL_ASSERT(sequences.nextId("bots") == 101);
    AWL_ASSERT(sequences.nextId("orders") == 2);
    sequences.createSequence("orders", 1000);
    sqlite::SequenceTable::createScheme().create(*db);
    AWL_ASSERT(sequences.nextId("orders") == 3);
    AWL_ASSERT(sequences.nextId("bots") == 102);
    // Names are bound values, including SQL punctuation and non-ASCII text.
    const std::string name = "orders'; DROP TABLE sequences; -- \xD0\xB1\xD0\xBE\xD1\x82";
    sequences.createSequence(name, 10);
    AWL_ASSERT(sequences.nextId(name) == 11);
    AWL_ASSERT(sequences.nextId("orders") == 4);
}

AWL_TEST(SequenceTableErrorsAndOverflow)
{
    const std::shared_ptr<sqlite::Database> db = createDatabase(context.logger);
    sqlite::SequenceTable sequences(db);
    awl::testing::Assert::throws<sqlite::SQLiteException>([&] { sequences.nextId("missing"); });
    awl::testing::Assert::throws<sqlite::SQLiteException>([&] { sequences.createSequence("negative", -1); });
    sequences.createSequence("missing");
    AWL_ASSERT(sequences.nextId("missing") == 1);
    sequences.createSequence("last", std::numeric_limits<int64_t>::max() - 1);
    AWL_ASSERT(sequences.nextId("last") == std::numeric_limits<int64_t>::max());

    for (int attempt = 0; attempt < 2; ++attempt)
    {
        awl::testing::Assert::throws<sqlite::SQLiteException>([&] { sequences.nextId("last"); });
    }

    sequences.createSequence("last", 0);
    awl::testing::Assert::throws<sqlite::SQLiteException>([&] { sequences.nextId("last"); });
    AWL_ASSERT(sequences.nextId("missing") == 2);
}

AWL_TEST(SequenceTableTransactions)
{
    const std::shared_ptr<sqlite::Database> db = createDatabase(context.logger);
    sqlite::SequenceTable sequences(db);
    sequences.createSequence("orders");

    {
        sqlite::TransactionGuard transaction(db);
        AWL_ASSERT(sequences.nextId("orders") == 1);
        sequences.createSequence("rolled_back");
    }

    AWL_ASSERT(sequences.nextId("orders") == 1);
    awl::testing::Assert::throws<sqlite::SQLiteException>([&] { sequences.nextId("rolled_back"); });

    {
        sqlite::TransactionGuard transaction(db);
        AWL_ASSERT(sequences.nextId("orders") == 2);
        transaction.commit();
    }

    AWL_ASSERT(sequences.nextId("orders") == 3);
}

AWL_TEST(SequenceTableReopen)
{
    TemporaryDatabase file;

    {
        const std::shared_ptr<sqlite::Database> db = createDatabase(context.logger, file.filename());
        sqlite::SequenceTable sequences(db);
        sequences.createSequence("orders", 40);
        AWL_ASSERT(sequences.nextId("orders") == 41);
    }

    {
        const std::shared_ptr<sqlite::Database> db = createDatabase(context.logger, file.filename());
        sqlite::SequenceTable sequences(db);
        sequences.createSequence("orders");
        AWL_ASSERT(sequences.nextId("orders") == 42);
    }
}

AWL_TEST(SequenceTableReusesStatementsAfterSqlErrors)
{
    const std::shared_ptr<sqlite::Database> db = createDatabase(context.logger);
    // Install the schema before preparing the cached statements; toggle only data later.
    db->exec(
        "CREATE TABLE sequence_failure (operation INTEGER NOT NULL);"
        "INSERT INTO sequence_failure VALUES (0);"
        "CREATE TRIGGER reject_sequence_insert BEFORE INSERT ON sequences "
        "WHEN (SELECT operation FROM sequence_failure) = 1 "
        "BEGIN SELECT RAISE(ABORT, 'sequence insertion failure'); END;"
        "CREATE TRIGGER reject_sequence_update BEFORE UPDATE ON sequences "
        "WHEN (SELECT operation FROM sequence_failure) = 2 "
        "BEGIN SELECT RAISE(ABORT, 'sequence update failure'); END;");
    sqlite::SequenceTable sequences(db);
    sequences.createSequence("orders");
    db->exec("UPDATE sequence_failure SET operation = 1;");
    awl::testing::Assert::throws<sqlite::SQLiteException>([&] { sequences.createSequence("bots"); });
    db->exec("UPDATE sequence_failure SET operation = 0;");
    sequences.createSequence("bots", 100);
    AWL_ASSERT(sequences.nextId("bots") == 101);
    db->exec("UPDATE sequence_failure SET operation = 2;");
    awl::testing::Assert::throws<sqlite::SQLiteException>([&] { sequences.nextId("orders"); });
    db->exec("UPDATE sequence_failure SET operation = 0;");
    AWL_ASSERT(sequences.nextId("orders") == 1);
    AWL_ASSERT(sequences.nextId("orders") == 2);
}

AWL_TEST(SequenceTableConcurrentConnections)
{
    TemporaryDatabase file;
    const std::shared_ptr<sqlite::Database> first_db = createDatabase(context.logger, file.filename());
    const std::shared_ptr<sqlite::Database> second_db = createDatabase(context.logger, file.filename());
    sqlite::SequenceTable first(first_db);
    sqlite::SequenceTable second(second_db);
    std::vector<int64_t> first_ids;
    std::vector<int64_t> second_ids;
    std::exception_ptr first_error;
    std::exception_ptr second_error;

    const auto generate = [](sqlite::SequenceTable& sequences, std::vector<int64_t>& ids, std::exception_ptr& error)
    {
        try
        {
            // Concurrent initialization must neither duplicate nor reset the counter.
            sequences.createSequence("orders");

            for (int index = 0; index < 50; ++index)
            {
                ids.push_back(sequences.nextId("orders"));
            }
        }
        catch (...)
        {
            error = std::current_exception();
        }
    };

    {
        std::jthread first_thread(generate, std::ref(first), std::ref(first_ids), std::ref(first_error));
        std::jthread second_thread(generate, std::ref(second), std::ref(second_ids), std::ref(second_error));
    }

    if (first_error)
    {
        std::rethrow_exception(first_error);
    }

    if (second_error)
    {
        std::rethrow_exception(second_error);
    }

    first_ids.insert(first_ids.end(), second_ids.begin(), second_ids.end());
    std::ranges::sort(first_ids);
    AWL_ASSERT(first_ids.size() == 100);

    for (std::size_t index = 0; index < first_ids.size(); ++index)
    {
        AWL_ASSERT(first_ids[index] == static_cast<int64_t>(index + 1));
    }

    AWL_ASSERT(first.nextId("orders") == 101);
}
