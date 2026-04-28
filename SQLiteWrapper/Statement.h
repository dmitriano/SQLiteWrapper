#pragma once

#include "sqlite3.h"

#include "SQLiteWrapper/Exception.h"

#include "Awl/TupleHelpers.h"

#include <stdint.h>
#include <type_traits>
#include <vector>
#include <limits>
#include <chrono>

namespace sqlite
{
    class Database;

    class Statement
    {
    public:
        
        Statement() = default;
        
        Statement(Database& db, const char* query)
        {
            open(db, query);
        }

        Statement(Database& db, const std::string& query) : Statement(db, query.c_str()) {}
            
        Statement(const Statement&) = delete;

        Statement& operator = (const Statement&) = delete;

        Statement(Statement&& other) : 
            m_stmt(std::move(other.m_stmt))
        {
            other.m_stmt = nullptr;
        }

        Statement& operator = (Statement&& other)
        {
            m_stmt = other.m_stmt;
            other.m_stmt = nullptr;

            return *this;
        }

        ~Statement()
        {
            close();
        }

        bool Isopen() const
        {
            return m_stmt != nullptr;
        }
        
        void open(Database& db, const char* query);

        void open(Database& db, const std::string& query)
        {
            open(db, query.c_str());
        }

        void close()
        {
            if (Isopen())
            {
                sqlite3_finalize(m_stmt);

                m_stmt = nullptr;
            }
        }

        void bindNull(size_t col)
        {
            Checkbind(sqlite3_bind_null(m_stmt, from0To1(col)));
        }

        void bindInt(size_t col, int val)
        {
            Checkbind(sqlite3_bind_int(m_stmt, from0To1(col), val));
        }

        void bindInt64(size_t col, sqlite3_int64 val)
        {
            Checkbind(sqlite3_bind_int64(m_stmt, from0To1(col), val));
        }

        void bindDouble(size_t col, double val)
        {
            Checkbind(sqlite3_bind_double(m_stmt, from0To1(col), val));
        }

        void bindText(size_t col, const char * val)
        {
            // SQLITE_STATIC: the caller must keep val alive until the statement is executed.
            Checkbind(sqlite3_bind_text(m_stmt, from0To1(col), val, -1, nullptr));
        }

        void bindBlob(size_t col, const std::vector<uint8_t>& v)
        {
            Checkbind(sqlite3_bind_blob(m_stmt, from0To1(col), v.data(), static_cast<int>(v.size()), SQLITE_STATIC));
        }

        bool Next()
        {
            const int rc = sqlite3_step(m_stmt);
            
            switch (rc)
            {
                case SQLITE_ROW:
                    return true;

                case SQLITE_DONE:
                    return false;
            }

            raiseError(rc, "Error while iterating over a record set.");
        }

        bool tryexec()
        {
            const int rc = sqlite3_step(m_stmt);

            if (rc != SQLITE_DONE)
            {
                sqlite3_reset(m_stmt);

                //It does not return SQLITE_CONSTRAINT.
                if (rc != SQLITE_ERROR)
                {
                    raiseError(rc, "Error while trying to execute a statement.");
                }

                return false;
            }

            reset();

            return true;
        }

        void select()
        {
            Internalexec(false);
        }

        void exec()
        {
            Internalexec(true);
        }

        void reset()
        {
            const int rc = sqlite3_reset(m_stmt);

            if (rc != SQLITE_OK)
            {
                raiseError(rc, "Error while resetting a statement.");
            }
        }

        void clearBindings()
        {
            const int rc = sqlite3_clear_bindings(m_stmt);

            if (rc != SQLITE_OK)
            {
                raiseError(rc, "Error while clearing the bindings.");
            }
        }

        bool isNull(size_t col) const
        {
            return columnType(col) == SQLITE_NULL;
        }

        bool isInt(size_t col) const
        {
            return columnType(col) == SQLITE_INTEGER;
        }

        bool isFloat(size_t col) const
        {
            return columnType(col) == SQLITE_FLOAT;
        }

        bool isText(size_t col) const
        {
            return columnType(col) == SQLITE_TEXT;
        }

        bool isBlob(size_t col) const
        {
            return columnType(col) == SQLITE_BLOB;
        }

        int intValue(size_t col) const
        {
            assert(isInt(col));

            return sqlite3_column_int(m_stmt, from0To0(col));
        }

        sqlite3_int64 int64Value(size_t col) const
        {
            assert(isInt(col));

            return sqlite3_column_int64(m_stmt, from0To0(col));
        }

        double doubleValue(size_t col) const
        {
            assert(isFloat(col));

            return sqlite3_column_double(m_stmt, from0To0(col));
        }

        const char * textValue(size_t col) const
        {
            // Empty string is not Null.
            assert(isText(col));

            return reinterpret_cast<const char *>(sqlite3_column_text(m_stmt, from0To0(col)));
        }

        const std::vector<uint8_t> blobValue(size_t col) const
        {
            //When we insert an empty std::vector it becomes Null.
            assert(isNull(col) || isBlob(col));

            const size_t size = static_cast<size_t>(sqlite3_column_bytes(m_stmt, from0To0(col)));
            
            const uint8_t* buffer = reinterpret_cast<const uint8_t*>(sqlite3_column_blob(m_stmt, from0To0(col)));

            return std::vector<uint8_t>(buffer, buffer + size);
        }

        [[noreturn]] void raiseError(int code, std::string message);

        [[noreturn]] void raiseError(std::string message);

    private:

        void Internalexec(bool auto_reset)
        {
            const int rc = sqlite3_step(m_stmt);

            if (rc != SQLITE_DONE)
            {
                if (auto_reset)
                {
                    //If the query failed sqlite3_reset also returns an error, but we ignore it.
                    sqlite3_reset(m_stmt);
                }

                raiseError(rc, "Error while executing a statement.");
            }
            else
            {
                if (auto_reset)
                {
                    //sqlite3_reset should not return an error.
                    reset();
                }
            }
        }

        void Checkbind(int rc)
        {
            if (rc != SQLITE_OK)
            {
                raiseError(rc, "Bining error.");
            }
        }

        //The index is zero-based.
        int columnType(size_t col) const
        {
            const int column_type = sqlite3_column_type(m_stmt, from0To0(col));

            return column_type;
        }

        //Bind index is 1-based.
        static constexpr int from0To1(size_t col)
        {
            return static_cast<int>(col + 1);
        }

        //Get index is zero-based.
        static constexpr int from0To0(size_t col)
        {
            return static_cast<int>(col);
        }

        static constexpr size_t to0Col(int col)
        {
            assert(col > 0);
            return static_cast<size_t>(col - 1);
        }

        //const char * GetLastError() const
        //{
        //    assert(Isopen());
        //    
        //    sqlite3 * db = sqlite3_db_handle(m_stmt);

        //    return sqlite3_errmsg(db);
        //}

        sqlite3_stmt * m_stmt = nullptr;
    };
}
