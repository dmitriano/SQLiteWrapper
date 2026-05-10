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
            _stmt(std::move(other._stmt))
        {
            other._stmt = nullptr;
        }

        Statement& operator = (Statement&& other)
        {
            _stmt = other._stmt;
            other._stmt = nullptr;

            return *this;
        }

        ~Statement()
        {
            close();
        }

        bool Isopen() const
        {
            return _stmt != nullptr;
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
                sqlite3_finalize(_stmt);

                _stmt = nullptr;
            }
        }

        void bindNull(size_t col)
        {
            checkBind(sqlite3_bind_null(_stmt, from0To1(col)));
        }

        void bindInt(size_t col, int val)
        {
            checkBind(sqlite3_bind_int(_stmt, from0To1(col), val));
        }

        void bindInt64(size_t col, sqlite3_int64 val)
        {
            checkBind(sqlite3_bind_int64(_stmt, from0To1(col), val));
        }

        void bindDouble(size_t col, double val)
        {
            checkBind(sqlite3_bind_double(_stmt, from0To1(col), val));
        }

        void bindText(size_t col, const char * val)
        {
            // SQLITE_STATIC: the caller must keep val alive until the statement is executed.
            checkBind(sqlite3_bind_text(_stmt, from0To1(col), val, -1, nullptr));
        }

        void bindTextCopy(size_t col, const char* val)
        {
            // For computed strings whose temporary storage cannot outlive sqlite3_step().
            checkBind(sqlite3_bind_text(_stmt, from0To1(col), val, -1, SQLITE_TRANSIENT));
        }

        void bindBlob(size_t col, const std::vector<uint8_t>& v)
        {
            checkBind(sqlite3_bind_blob(_stmt, from0To1(col), v.data(), static_cast<int>(v.size()), SQLITE_STATIC));
        }

        bool next()
        {
            const int rc = sqlite3_step(_stmt);
            
            switch (rc)
            {
                case SQLITE_ROW:
                    return true;

                case SQLITE_DONE:
                    return false;
            }

            raiseError(rc, "Error while iterating over a record set.");
        }

        bool tryExec()
        {
            const int rc = sqlite3_step(_stmt);

            if (rc != SQLITE_DONE)
            {
                sqlite3_reset(_stmt);

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
            internalExec(false);
        }

        void exec()
        {
            internalExec(true);
        }

        void reset()
        {
            const int rc = sqlite3_reset(_stmt);

            if (rc != SQLITE_OK)
            {
                raiseError(rc, "Error while resetting a statement.");
            }
        }

        void clearBindings()
        {
            const int rc = sqlite3_clear_bindings(_stmt);

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

            return sqlite3_column_int(_stmt, from0To0(col));
        }

        sqlite3_int64 int64Value(size_t col) const
        {
            assert(isInt(col));

            return sqlite3_column_int64(_stmt, from0To0(col));
        }

        double doubleValue(size_t col) const
        {
            assert(isFloat(col));

            return sqlite3_column_double(_stmt, from0To0(col));
        }

        const char * textValue(size_t col) const
        {
            // Empty string is not Null.
            assert(isText(col));

            return reinterpret_cast<const char *>(sqlite3_column_text(_stmt, from0To0(col)));
        }

        const std::vector<uint8_t> blobValue(size_t col) const
        {
            //When we insert an empty std::vector it becomes Null.
            assert(isNull(col) || isBlob(col));

            const size_t size = static_cast<size_t>(sqlite3_column_bytes(_stmt, from0To0(col)));
            
            const uint8_t* buffer = reinterpret_cast<const uint8_t*>(sqlite3_column_blob(_stmt, from0To0(col)));

            return std::vector<uint8_t>(buffer, buffer + size);
        }

        [[noreturn]] void raiseError(int code, std::string message);

        [[noreturn]] void raiseError(std::string message);

    private:

        void internalExec(bool auto_reset)
        {
            const int rc = sqlite3_step(_stmt);

            if (rc != SQLITE_DONE)
            {
                if (auto_reset)
                {
                    //If the query failed sqlite3_reset also returns an error, but we ignore it.
                    sqlite3_reset(_stmt);
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

        void checkBind(int rc)
        {
            if (rc != SQLITE_OK)
            {
                raiseError(rc, "Bining error.");
            }
        }

        //The index is zero-based.
        int columnType(size_t col) const
        {
            const int column_type = sqlite3_column_type(_stmt, from0To0(col));

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

        sqlite3_stmt * _stmt = nullptr;
    };
}
