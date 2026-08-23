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
    inline void get(Statement & st, size_t col, bool & val)
    {
        val = st.intValue(col) != 0;
    }

    template <class T> requires (!std::is_same_v<T, bool>&& std::is_integral_v<T>&& std::is_signed_v<T> && sizeof(T) < sizeof(sqlite3_int64))
    void get(Statement& st, size_t col, T & val)
    {
        val = static_cast<T>(st.intValue(col));
    }

    template <class T> requires (!std::is_same_v<T, bool>&& std::is_integral_v<T>&& std::is_unsigned_v<T> && sizeof(T) < sizeof(sqlite3_int64))
    void get(Statement & st, size_t col, T & val)
    {
        std::make_signed_t<T> signedVal;

        get(st, col, signedVal);

        val = helpers::makeUnsigned(signedVal);
    }

    template <class T> requires (!std::is_same_v<T, bool>&& std::is_integral_v<T>&& std::is_signed_v<T> && sizeof(T) == sizeof(sqlite3_int64))
    void get(Statement& st, size_t col, T& val)
    {
        val = st.int64Value(col);
    }

    template <class T> requires (!std::is_same_v<T, bool>&& std::is_integral_v<T>&& std::is_unsigned_v<T> && sizeof(T) == sizeof(sqlite3_int64))
    void get(Statement& st, size_t col, T& val)
    {
        std::make_signed_t<uint64_t> signedVal;

        get(st, col, signedVal);

        val = helpers::makeUnsigned(signedVal);
    }

    inline void get(Statement & st, size_t col, double & val)
    {
        val = st.doubleValue(col);
    }

    template <typename UInt, uint8_t exp_len, template <typename, uint8_t> class DataTemplate>
    void get(Statement& st, size_t col, awl::decimal<UInt, exp_len, DataTemplate>& val)
    {
        using Decimal = awl::decimal<UInt, exp_len, DataTemplate>;
        using Rep = typename Decimal::Rep;

        const uint64_t int_val = static_cast<uint64_t>(st.int64Value(col));
        
        if constexpr (std::is_same_v<Rep, uint64_t>)
        {
            val = Decimal::from_bits(int_val);
        }
        else
        {
            val = Decimal::from_bits(Rep(int_val));
        }
    }

    inline void get(Statement & st, size_t col, std::string & val)
    {
        val = st.textValue(col);
    }

    inline void get(Statement & st, size_t col, const char * & val)
    {
        val = st.textValue(col);
    }

    template <class Rep, class Period>
    void get(Statement & st, size_t col, std::chrono::duration<Rep, Period> & val)
    {
        using namespace std::chrono;

        int64_t count;
        get(st, col, count);

        val = duration_cast<duration<Rep, Period>>(nanoseconds(count));
    }

    template <class Clock, class Duration>
    void get(Statement & st, size_t col, std::chrono::time_point<Clock, Duration> & val)
    {
        using namespace std::chrono;

        Duration d;
        get(st, col, d);

        val = time_point<Clock, Duration>(d);
    }

    template <class T> requires std::is_enum_v<T>
    void get(Statement& st, size_t col, T& val)
    {
        std::underlying_type_t<T> under_val;
        
        get(st, col, under_val);

        val = static_cast<T>(under_val);
    }

    inline void get(Statement& st, size_t col, std::vector<uint8_t>& val)
    {
        val = st.blobValue(col);
    }

    template <class T>
    void get(Statement& st, size_t col, std::optional<T>& opt_val)
    {
        if (st.isNull(col))
        {
            opt_val = {};
        }
        else
        {
            T val;
            
            get(st, col, val);

            opt_val = std::move(val);
        }
    }

    template <typename... Args>
    void get(Statement & st, size_t col, std::tuple<Args...> & t)
    {
        awl::for_each_index(t, [&st, col](auto & field, auto fieldIndex)
        {
            get(st, fieldIndex + col, field);
        });
    }

    template <class Struct> requires awl::tuplizable<Struct>
    void get(Statement & st, size_t col, Struct & val)
    {
        helpers::forEachFieldValue(val, [&st, col](auto& field, auto fieldIndex)
        {
            get(st, fieldIndex + col, field);
        });
    }
}
