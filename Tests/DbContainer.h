#include "SQLiteWrapper/Database.h"
#include "SQLiteWrapper/Statement.h"
#include "SQLiteWrapper/TableBuilder.h"
#include "SQLiteWrapper/QueryBuilder.h"

#include "Awl/StopWatch.h"
#include "Awl/Random.h"
#include "Awl/Stringizable.h"
#include "Awl/StringFormat.h"

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

        DbContainer() : m_db(std::make_shared<Database>(fileName))
        {
        }

        DbContainer(const awl::testing::TestContext & context) : DbContainer()
        {
            SetAttributes(context);
        }

        ~DbContainer()
        {
            m_db->Close();
            std::filesystem::remove(fileName);
        }

        Database& db()
        {
            return *m_db;
        }

        //Inserts 1000 row by default.
        void FillDatabase(size_t batchCount = 20, size_t transactionCount = 10);

        void SetAttributes(const awl::testing::TestContext & context);

        std::shared_ptr<Database> m_db;

        std::vector<size_t> m_ages;

        bool m_insertWithBinding = true;

    private:

        static constexpr char fileName[] = "test.db";
    };
}
