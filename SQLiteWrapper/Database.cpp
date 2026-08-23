#include "SQLiteWrapper/Database.h"
#include "SQLiteWrapper/Bind.h"
#include "SQLiteWrapper/Get.h"
#include "SQLiteWrapper/Scalar.h"

#include <format>
#include <sstream>

using namespace sqlite;

void Database::open(const char* fileName)
{
    const int rc = sqlite3_open(fileName, &_db);

    if (rc != SQLITE_OK)
    {
        raiseError(_db, rc, std::format("Can't open database '{}'", fileName));
    }
}

void Database::close()
{
    if (_db != nullptr)
    {
        // Close the statements.
        invalidateScheme();

        const int rc = sqlite3_close(_db);

        if (rc == SQLITE_OK)
        {
            _logger->debug("sqlite3_close succeeded.");
            _db = nullptr;
        }
        else
        {
            _logger->error("sqlite3_close failed with code {}: {}", rc, sqlite3_errmsg(_db));
        }
    }
    else
    {
        _logger->debug("sqlite3_close skipped: database is not open.");
    }
}

void Database::exec(const char * query)
{
    char *zErrMsg = nullptr;

    const int rc = execRaw(query, &zErrMsg);

    if (rc != SQLITE_OK)
    {
        //std::cout << "SQL error: " << sqlite3_errmsg(db) << "\n";
        SQLiteException e(rc, zErrMsg);
        sqlite3_free(zErrMsg);
        throw e;
    }
}

void Database::setJournalMode(const std::string& journal_mode)
{
    exec(std::format("PRAGMA journal_mode = {};", journal_mode));
}

void Database::setCacheSize(const int cache_size)
{
    exec(std::format("PRAGMA cache_size = {};", cache_size));
}

void Database::setForeignKeys(const bool enabled)
{
    exec(std::format("PRAGMA foreign_keys = {};", enabled ? "ON" : "OFF"));
}

void Database::setPageSize(const size_t page_size)
{
    exec(std::format("PRAGMA page_size = {};", page_size));
}

bool Database::tableExists(const char * name)
{
    int exists;

    // Creating a table invaidates prepared statements.
    if (!tableExistsStatement.isOpen())
    {
        tableExistsStatement.open(*this, "SELECT count(*) FROM sqlite_master WHERE type='table' AND name=?;");
    }

    bind(tableExistsStatement, 0, name);
    selectScalar(tableExistsStatement, exists);

    return exists != 0;
}

void Database::dropTable(const char* name, const bool exists)
{
    std::ostringstream out;

    out << "DROP TABLE";

    if (!exists)
    {
        out << " IF EXISTS";
    }

    out << " " << name << ";";

    exec(out.str());

    invalidateScheme();
}

bool Database::indexExists(const char * name)
{
    int exists;

    if (!indexExistsStatement.isOpen())
    {
        indexExistsStatement.open(*this, "SELECT count(*) FROM sqlite_master WHERE type='index' AND name=?;");
    }

    bind(indexExistsStatement, 0, name);
    selectScalar(indexExistsStatement, exists);

    return exists != 0;
}

void Database::dropIndex(const char* name, const bool exists)
{
    std::ostringstream out;

    out << "DROP INDEX";

    if (!exists)
    {
        out << " IF EXISTS";
    }

    out << " " << name << ";";

    exec(out.str());

    invalidateScheme();
}

[[noreturn]]
void Database::raiseError(sqlite3* db, const int code, const std::string message)
{
    std::string user_message = std::format("{}, Error message: {}.", message, sqlite3_errmsg(db));

    throw SQLiteException(code, user_message);
}
