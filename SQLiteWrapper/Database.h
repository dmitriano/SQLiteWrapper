#pragma once

#include "sqlite3.h"

#include "SQLiteWrapper/Types.h"
#include "SQLiteWrapper/Exception.h"
#include "SQLiteWrapper/Element.h"
#include "SQLiteWrapper/Statement.h"

#include "Awl/StringFormat.h"
#include "Awl/Observable.h"
#include "Awl/ScopeGuard.h"
#include "Awl/Logger.h"

#define SQL_QUERY(src) #src

namespace sqlite
{
    [[noreturn]] void RaiseError(sqlite3 * db, int code, std::string message);

    [[noreturn]] inline void RaiseError(sqlite3 * db, std::string message)
    {
        RaiseError(db, 0, message);
    }

    class Database : public awl::Observable<Element, Database>
    {
    public:
        
        Database(awl::Logger& logger) : m_logger(logger) {}
        
        Database(const char * fileName, awl::Logger& logger) : Database(logger)
        {
            Open(fileName);
        }
        
        ~Database()
        {
            Close();
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

        void Open(const char* fileName);

        void Close();

        void Clear()
        {
            Notify(&Element::Delete);
        }

        void Begin()
        {
            Exec("BEGIN;");
        }

        void Commit()
        {
            Exec("COMMIT;");
        }

        void Rollback()
        {
            Exec("ROLLBACK;");
        }

        // Begin transaction
        void SavePoint(const char* savepoint)
        {
            Exec(awl::aformat() << "SAVEPOINT " << savepoint << ";");
        }

        // Commit changes.
        void Release(const char* savepoint)
        {
            Exec(awl::aformat() << "RELEASE " << savepoint << ";");
        }

        void RollbackTo(const char* savepoint)
        {
            Exec(awl::aformat() << "ROLLBACK TO " << savepoint << ";");
        }

        // The BEGIN command only works if the transaction stack is empty.
        template <class Func>
        void TryOutermost(Func && func)
        {
            Begin();
            
            try
            {
                func();
            }
            catch (const std::exception &)
            {
                Rollback();

                throw;
            }

            Commit();
        }

        // A thrown exception causes rollback.
        template <class Func>
        void Try(Func&& func, std::string savepoint = {})
        {
            ++m_transactionLevel;

            auto guard = awl::make_scope_guard([this] { --m_transactionLevel; });

            if (savepoint.empty())
            {
                savepoint = awl::aformat() << "sp" << m_transactionLevel;
            }

            SavePoint(savepoint.c_str());

            try
            {
                func();
            }
            catch (const std::exception&)
            {
                RollbackTo(savepoint.c_str());

                throw;
            }

            Release(savepoint.c_str());
        }

        int ExecRaw(const char* query, char** errmsg = nullptr)
        {
            return sqlite3_exec(m_db, query, nullptr, 0, errmsg);
        }

        void ExecRaw(const std::string& query, char** errmsg = nullptr)
        {
            ExecRaw(query.c_str(), errmsg);
        }

        void Exec(const std::string & query)
        {
            Exec(query.c_str());
        }
        
        void Exec(const char * query);

        bool TableExists(const char * name);

        bool TableExists(const std::string & name)
        {
            return TableExists(name.c_str());
        }

        void DropTable(const char* name, bool exists = false);

        void DropTable(const std::string& name, bool exists = false)
        {
            DropTable(name.c_str(), exists);
        }

        bool IndexExists(const char * name);

        bool IndexExists(const std::string & name)
        {
            return IndexExists(name.c_str());
        }

        void DropIndex(const char* name, bool exists = false);

        void DropIndex(const std::string& name, bool exists = false)
        {
            DropIndex(name.c_str(), exists);
        }

        void CreateFunction(const char* zFunc, int nArg, void (*xSFunc)(sqlite3_context*, int, sqlite3_value**))
        {
            const int rc = sqlite3_create_function(m_db, zFunc, nArg, SQLITE_UTF8, NULL, xSFunc, NULL, NULL);

            if (rc != SQLITE_OK)
            {
                RaiseError(m_db, rc, awl::aformat() << "Can't create function '" << zFunc << "'");
            }
        }

        //Returns the number of rows modified, inserted or deleted by the most recently completed INSERT, UPDATE or DELETE statement.
        int GetAffectedCount() const
        {
            return sqlite3_changes(m_db);
        }

        void EnsureAffected(int expected)
        {
            const int count = GetAffectedCount();

            if (count != expected)
            {
                throw SQLiteException(0, awl::aformat() << count << " rows have been affected, but expected " << expected << ".");
            }
        }

        RowId GetLastRowId() const
        {
            return sqlite3_last_insert_rowid(m_db);
        }

        awl::Logger& logger()
        {
            return m_logger;
        }

    private:

        std::reference_wrapper<awl::Logger> m_logger;

        sqlite3 * m_db = nullptr;

        std::size_t m_transactionLevel = 0u;

        Statement tableExistsStatement;
        Statement indexExistsStatement;

        friend Statement;
    };
}
