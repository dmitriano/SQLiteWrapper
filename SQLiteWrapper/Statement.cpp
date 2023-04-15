#include "SQLiteWrapper/Statement.h"

using namespace sqlite;

[[noreturn]] void Statement::RaiseError(int code, std::string message)
{
    assert(IsOpen());

    sqlite3 * db = sqlite3_db_handle(m_stmt);

    sqlite::RaiseError(db, code, message);
}

[[noreturn]] void Statement::RaiseError(std::string message)
{
    //We do not have the code and do not have the last error.
    throw SQLiteException(0, message);
}
