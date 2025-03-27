#include "SQLiteWrapper/Database.h"
#include "SQLiteWrapper/Bind.h"
#include "SQLiteWrapper/Get.h"
#include "SQLiteWrapper/Scalar.h"

#include <sstream>

using namespace sqlite;

void Database::Open(const char* fileName)
{
    const int rc = sqlite3_open(fileName, &m_db);

    if (rc != SQLITE_OK)
    {
        RaiseError(m_db, rc, awl::aformat() << "Can't open database '" << fileName << "'");
    }

    Notify(&Element::Create);
}

void Database::Close()
{
    if (m_db != nullptr)
    {
        // Close the statements.
        InvalidateScheme();

        sqlite3_close(m_db);
    }
}

void Database::Exec(const char * query)
{
    char *zErrMsg = nullptr;

    const int rc = ExecRaw(query, &zErrMsg);

    if (rc != SQLITE_OK)
    {
        //std::cout << "SQL error: " << sqlite3_errmsg(db) << "\n";
        SQLiteException e(rc, zErrMsg);
        sqlite3_free(zErrMsg);
        throw e;
    }
}

bool Database::TableExists(const char * name)
{
    int exists;

    // Creating a table invaidates prepared statements.
    if (!tableExistsStatement.IsOpen())
    {
        tableExistsStatement.Open(*this, "SELECT count(*) FROM sqlite_master WHERE type='table' AND name=?;");
    }

    Bind(tableExistsStatement, 0, name);
    SelectScalar(tableExistsStatement, exists);

    return exists != 0;
}

void Database::DropTable(const char* name, bool exists)
{
    std::ostringstream out;

    out << "DROP TABLE";

    if (!exists)
    {
        out << " IF EXISTS";
    }

    out << " " << name << ";";

    Exec(out.str());

    InvalidateScheme();
}

bool Database::IndexExists(const char * name)
{
    int exists;

    if (!indexExistsStatement.IsOpen())
    {
        indexExistsStatement.Open(*this, "SELECT count(*) FROM sqlite_master WHERE type='index' AND name=?;");
    }

    Bind(indexExistsStatement, 0, name);
    SelectScalar(indexExistsStatement, exists);

    return exists != 0;
}

void Database::DropIndex(const char* name, bool exists)
{
    std::ostringstream out;

    out << "DROP INDEX";

    if (!exists)
    {
        out << " IF EXISTS";
    }

    out << " " << name << ";";

    Exec(out.str());

    InvalidateScheme();
}

[[noreturn]]
void Database::RaiseError(sqlite3* db, int code, std::string message)
{
    std::string user_message = awl::aformat() << message << ", Error message: " << sqlite3_errmsg(db) << ".";

    throw SQLiteException(code, user_message);
}
