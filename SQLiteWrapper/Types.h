#pragma once

#include "sqlite3.h"

namespace sqlite
{
    using RowId = sqlite3_int64;

    constexpr RowId noId = static_cast<RowId>(-1);
}
