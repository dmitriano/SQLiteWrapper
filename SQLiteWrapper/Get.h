#pragma once

#include "SQLiteWrapper/Exception.h"
#include "SQLiteWrapper/Statement.h"
#include "SQLiteWrapper/Helpers.h"

#include "Awl/TupleHelpers.h"
#include "Awl/Decimal.h"

#include <stdint.h>
#include <type_traits>
#include <limits>
#include <chrono>
#include <optional>

namespace sqlite
{
    inline void Get(Statement & st, size_t col, bool & val)
    {
        val = st.GetInt(col) != 0;
    }

    template <class T> requires (!std::is_same_v<T, bool>&& std::is_integral_v<T>&& std::is_signed_v<T> && sizeof(T) < sizeof(sqlite3_int64))
    void Get(Statement& st, size_t col, T & val)
    {
        val = static_cast<T>(st.GetInt(col));
    }

    template <class T> requires (!std::is_same_v<T, bool>&& std::is_integral_v<T>&& std::is_unsigned_v<T> && sizeof(T) < sizeof(sqlite3_int64))
    void Get(Statement & st, size_t col, T & val)
    {
        std::make_signed_t<T> signedVal;

        Get(st, col, signedVal);

        val = helpers::MakeUnsigned(signedVal);
    }

    template <class T> requires (!std::is_same_v<T, bool>&& std::is_integral_v<T>&& std::is_signed_v<T> && sizeof(T) == sizeof(sqlite3_int64))
    void Get(Statement& st, size_t col, T& val)
    {
        val = st.GetInt64(col);
    }

    template <class T> requires (!std::is_same_v<T, bool>&& std::is_integral_v<T>&& std::is_unsigned_v<T> && sizeof(T) == sizeof(sqlite3_int64))
    void Get(Statement& st, size_t col, T& val)
    {
        std::make_signed_t<uint64_t> signedVal;

        Get(st, col, signedVal);

        val = helpers::MakeUnsigned(signedVal);
    }

    inline void Get(Statement & st, size_t col, double & val)
    {
        val = st.GetDouble(col);
    }

    template <typename UInt, uint8_t exp_len, template <typename, uint8_t> class DataTemplate>
    void Get(Statement& st, size_t col, awl::decimal<UInt, exp_len, DataTemplate>& val)
    {
        using Decimal = awl::decimal<UInt, exp_len, DataTemplate>;
        using Rep = typename Decimal::Rep;

        const uint64_t int_val = static_cast<uint64_t>(st.GetInt64(col));
        
        if constexpr (std::is_same_v<Rep, uint64_t>)
        {
            val = Decimal::from_bits(int_val);
        }
        else
        {
            val = Decimal::from_bits(Rep(int_val));
        }
    }

    inline void Get(Statement & st, size_t col, std::string & val)
    {
        val = st.GetText(col);
    }

    inline void Get(Statement & st, size_t col, const char * & val)
    {
        val = st.GetText(col);
    }

    template <class Clock, class Duration>
    void Get(Statement & st, size_t col, std::chrono::time_point<Clock, Duration> & val)
    {
        using namespace std::chrono;

        Duration d;
        Get(st, col, d);

        val = time_point<Clock, Duration>(d);
    }

    template <class Rep, class Period>
    void Get(Statement & st, size_t col, std::chrono::duration<Rep, Period> & val)
    {
        using namespace std::chrono;

        int64_t count;
        Get(st, col, count);

        val = duration_cast<duration<Rep, Period>>(nanoseconds(count));
    }

    template <class T> requires std::is_enum_v<T>
    void Get(Statement& st, size_t col, T& val)
    {
        std::underlying_type_t<T> under_val;
        
        Get(st, col, under_val);

        val = static_cast<T>(under_val);
    }

    template <typename... Args>
    void Get(Statement & st, size_t col, std::tuple<Args...> & t)
    {
        awl::for_each_index(t, [&st, col](auto & field, auto fieldIndex)
        {
            Get(st, fieldIndex + col, field);
        });
    }

    template <class Struct> requires awl::is_tuplizable_v<Struct>
    void Get(Statement & st, size_t col, Struct & val)
    {
        helpers::ForEachFieldValue(val, [&st, col](auto& field, auto fieldIndex)
        {
            Get(st, fieldIndex + col, field);
        });
    }

    inline void Get(Statement& st, size_t col, std::vector<uint8_t>& val)
    {
        val = st.GetBlob(col);
    }

    template <class T>
    void Get(Statement& st, size_t col, std::optional<T>& opt_val)
    {
        if (st.IsNull(col))
        {
            opt_val = {};
        }
        else
        {
            T val;
            
            Get(st, col, val);

            opt_val = std::move(val);
        }
    }
}
