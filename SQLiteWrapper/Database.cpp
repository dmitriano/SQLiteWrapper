#include "SQLiteWrapper/Database.h"
#include "SQLiteWrapper/Statement.h"
#include "SQLiteWrapper/Bind.h"
#include "SQLiteWrapper/Get.h"

#include <sstream>

namespace sqlite
{
    [[noreturn]] void RaiseError(sqlite3 * db, int code, std::string message)
    {
        std::string user_message = awl::aformat() << message << ", Error message: " << sqlite3_errmsg(db) << ".";
        
        throw SQLiteException(code, user_message);
    }
}

using namespace sqlite;

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

    Statement rs(*this, "SELECT count(*) FROM sqlite_master WHERE type='table' AND name=?;");
    Bind(rs, 0, name);
    SelectScalar(rs, exists);

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
}

bool Database::IndexExists(const char * name)
{
    int exists;

    Statement rs(*this, "SELECT count(*) FROM sqlite_master WHERE type='index' AND name=?;");
    Bind(rs, 0, name);
    SelectScalar(rs, exists);

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
}
