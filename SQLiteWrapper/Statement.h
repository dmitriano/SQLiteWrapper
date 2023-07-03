#pragma once

#include "SQLiteWrapper/Exception.h"
#include "SQLiteWrapper/Database.h"

#include "Awl/TupleHelpers.h"
#include "Awl/QuickList.h"

#include <stdint.h>
#include <type_traits>
#include <vector>
#include <limits>
#include <chrono>
#include <any>

namespace sqlite
{
    class Statement
    {
    public:
        
        Statement() = default;
        
        Statement(Database& db, const char* query)
        {
            Open(db, query);
        }

        Statement(Database& db, const std::string& query) : Statement(db, query.c_str())
        {
        }
            
        Statement(const Statement&) = delete;

        Statement& operator = (const Statement&) = delete;

        Statement(Statement&& other) : 
            m_stmt(std::move(other.m_stmt)),
            usedValues(std::move(other.usedValues)),
            freeValues(std::move(other.freeValues))
        {
            other.m_stmt = nullptr;

            assert(other.usedValues.empty());
            assert(other.freeValues.empty());
        }

        Statement& operator = (Statement&& other)
        {
            m_stmt = other.m_stmt;
            other.m_stmt = nullptr;

            usedValues = std::move(other.usedValues);
            freeValues = std::move(other.freeValues);

            assert(other.usedValues.empty());
            assert(other.freeValues.empty());

            return *this;
        }

        ~Statement()
        {
            Close();

            ClearFreeValues();
        }

        bool IsOpen() const
        {
            return m_stmt != nullptr;
        }
        
        void Open(Database& db, const char* query)
        {
            const int rc = sqlite3_prepare(db.m_db, query, -1, &m_stmt, NULL);

            if (rc != SQLITE_OK)
            {
                sqlite::RaiseError(db.m_db, rc, awl::aformat() << "Error while preparing SQL query: '" << query << "'.");
            }
        }

        void Open(Database& db, const std::string& query)
        {
            Open(db, query.c_str());
        }

        void Close()
        {
            if (IsOpen())
            {
                sqlite3_finalize(m_stmt);

                m_stmt = nullptr;
            }
        }

        void BindNull(size_t col)
        {
            CheckBind(sqlite3_bind_null(m_stmt, From0To1(col)));
        }

        void BindInt(size_t col, int val)
        {
            CheckBind(sqlite3_bind_int(m_stmt, From0To1(col), val));
        }

        void BindInt64(size_t col, sqlite3_int64 val)
        {
            CheckBind(sqlite3_bind_int64(m_stmt, From0To1(col), val));
        }

        void BindDouble(size_t col, double val)
        {
            CheckBind(sqlite3_bind_double(m_stmt, From0To1(col), val));
        }

        void BindText(size_t col, const char * val)
        {
            CheckBind(sqlite3_bind_text(m_stmt, From0To1(col), val, -1, nullptr));
        }

        void BindBlob(size_t col, const std::vector<uint8_t>& v)
        {
            CheckBind(sqlite3_bind_blob(m_stmt, From0To1(col), v.data(), static_cast<int>(v.size()), SQLITE_STATIC));
        }

        bool Next()
        {
            const int rc = sqlite3_step(m_stmt);

            ClearUsedValues();
            
            switch (rc)
            {
                case SQLITE_ROW:
                    return true;

                case SQLITE_DONE:
                    return false;
            }

            RaiseError(rc, "Error while iterating over a record set.");
        }

        bool TryExec()
        {
            const int rc = sqlite3_step(m_stmt);

            ClearUsedValues();

            if (rc != SQLITE_DONE)
            {
                sqlite3_reset(m_stmt);

                //It does not return SQLITE_CONSTRAINT.
                if (rc != SQLITE_ERROR)
                {
                    RaiseError(rc, "Error while trying to execute a statement.");
                }

                return false;
            }

            Reset();

            return true;
        }

        void Select()
        {
            InternalExec(false);
        }

        void Exec()
        {
            InternalExec(true);
        }

        void Reset()
        {
            const int rc = sqlite3_reset(m_stmt);

            if (rc != SQLITE_OK)
            {
                RaiseError(rc, "Error while resetting a statement.");
            }
        }

        void ClearBindings()
        {
            const int rc = sqlite3_clear_bindings(m_stmt);

            if (rc != SQLITE_OK)
            {
                RaiseError(rc, "Error while clearing the bindings.");
            }
        }

        bool IsNull(int col) const
        {
            return GetColumnType(col) == SQLITE_NULL;
        }

        bool IsInt(int col) const
        {
            return GetColumnType(col) == SQLITE_INTEGER;
        }

        bool IsFloat(int col) const
        {
            return GetColumnType(col) == SQLITE_FLOAT;
        }

        bool IsText(int col) const
        {
            return GetColumnType(col) == SQLITE_TEXT;
        }

        bool IsBlob(int col) const
        {
            return GetColumnType(col) == SQLITE_BLOB;
        }

        int GetInt(size_t col) const
        {
            assert(IsInt(col));

            return sqlite3_column_int(m_stmt, From0To0(col));
        }

        sqlite3_int64 GetInt64(size_t col) const
        {
            assert(IsInt(col));

            return sqlite3_column_int64(m_stmt, From0To0(col));
        }

        double GetDouble(size_t col) const
        {
            assert(IsFloat(col));

            return sqlite3_column_double(m_stmt, From0To0(col));
        }

        const char * GetText(size_t col) const
        {
            assert(IsText(col));

            return reinterpret_cast<const char *>(sqlite3_column_text(m_stmt, From0To0(col)));
        }

        const std::vector<uint8_t> GetBlob(size_t col) const
        {
            //When we insert an empty std::vector it becomes Null.
            assert(IsNull(col) || IsBlob(col));

            const size_t size = static_cast<size_t>(sqlite3_column_bytes(m_stmt, From0To0(col)));
            
            const uint8_t* buffer = reinterpret_cast<const uint8_t*>(sqlite3_column_blob(m_stmt, From0To0(col)));

            return std::vector<uint8_t>(buffer, buffer + size);
        }

        [[noreturn]] void RaiseError(int code, std::string message);

        [[noreturn]] void RaiseError(std::string message);

        template <class T>
        const T& SaveConvertedValue(T val)
        {
            std::unique_ptr<Any> p_any;
            
            if (freeValues.empty())
            {
                p_any = MakeAny(val);
            }
            else
            {
                p_any.reset(freeValues.pop_back());

                p_any->val.emplace<T>(std::move(val));
            }

            usedValues.push_back(p_any.release());

            T* p_saved_val = std::any_cast<T>(&usedValues.back()->val);

            return *p_saved_val;
        }

    private:

        struct Any : public awl::quick_link
        {
            Any(std::any v) : val(v) {}
            
            std::any val;
        };

        using AnyList = awl::quick_list<Any>;

        template <class T>
        std::unique_ptr<Any> MakeAny(T val)
        {
            return std::make_unique<Any>(std::any(std::move(val)));
        }

        void ClearUsedValues()
        {
            freeValues.push_back(usedValues);

            assert(usedValues.empty());
        }

        void ClearFreeValues()
        {
            assert(usedValues.empty());

            while (!freeValues.empty())
            {
                delete freeValues.pop_back();
            }
        }

        void InternalExec(bool auto_reset)
        {
            const int rc = sqlite3_step(m_stmt);

            ClearUsedValues();

            if (rc != SQLITE_DONE)
            {
                if (auto_reset)
                {
                    //If the query failed sqlite3_reset also returns an error, but we ignore it.
                    sqlite3_reset(m_stmt);
                }

                RaiseError(rc, "Error while executing a statement.");
            }
            else
            {
                if (auto_reset)
                {
                    //sqlite3_reset should not return an error.
                    Reset();
                }
            }
        }

        void CheckBind(int rc)
        {
            if (rc != SQLITE_OK)
            {
                RaiseError(rc, "Bining error.");
            }
        }

        //The index is zero-based.
        int GetColumnType(int col) const
        {
            const int column_type = sqlite3_column_type(m_stmt, col);

            return column_type;
        }

        //Bind index is 1-based.
        static constexpr int From0To1(size_t col)
        {
            return static_cast<int>(col + 1);
        }

        //Get index is zero-based.
        static constexpr int From0To0(size_t col)
        {
            return static_cast<int>(col);
        }

        static constexpr size_t To0Col(int col)
        {
            assert(col > 0);
            return static_cast<size_t>(col - 1);
        }

        //const char * GetLastError() const
        //{
        //    assert(IsOpen());
        //    
        //    sqlite3 * db = sqlite3_db_handle(m_stmt);

        //    return sqlite3_errmsg(db);
        //}

        sqlite3_stmt * m_stmt = nullptr;

        AnyList usedValues;
        AnyList freeValues;
    };
}
