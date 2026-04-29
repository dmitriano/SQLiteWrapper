#pragma once

#include "sqlite3.h"

#include <string>
#include <type_traits>

namespace sqlite
{
    using RowId = sqlite3_int64;

    constexpr RowId noId = static_cast<RowId>(-1);

    template <class T>
    struct is_text_type : std::is_same<T, std::string>
    {
    };

    template <class T>
    inline constexpr bool is_text_type_v = is_text_type<T>::value;
}
