#pragma once

#include "sqlite3.h"

#include "SQLiteWrapper/Types.h"
#include "SQLiteWrapper/Exception.h"
#include "SQLiteWrapper/Element.h"
#include "SQLiteWrapper/Statement.h"

#include "Awl/LegacyFormat.h"
#include "Awl/Observable.h"
#include "Awl/ScopeGuard.h"
#include "Awl/Logger.h"

namespace sqlite
{
    class Database : public awl::Observable<Element, Database>
    {
    public:
        
        Database(awl::Logger& logger) : m_logger(logger) {}
        
        Database(const char * fileName, awl::Logger& logger) : Database(logger)
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
            m_logger(other.m_logger),
            m_db(std::move(other.m_db))
        {
            other.m_db = nullptr;
        }

        Database& operator = (Database && other)
        {
            m_db = other.m_db;
            other.m_db = nullptr;
            return *this;
        }

        void open(const char* fileName);

        void close();

        void clear()
        {
            notify(&Element::deleteElement, std::ref(*this));
        }

        void beginTransaction()
        {
            exec("BEGIN;");
        }

        void commit()
        {
            exec("COMMIT;");
        }

        void rollback()
        {
            exec("ROLLBACK;");
        }

        // Begin transaction
        void savePoint(const char* savepoint)
        {
            exec(awl::aformat() << "SAVEPOINT " << savepoint << ";");
        }

        // Commit changes.
        void release(const char* savepoint)
        {
            exec(awl::aformat() << "RELEASE " << savepoint << ";");
        }

        void rollbackTo(const char* savepoint)
        {
            exec(awl::aformat() << "ROLLBACK TO " << savepoint << ";");
        }

        // The BEGIN command only works if the transaction stack is empty.
        template <class Func>
        void tryOutermost(Func && func)
        {
            beginTransaction();
            
            try
            {
                func();
            }
            catch (const std::exception &)
            {
                rollback();

                throw;
            }

            commit();
        }

        // A thrown exception causes rollback.
        template <class Func>
        void tryRun(Func&& func, std::string savepoint = {})
        {
            ++m_transactionLevel;

            auto guard = awl::make_scope_guard([this] { --m_transactionLevel; });

            if (savepoint.empty())
            {
                savepoint = awl::aformat() << "sp" << m_transactionLevel;
            }

            savePoint(savepoint.c_str());

            try
            {
                func();
            }
            catch (const std::exception&)
            {
                rollbackTo(savepoint.c_str());

                throw;
            }

            release(savepoint.c_str());
        }

        int execRaw(const char* query, char** errmsg = nullptr)
        {
            return sqlite3_exec(m_db, query, nullptr, 0, errmsg);
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
            const int rc = sqlite3_create_function(m_db, zFunc, nArg, SQLITE_UTF8, NULL, xSFunc, NULL, NULL);

            if (rc != SQLITE_OK)
            {
                raiseError(m_db, rc, awl::aformat() << "Can't create function '" << zFunc << "'");
            }
        }

        //Returns the number of rows modified, inserted or deleted by the most recently completed INSERT, UPDATE or DELETE statement.
        int affectedCount() const
        {
            return sqlite3_changes(m_db);
        }

        void ensureAffected(int expected)
        {
            const int count = affectedCount();

            if (count != expected)
            {
                throw SQLiteException(0, awl::aformat() << count << " rows have been affected, but expected " << expected << ".");
            }
        }

        RowId lastRowId() const
        {
            return sqlite3_last_insert_rowid(m_db);
        }

        awl::Logger& logger()
        {
            return m_logger;
        }

        // Should be called after CREATE TABLE.
        void invalidateScheme()
        {
            tableExistsStatement.close();
            indexExistsStatement.close();
        }

    private:

        [[noreturn]]
        static void raiseError(sqlite3* db, int code, std::string message);

        [[noreturn]]
        static void raiseError(sqlite3* db, std::string message)
        {
            raiseError(db, 0, message);
        }

        std::reference_wrapper<awl::Logger> m_logger;

        sqlite3 * m_db = nullptr;

        std::size_t m_transactionLevel = 0u;

        Statement tableExistsStatement;
        Statement indexExistsStatement;

        friend Statement;
    };
}

