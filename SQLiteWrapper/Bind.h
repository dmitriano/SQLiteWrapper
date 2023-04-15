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

namespace sqlite
{
    inline void Bind(Statement & st, size_t col, bool val)
    {
        st.BindInt(col, val ? 1 : 0);
    }

    template <class T> requires (!std::is_same_v<T, bool> && std::is_integral_v<T> && std::is_signed_v<T> && sizeof(T) < sizeof(sqlite3_int64))
    void Bind(Statement& st, size_t col, T val)
    {
        st.BindInt(col, static_cast<int>(val));
    }

    template <class T> requires (!std::is_same_v<T, bool> && std::is_integral_v<T> && std::is_unsigned_v<T> && sizeof(T) < sizeof(sqlite3_int64))
    void Bind(Statement& st, size_t col, T val)
    {
        st.BindInt(col, static_cast<int>(helpers::MakeSigned(val)));
    }

    template <class T> requires (!std::is_same_v<T, bool> && std::is_integral_v<T> && std::is_signed_v<T> && sizeof(T) == sizeof(sqlite3_int64))
    void Bind(Statement& st, size_t col, T val)
    {
        st.BindInt64(col, val);
    }

    template <class T> requires (!std::is_same_v<T, bool> && std::is_integral_v<T> && std::is_unsigned_v<T> && sizeof(T) == sizeof(sqlite3_int64))
    void Bind(Statement& st, size_t col, T val)
    {
        st.BindInt64(col, helpers::MakeSigned(val));
    }

    inline void Bind(Statement & st, size_t col, double val)
    {
        st.BindDouble(col, val);
    }

    template <typename UInt, uint8_t exp_len, template <typename, uint8_t> class DataTemplate>
    void Bind(Statement& st, size_t col, const awl::decimal<UInt, exp_len, DataTemplate>& val)
    {
        using Decimal = awl::decimal<UInt, exp_len, DataTemplate>;
        using Rep = typename Decimal::Rep;

        if constexpr (std::is_same_v<Rep, uint64_t>)
        {
            const uint64_t int_val = val.to_bits();

            st.BindInt64(col, static_cast<int64_t>(int_val));
        }
        else
        {
            Rep rep = val.to_bits();
            
            const uint64_t int_val = static_cast<uint64_t>(rep);

            if (rep != Rep(int_val))
            {
                throw std::logic_error("Not supported decimal value.");
            }

            st.BindInt64(col, static_cast<int64_t>(int_val));
        }
    }

    inline void Bind(Statement & st, size_t col, const std::string & val)
    {
        st.BindText(col, val.c_str());
    }

    inline void Bind(Statement & st, size_t col, const char * val)
    {
        st.BindText(col, val);
    }

    //Check if nanoseconds representation is either long or long long and its size is 8, so it can be converted to int64_t.
    static_assert(std::is_arithmetic_v<std::chrono::nanoseconds::rep> && std::is_signed_v<std::chrono::nanoseconds::rep> && sizeof(std::chrono::nanoseconds::rep) == 8);

    template <class Rep, class Period>
    void Bind(Statement & st, size_t col, const std::chrono::duration<Rep, Period> & val)
    {
        using namespace std::chrono;

        const nanoseconds ns = duration_cast<nanoseconds>(val);

        const int64_t count = ns.count();

        st.BindInt64(col, count);
    }

    template <class Clock, class Duration>
    void Bind(Statement & st, size_t col, const std::chrono::time_point<Clock, Duration> & val)
    {
        Bind(st, col, val.time_since_epoch());
    }

    template <class T> requires std::is_enum_v<T>
    void Bind(Statement& st, size_t col, T val)
    {
        Bind(st, col, static_cast<std::underlying_type_t<T>>(val));
    }

    template <typename... Args>
    void Bind(Statement & st, size_t col, const std::tuple<Args...> & t)
    {
        awl::for_each_index(t, [&st, col](auto & field, auto fieldIndex)
        {
            Bind(st, fieldIndex + col, field);
        });
    }

    template <class Struct> requires awl::is_tuplizable_v<Struct>
    void Bind(Statement & st, size_t col, const Struct & val)
    {
        helpers::ForEachFieldValue(val, [&st, col](auto& field, auto fieldIndex)
        {
            Bind(st, fieldIndex + col, field);
        });
    }

    inline void Bind(Statement& st, size_t col, const std::vector<uint8_t>& val)
    {
        st.BindBlob(col, val);
    }
}
