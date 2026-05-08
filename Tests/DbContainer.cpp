#include "DbContainer.h"
#include "SQLiteWrapper/Bind.h"

#include "Awl/Testing/UnitTest.h"
#include "Awl/String.h"

namespace swtest
{
    using namespace sqlite;

    //Inserts 1000 row by default.
    void DbContainer::FillDatabase(size_t batchCount, size_t transactionCount)
    {
        //db().exec("PRAGMA synchronous = OFF;");

        db().exec("create table myTable (id INTEGER PRIMARY KEY AUTOINCREMENT, FirstName varchar(30), LastName varchar(30), Age smallint, Hometown varchar(30), Job varchar(30))");

        //exec("CREATE INDEX i_fn ON myTable(FirstName)");
        //exec("CREATE INDEX i_ln ON myTable(LastName)");
        db().exec("CREATE INDEX i_ln ON myTable(Age)");

        Statement stmt(db(), "insert into myTable (FirstName, LastName, Age, Hometown, Job) values (?, ?, ?, ?, ?);");

        auto insert = [&stmt, this](const char * f, const char * l, int age, const char * home_town, const char * job)
        {
            std::uniform_int_distribution<size_t> first_name_dist(0, 1000);
            std::uniform_int_distribution<size_t> last_name_dist(0, 100000);
            std::uniform_int_distribution<size_t> age_dist(0, 100000);

            const std::string randomFirstName = std::string(f) + std::to_string(first_name_dist(awl::random()));
            const std::string randomLastName = std::string(l) + std::to_string(last_name_dist(awl::random()));
            const size_t randomAge = age + age_dist(awl::random());

            _ages.push_back(randomAge);

            if (_insertWithBinding)
            {
                sqlite::bind(stmt, 0, randomFirstName);
                sqlite::bind(stmt, 1, randomLastName);
                sqlite::bind(stmt, 2, randomAge);
                sqlite::bind(stmt, 3, home_town);
                sqlite::bind(stmt, 4, job);

                stmt.exec();
            }
            else
            {
                std::string query(awl::aformat() << "insert into myTable (FirstName, LastName, Age, Hometown, Job) values ('" <<
                    randomFirstName << "', '" <<
                    randomLastName << "', " <<
                    sqlite::helpers::makeSigned(randomAge) << ", '" <<
                    home_town << "', '" <<
                    job << "');");

                db().exec(query.c_str());
            }
        };

        auto insertBatch = [&insert]()
        {
            insert("Peter", "Griffin", 41, "Newport", "Brewery");
            insert("Lois", "Griffin", 40, "Quahog", "Piano Teacher");
            insert("Joseph", "Swanson", 39, "Quahog", "Police Officer");
            insert("Glenn", "Quagmire", 41, "Quahog", "Pyaniy Pilot");
            insert("Vasya", "Nastroykegulyal", 11, "Partizansk", "Gopnik");
        };

        awl::StopWatch sw;

        for (size_t j = 0; j < transactionCount; ++j)
        {
            db().beginTransaction();

            for (size_t i = 0; i < batchCount; ++i)
            {
                insertBatch();
            }

            db().commit();

            //std::ostringstream out;

            //out << j << ": " << sw.elapsedSeconds<int>() << "s, ";

            //Logger::WriteMessage(out.str().c_str());

            sw.reset();
        }
    }

    void DbContainer::SetAttributes(const awl::testing::TestContext & context)
    {
        AWL_ATTRIBUTE(awl::String, synchronous, _T("FULL"));
        AWL_ATTRIBUTE(awl::String, journal_mode, _T("DELETE"));

        _db->exec(awl::aformat() << "PRAGMA synchronous = " << awl::toAString(synchronous) << ";");
        _db->exec(awl::aformat() << "PRAGMA journal_mode = " << awl::toAString(journal_mode) << ";");
    }
}
