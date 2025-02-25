#pragma once

#include "sqlite3.h"

#include "SQLiteWrapper/Types.h"
#include "SQLiteWrapper/Exception.h"
#include "SQLiteWrapper/Element.h"

#include "Awl/StringFormat.h"
#include "Awl/Observable.h"

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
        
        Database() = default;
        
        Database(const char * fileName)
        {
            Open(fileName);
        }
        
        ~Database()
        {
            Close();
        }

        void Open(const char * fileName)
        {
            const int rc = sqlite3_open(fileName, &m_db);

            if (rc != SQLITE_OK)
            {
                RaiseError(m_db, rc, awl::aformat() << "Can't open database '" << fileName << "'");
            }

            Notify(&Element::Create);
            Prepare();
        }

        void Close()
        {
            if (m_db != nullptr)
            {
                Notify(&Element::Close);

                sqlite3_close(m_db);
            }
        }

        void Prepare()
        {
            Notify(&Element::Open);
        }

        void Clear()
        {
            Notify(&Element::Delete);
        }

        Database(const Database&) = delete;

        Database& operator = (const Database&) = delete;

        Database(Database&& other) : m_db(std::move(other.m_db))
        {
            other.m_db = nullptr;
        }

        Database& operator = (Database && other)
        {
            m_db = other.m_db;
            other.m_db = nullptr;
            return *this;
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
        void Save(const char* savepoint)
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

        //A thrown exception causes rollback.
        template <class Func>
        void Try(Func && func)
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

        //A thrown exception causes rollback.
        template <class Func>
        void Try(Func&& func, const char* savepoint)
        {
            Save(savepoint);

            try
            {
                func();
            }
            catch (const std::exception&)
            {
                RollbackTo(savepoint);

                throw;
            }

            Release(savepoint);
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

    private:

        sqlite3 * m_db = nullptr;

        friend class Statement;
    };
}
