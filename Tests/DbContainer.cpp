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
        //db().Exec("PRAGMA synchronous = OFF;");

        db().Exec("create table myTable (id INTEGER PRIMARY KEY AUTOINCREMENT, FirstName varchar(30), LastName varchar(30), Age smallint, Hometown varchar(30), Job varchar(30))");

        //exec("CREATE INDEX i_fn ON myTable(FirstName)");
        //exec("CREATE INDEX i_ln ON myTable(LastName)");
        db().Exec("CREATE INDEX i_ln ON myTable(Age)");

        Statement stmt(db(), "insert into myTable (FirstName, LastName, Age, Hometown, Job) values (?, ?, ?, ?, ?);");

        auto insert = [&stmt, this](const char * f, const char * l, int age, const char * home_town, const char * job)
        {
            std::uniform_int_distribution<size_t> first_name_dist(0, 1000);
            std::uniform_int_distribution<size_t> last_name_dist(0, 100000);
            std::uniform_int_distribution<size_t> age_dist(0, 100000);

            const std::string randomFirstName = std::string(f) + std::to_string(first_name_dist(awl::random()));
            const std::string randomLastName = std::string(l) + std::to_string(last_name_dist(awl::random()));
            const size_t randomAge = age + age_dist(awl::random());

            m_ages.push_back(randomAge);

            if (m_insertWithBinding)
            {
                Bind(stmt, 0, randomFirstName);
                Bind(stmt, 1, randomLastName);
                Bind(stmt, 2, randomAge);
                Bind(stmt, 3, home_town);
                Bind(stmt, 4, job);

                stmt.Exec();
            }
            else
            {
                std::string query(awl::aformat() << "insert into myTable (FirstName, LastName, Age, Hometown, Job) values ('" <<
                    randomFirstName << "', '" <<
                    randomLastName << "', " <<
                    sqlite::helpers::MakeSigned(randomAge) << ", '" <<
                    home_town << "', '" <<
                    job << "');");

                db().Exec(query.c_str());
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
            db().Begin();

            for (size_t i = 0; i < batchCount; ++i)
            {
                insertBatch();
            }

            db().Commit();

            //std::ostringstream out;

            //out << j << ": " << sw.GetElapsedSeconds<int>() << "s, ";

            //Logger::WriteMessage(out.str().c_str());

            sw.Reset();
        }
    }

    void DbContainer::SetAttributes(const awl::testing::TestContext & context)
    {
        AWL_ATTRIBUTE(awl::String, synchronous, _T("FULL"));
        AWL_ATTRIBUTE(awl::String, journal_mode, _T("DELETE"));

        m_db->Exec(awl::aformat() << "PRAGMA synchronous = " << awl::ToAString(synchronous) << ";");
        m_db->Exec(awl::aformat() << "PRAGMA journal_mode = " << awl::ToAString(journal_mode) << ";");
    }
}
