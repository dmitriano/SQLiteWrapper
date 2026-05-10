#include "SQLiteWrapper/Statement.h"
#include "SQLiteWrapper/Database.h"
using namespace sqlite;

[[noreturn]] void Statement::raiseError(int code, std::string message)
{
    assert(Isopen());

    sqlite3 * db = sqlite3_db_handle(_stmt);

    Database::raiseError(db, code, message);
}

[[noreturn]] void Statement::raiseError(std::string message)
{
    //We do not have the code and do not have the last error.
    throw SQLiteException(0, message);
}

void Statement::open(Database& db, const char* query)
{
    const int rc = sqlite3_prepare(db._db, query, -1, &_stmt, NULL);

    if (rc != SQLITE_OK)
    {
        Database::raiseError(db._db, rc, awl::aformat() << "Error while preparing SQL query: '" << query << "'.");
    }
}
