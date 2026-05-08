#include "SQLiteWrapper/Database.h"
#include "SQLiteWrapper/Statement.h"
#include "SQLiteWrapper/TableBuilder.h"
#include "SQLiteWrapper/QueryBuilder.h"

#include "Awl/StopWatch.h"
#include "Awl/Random.h"
#include "Awl/Reflection.h"
#include "Awl/LegacyFormat.h"

#include "Awl/Testing/UnitTest.h"

#include <vector>
#include <algorithm>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <memory>

namespace swtest
{
    using namespace sqlite;

    class DbContainer
    {
    public:

        explicit DbContainer(std::shared_ptr<awl::Logger> logger)
        {
            RemoveFile();
            _db = std::make_shared<Database>(fileName, std::move(logger));
        }

        DbContainer(const awl::testing::TestContext& context) : DbContainer(context.logger)
        {
            SetAttributes(context);
        }

        ~DbContainer()
        {
            _db->close();
            RemoveFile();
        }

        Database& db()
        {
            return *_db;
        }

        //Inserts 1000 row by default.
        void FillDatabase(size_t batchCount = 20, size_t transactionCount = 10);

        void SetAttributes(const awl::testing::TestContext & context);

        std::shared_ptr<Database> _db;

        std::vector<size_t> _ages;

        bool _insertWithBinding = true;

    private:

        void RemoveFile() const
        {
            std::filesystem::remove(fileName);
        }

        static constexpr char fileName[] = "test.db";
    };
}

