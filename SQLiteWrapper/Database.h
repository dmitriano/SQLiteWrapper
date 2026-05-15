#pragma once

#include "sqlite3.h"

#include "SQLiteWrapper/Exception.h"
#include "SQLiteWrapper/Statement.h"
#include "SQLiteWrapper/Types.h"

#include "Awl/ILogger.h"
#include "Awl/ITransaction.h"

#include <format>
#include <memory>
#include <stdexcept>

namespace sqlite
{
    class TransactionGuard;

    class Database :
        public awl::ITransactionProvider,
        public std::enable_shared_from_this<Database>
    {
    public:
        
        explicit Database(std::shared_ptr<awl::ILogger> logger) : _logger(std::move(logger))
        {
            if (!_logger)
            {
                throw std::invalid_argument("Database logger must not be null.");
            }
        }
        
        Database(const char * fileName, std::shared_ptr<awl::ILogger> logger) : Database(std::move(logger))
        {
            open(fileName);
        }
        
        ~Database()
        {
            close();
        }

        Database(const Database&) = delete;

        Database& operator = (const Database&) = delete;

        Database(Database&& other) :
            _logger(std::move(other._logger)),
            _db(std::move(other._db))
        {
            other._db = nullptr;
        }

        Database& operator = (Database && other)
        {
            _logger = std::move(other._logger);
            _db = other._db;
            other._db = nullptr;
            return *this;
        }

        void open(const char* fileName);

        void close();

        std::unique_ptr<awl::ITransaction> startTransaction() override;

        int execRaw(const char* query, char** errmsg = nullptr)
        {
            return sqlite3_exec(_db, query, nullptr, 0, errmsg);
        }

        void execRaw(const std::string& query, char** errmsg = nullptr)
        {
            execRaw(query.c_str(), errmsg);
        }

        void exec(const std::string & query)
        {
            exec(query.c_str());
        }
        
        void exec(const char * query);

        bool tableExists(const char * name);

        bool tableExists(const std::string & name)
        {
            return tableExists(name.c_str());
        }

        void dropTable(const char* name, bool exists = false);

        void dropTable(const std::string& name, bool exists = false)
        {
            dropTable(name.c_str(), exists);
        }

        bool indexExists(const char * name);

        bool indexExists(const std::string & name)
        {
            return indexExists(name.c_str());
        }

        void dropIndex(const char* name, bool exists = false);

        void dropIndex(const std::string& name, bool exists = false)
        {
            dropIndex(name.c_str(), exists);
        }

        void createFunction(const char* zFunc, int nArg, void (*xSFunc)(sqlite3_context*, int, sqlite3_value**))
        {
            const int rc = sqlite3_create_function(_db, zFunc, nArg, SQLITE_UTF8, NULL, xSFunc, NULL, NULL);

            if (rc != SQLITE_OK)
            {
                raiseError(_db, rc, std::format("Can't create function '{}'", zFunc));
            }
        }

        //Returns the number of rows modified, inserted or deleted by the most recently completed INSERT, UPDATE or DELETE statement.
        int affectedCount() const
        {
            return sqlite3_changes(_db);
        }

        void ensureAffected(int expected)
        {
            const int count = affectedCount();

            if (count != expected)
            {
                throw SQLiteException(0, std::format("{} rows have been affected, but expected {}.", count, expected));
            }
        }

        RowId lastRowId() const
        {
            return sqlite3_last_insert_rowid(_db);
        }

        const std::shared_ptr<awl::ILogger>& logger() const
        {
            return _logger;
        }

        // Should be called after CREATE TABLE.
        void invalidateScheme()
        {
            tableExistsStatement.close();
            indexExistsStatement.close();
        }

    private:

        // Begin transaction
        void savePoint(const char* savepoint)
        {
            exec(std::format("SAVEPOINT {};", savepoint));
        }

        // Commit changes.
        void release(const char* savepoint)
        {
            exec(std::format("RELEASE {};", savepoint));
        }

        void rollbackTo(const char* savepoint)
        {
            exec(std::format("ROLLBACK TO {};", savepoint));
        }

        [[noreturn]]
        static void raiseError(sqlite3* db, int code, std::string message);

        [[noreturn]]
        static void raiseError(sqlite3* db, std::string message)
        {
            raiseError(db, 0, message);
        }

        std::shared_ptr<awl::ILogger> _logger;

        sqlite3 * _db = nullptr;

        std::size_t _transactionLevel = 0u;

        Statement tableExistsStatement;
        Statement indexExistsStatement;

        friend Statement;
        friend TransactionGuard;
    };
}

