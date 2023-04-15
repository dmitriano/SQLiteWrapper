#pragma once

#include "Awl/Stringizable.h"

#include <variant>
#include <string>
#include <chrono>
#include <optional>
#include <array>
#include <iostream>

namespace
{
    using Clock = std::chrono::system_clock;

    struct SitVersion
    {
        std::array<wchar_t, 16> Type;
        uint16_t Version;
        uint64_t Reserverd;

        std::wstring GetType() const
        {
            return std::wstring(std::begin(Type), std::end(Type));
        }

        AWL_SERIALIZABLE(Type, Version, Reserverd)
    };

    AWL_MEMBERWISE_EQUATABLE(SitVersion)

    using SitId_t = std::chrono::time_point<Clock, std::chrono::seconds>;
    
    struct SitHeader
    {
        std::wstring ItemId;
        SitId_t SitId;

        AWL_SERIALIZABLE(ItemId, SitId)
    };
    
    using DateType = std::chrono::time_point<Clock, std::chrono::seconds>;

    struct TimestampType
    {
        std::chrono::time_point<Clock, std::chrono::nanoseconds> time;
        int tz_hour;
        int tz_minute;

        AWL_SERIALIZABLE(time, tz_hour, tz_minute);
    };

    AWL_MEMBERWISE_EQUATABLE(TimestampType)

    typedef int64_t NumberType;

    using FieldV = std::variant<
        std::wstring,
        std::optional<std::wstring>,
        NumberType,
        std::optional<NumberType>,
        DateType,
        std::optional<DateType>,
        TimestampType,
        std::optional<TimestampType>
    >;
}//DataCollection::OracleAuditing

#define OA_TABLE_FIELDS(...) \
    static const awl::helpers::MemberList & GetTableFieldNames() \
    { \
        static const wchar_t va[] = L#__VA_ARGS__; \
        static const awl::helpers::MemberList ml(va); \
        return ml; \
    }
