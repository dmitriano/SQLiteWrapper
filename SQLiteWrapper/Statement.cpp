#include "SQLiteWrapper/Statement.h"
#include "SQLiteWrapper/Database.h"

using namespace sqlite;

[[noreturn]] void Statement::RaiseError(int code, std::string message)
{
    assert(IsOpen());

    sqlite3 * db = sqlite3_db_handle(m_stmt);

    Database::RaiseError(db, code, message);
}

[[noreturn]] void Statement::RaiseError(std::string message)
{
    //We do not have the code and do not have the last error.
    throw SQLiteException(0, message);
}

void Statement::Open(Database& db, const char* query)
{
    const int rc = sqlite3_prepare(db.m_db, query, -1, &m_stmt, NULL);

    if (rc != SQLITE_OK)
    {
        Database::RaiseError(db.m_db, rc, awl::aformat() << "Error while preparing SQL query: '" << query << "'.");
    }
}
