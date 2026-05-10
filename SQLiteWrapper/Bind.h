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
    inline void bind(Statement & st, size_t col, bool val)
    {
        st.bindInt(col, val ? 1 : 0);
    }

    template <class T> requires (!std::is_same_v<T, bool> && std::is_integral_v<T> && std::is_signed_v<T> && sizeof(T) < sizeof(sqlite3_int64))
    void bind(Statement& st, size_t col, T val)
    {
        st.bindInt(col, static_cast<int>(val));
    }

    template <class T> requires (!std::is_same_v<T, bool> && std::is_integral_v<T> && std::is_unsigned_v<T> && sizeof(T) < sizeof(sqlite3_int64))
    void bind(Statement& st, size_t col, T val)
    {
        st.bindInt(col, static_cast<int>(helpers::makeSigned(val)));
    }

    template <class T> requires (!std::is_same_v<T, bool> && std::is_integral_v<T> && std::is_signed_v<T> && sizeof(T) == sizeof(sqlite3_int64))
    void bind(Statement& st, size_t col, T val)
    {
        st.bindInt64(col, val);
    }

    template <class T> requires (!std::is_same_v<T, bool> && std::is_integral_v<T> && std::is_unsigned_v<T> && sizeof(T) == sizeof(sqlite3_int64))
    void bind(Statement& st, size_t col, T val)
    {
        st.bindInt64(col, helpers::makeSigned(val));
    }

    inline void bind(Statement & st, size_t col, double val)
    {
        st.bindDouble(col, val);
    }

    template <typename UInt, uint8_t exp_len, template <typename, uint8_t> class DataTemplate>
    void bind(Statement& st, size_t col, const awl::decimal<UInt, exp_len, DataTemplate>& val)
    {
        using Decimal = awl::decimal<UInt, exp_len, DataTemplate>;
        using Rep = typename Decimal::Rep;

        if constexpr (std::is_same_v<Rep, uint64_t>)
        {
            const uint64_t int_val = val.to_bits();

            st.bindInt64(col, static_cast<int64_t>(int_val));
        }
        else
        {
            Rep rep = val.to_bits();
            
            const uint64_t int_val = static_cast<uint64_t>(rep);

            if (rep != Rep(int_val))
            {
                throw std::logic_error("Not supported decimal value.");
            }

            st.bindInt64(col, static_cast<int64_t>(int_val));
        }
    }

    inline void bind(Statement & st, size_t col, const std::string & val)
    {
        st.bindText(col, val.c_str());
    }

    inline void bind(Statement & st, size_t col, const char * val)
    {
        st.bindText(col, val);
    }

    //Check if nanoseconds representation is either long or long long and its size is 8, so it can be converted to int64_t.
    static_assert(std::is_arithmetic_v<std::chrono::nanoseconds::rep> && std::is_signed_v<std::chrono::nanoseconds::rep> && sizeof(std::chrono::nanoseconds::rep) == 8);

    template <class Rep, class Period>
    void bind(Statement & st, size_t col, const std::chrono::duration<Rep, Period> & val)
    {
        using namespace std::chrono;

        const nanoseconds ns = duration_cast<nanoseconds>(val);

        const int64_t count = ns.count();

        st.bindInt64(col, count);
    }

    template <class Clock, class Duration>
    void bind(Statement & st, size_t col, const std::chrono::time_point<Clock, Duration> & val)
    {
        using namespace std::chrono;

        const nanoseconds ns = duration_cast<nanoseconds>(val.time_since_epoch());

        st.bindInt64(col, ns.count());
    }

    template <class T> requires std::is_enum_v<T>
    void bind(Statement& st, size_t col, T val)
    {
        bind(st, col, static_cast<std::underlying_type_t<T>>(val));
    }

    inline void bind(Statement& st, size_t col, const std::vector<uint8_t>& val)
    {
        st.bindBlob(col, val);
    }

    template <class T>
    void bind(Statement& st, size_t col, const std::optional<T>& opt_val)
    {
        if (opt_val)
        {
            bind(st, col, *opt_val);
        }
        else
        {
            st.bindNull(col);
        }
    }

    template <typename... Args>
    void bind(Statement & st, size_t col, const std::tuple<Args...> & t)
    {
        awl::for_each_index(t, [&st, col](auto & field, auto fieldIndex)
        {
            bind(st, fieldIndex + col, field);
        });
    }

    template <class Struct> requires awl::is_tuplizable_v<Struct>
    void bind(Statement & st, size_t col, const Struct & val)
    {
        helpers::forEachFieldValue(val, [&st, col](auto& field, auto fieldIndex)
        {
            bind(st, fieldIndex + col, field);
        });
    }
}
