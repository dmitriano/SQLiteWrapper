#include "Awl/EnumTraits.h"
#include "Awl/Decimal.h"

#include <vector>
#include <optional>
#include <chrono>
#include <optional>

namespace exchange::data
{
    using Decimal = awl::decimal<uint64_t, 4>;
    
    constexpr Decimal operator"" _d(const char* str, std::size_t len)
    {
        return Decimal(std::string_view(str, len));
    }

    constexpr Decimal operator"" _d(const wchar_t* str, std::size_t len)
    {
        return Decimal(std::wstring_view(str, len));
    }

    struct Range
    {
        int min;
        int max;

        AWL_REFLECT(min, max)
    };

    AWL_MEMBERWISE_EQUATABLE(Range);

    struct Limits
    {
        Range amount;
        Range price;
        Range cost;

        AWL_REFLECT(amount, price, cost)
    };

    AWL_MEMBERWISE_EQUATABLE(Limits);

    struct Precision
    {
        uint8_t base;
        uint8_t quote;
        uint8_t amount;
        uint8_t price;

        AWL_REFLECT(base, quote, amount, price)
    };

    AWL_MEMBERWISE_EQUATABLE(Precision);

    struct Market
    {
        Limits limits;
        
        std::string id;
        
        Precision precision;

        AWL_REFLECT(limits, id, precision)
    };

    AWL_MEMBERWISE_EQUATABLE(Market)

    using Clock = std::chrono::system_clock;
    using TimePoint = std::chrono::time_point<Clock>;
    using Duration = typename Clock::duration;

    using OrderId = int64_t;
    
    AWL_SEQUENTIAL_ENUM(OrderSide, Buy, Sell)

    AWL_SEQUENTIAL_ENUM(OrderStatus,
        Pending,
        Open,
        Closed
    )

    AWL_SEQUENTIAL_ENUM(OrderType,
        Limit,
        Market,
        StopLoss,
        StopLossLimit,
        TakeProfit,
        TakeProfitLimit,
        LimitMaker
    )
        
    struct Order
    {
        std::string marketId;
        OrderId id;
        int64_t listId;
        std::string clientId;

        OrderSide side;
        OrderType type;
        OrderStatus status;

        Decimal price;
        Decimal stopPrice;
        Decimal amount;
        Decimal filled;
        Decimal cost;
        Decimal reserved;

        TimePoint createTime;
        TimePoint updateTime;

        AWL_REFLECT(listId, clientId, marketId, side, type, status, id, price, stopPrice,
            amount, filled, cost, createTime, updateTime)
    };

    AWL_MEMBERWISE_EQUATABLE(Order)

    namespace v2
    {
        AWL_SEQUENTIAL_ENUM(AccountType, Spot, CrossMargin, IsolatedMargin)

        struct Order
        {
            std::string exchangeId;
            std::string marketId;
            OrderId id;
            int64_t listId;
            std::string clientId;

            AccountType accountType;

            OrderSide side;
            OrderType type;
            OrderStatus status;

            Decimal price;
            Decimal stopPrice;
            Decimal amount;
            Decimal filled;
            Decimal cost;
            Decimal reserved;

            TimePoint createTime;
            TimePoint updateTime;

            AWL_REFLECT(exchangeId, marketId, id, accountType, listId, clientId, side, type, status, price, stopPrice,
                amount, filled, cost, createTime, updateTime)
        };

        AWL_MEMBERWISE_EQUATABLE(Order)
    }

    namespace v3
    {
        using AccountType = int;

        constexpr AccountType Spot = 0;

        struct Order
        {
            std::string exchangeId;
            std::string marketId;
            OrderId id;
            int64_t listId;
            std::string clientId;

            int accountType;

            OrderSide side;
            OrderType type;
            OrderStatus status;

            Decimal price;
            Decimal stopPrice;
            Decimal amount;
            Decimal filled;
            Decimal cost;
            Decimal reserved;

            TimePoint createTime;
            TimePoint updateTime;

            AWL_REFLECT(exchangeId, marketId, id, accountType, listId, clientId, side, type, status, price, stopPrice,
                amount, filled, cost, createTime, updateTime)
        };

        AWL_MEMBERWISE_EQUATABLE(Order)
    }

    namespace v4
    {
        AWL_SEQUENTIAL_ENUM(AccountType, Spot, CrossMargin, IsolatedMargin)
            
        struct Order
        {
            int64_t clientId;

            std::string exchangeId;
            std::string marketId;
            OrderId id;
            int64_t listId;
            std::string clientGuid;

            AccountType accountType;

            OrderSide side;
            OrderType type;
            OrderStatus status;

            Decimal price;
            Decimal stopPrice;
            Decimal amount;
            Decimal filled;
            Decimal cost;
            Decimal reserved;

            TimePoint createTime;
            TimePoint updateTime;

            AWL_REFLECT(clientId, exchangeId, marketId, id, accountType, listId, clientGuid, side, type, status, price, stopPrice,
                amount, filled, cost, createTime, updateTime)
        };
    }

    namespace v5
    {
        AWL_SEQUENTIAL_ENUM(AccountType, Spot, CrossMargin, IsolatedMargin)

        struct Order
        {
            int64_t clientId;
            std::optional<int64_t> clientListId;

            std::string exchangeId;
            std::string marketId;
            OrderId id;
            int64_t listId;
            std::string clientGuid;

            AccountType accountType;

            OrderSide side;
            OrderType type;
            OrderStatus status;

            Decimal price;
            Decimal stopPrice;
            Decimal amount;
            Decimal filled;
            Decimal cost;
            Decimal reserved;

            TimePoint createTime;
            TimePoint updateTime;

            AWL_REFLECT(clientId, clientListId, exchangeId, marketId, id, accountType, listId, clientGuid, side, type, status, price, stopPrice,
                amount, filled, cost, createTime, updateTime)
        };
    }
}
