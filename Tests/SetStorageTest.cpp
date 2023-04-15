#include "DbContainer.h"

#include "SQLiteWrapper/SetStorage.h"

#include "Awl/EnumTraits.h"

#include <vector>
#include <optional>

#include "Awl/IntRange.h"
#include "Awl/StdConsole.h"

using namespace swtest;

namespace
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

        AWL_STRINGIZABLE(min, max)
    };

    AWL_MEMBERWISE_EQUATABLE(Range);

    struct Limits
    {
        Range amount;
        Range price;
        Range cost;

        AWL_STRINGIZABLE(amount, price, cost)
    };

    AWL_MEMBERWISE_EQUATABLE(Limits);

    struct Precision
    {
        uint8_t base;
        uint8_t quote;
        uint8_t amount;
        uint8_t price;

        AWL_STRINGIZABLE(base, quote, amount, price)
    };

    AWL_MEMBERWISE_EQUATABLE(Precision);

    struct Market
    {
        Limits limits;
        
        std::string id;
        
        Precision precision;

        AWL_STRINGIZABLE(limits, id, precision)
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

        AWL_STRINGIZABLE(listId, clientId, marketId, side, type, status, id, price, stopPrice,
            amount, filled, cost, createTime, updateTime)
    };

    AWL_MEMBERWISE_EQUATABLE(Order)
}

AWT_TEST(FieldIndex)
{
    AWT_UNUSED_CONTEXT;

    AWT_ASSERT(sqlite::helpers::FindFieldIndex(&Market::id) == 1);

    AWT_ASSERT(sqlite::helpers::FindTransparentFieldIndex(&Market::id) == 6);
}

AWT_TEST(SetStorageMarket)
{
    DbContainer c(context);

    sqlite::SetStorage ms(c.m_db, "markets", std::make_tuple(&Market::id));
    ms.Create();
    ms.Prepare();

    Precision precision_sample{ 1, 2, 3, 4 };
    Precision precision_result{ 5, 6, 7, 8 };

    const std::string id = "abc";
    const std::string wrong_id = "xyz";

    Market m_sample{ {}, id, precision_sample };
    Market m_wrong_sample{ {}, wrong_id, precision_sample };
    Market m_result{ {}, id, precision_result };

    auto assert_does_not_exist = [&ms, &wrong_id]()
    {
        Market m;

        AWT_ASSERT(!ms.Find(wrong_id, m));
    };

    auto assert_exists = [&ms](const std::string& id, const Market& expected)
    {
        Market m;

        AWT_ASSERT(ms.Find(id, m));

        AWT_ASSERT(m == expected);
    };

    assert_does_not_exist();

    ms.Insert(m_sample);

    assert_does_not_exist();
    assert_exists(id, m_sample);

    ms.Update(m_result);

    assert_does_not_exist();
    assert_exists(id, m_result);

    ms.Insert(m_wrong_sample);
    assert_exists(wrong_id, m_wrong_sample);
}

namespace
{
    using KeyTuple = std::tuple<std::string, OrderId>;

    const std::string btc_market_id = "BTCUSDT";

    KeyTuple btc_key1(btc_market_id, 1);
    KeyTuple btc_key2(btc_market_id, 2);

    const Order btc_order1 =
    {
        btc_market_id,
        std::get<1>(btc_key1),
        -1,
        "9cd88f32-4025-44df-bc36-a0b502d6b62a",
        OrderSide::Buy,
        OrderType::Limit,
        OrderStatus::Open,
        "48456.08"_d,
        Decimal::zero(),
        "0.0015"_d,
        Decimal::zero(),
        "72.637515"_d,
        Decimal::zero(),

        Clock::now(),
        {}
    };

    const Order btc_order2 =
    {
        btc_market_id,
        std::get<1>(btc_key2),
        -1,
        "dcb1aa07-7448-4e8e-8f88-713dd4feb159",
        OrderSide::Sell,
        OrderType::StopLossLimit,
        OrderStatus::Closed,
        "45441.5"_d,
        Decimal::zero(),
        "0.0015"_d,
        Decimal::zero(),
        "68.14884"_d,
        Decimal::zero(),

        Clock::now(),
        {}
    };

    const std::string trx_market_id = "TRXUSDT";

    KeyTuple trx_key1(trx_market_id, 1);

    const Order trx_order1 =
    {
        trx_market_id,
        std::get<1>(trx_key1),
        -1,
        "3e294263-902e-4d02-b88a-85f01de62ba3",
        OrderSide::Buy,
        OrderType::Limit,
        OrderStatus::Closed,
        "0.1176"_d,
        Decimal::zero(),
        "500"_d,
        "500"_d,
        "58.98"_d,
        Decimal::zero(),

        Clock::now(),
        {}
    };
}

AWT_TEST(SetStorageOrder)
{
    DbContainer c(context);

    sqlite::SetStorage storage(c.m_db, "orders", std::make_tuple(&Order::marketId, &Order::id));

    storage.Create();
    storage.Prepare();

    {
        Order order;

        //SELECT listId, clientId, marketId, side, type, status, id, price, stopPrice, amount, filled, cost, createTime, updateTime FROM orders WHERE marketId=?3 AND id=?7;
        AWT_ASSERT(!storage.Find(btc_key1, order));
        AWT_ASSERT(!storage.Find(btc_key2, order));
    }

    storage.Insert(btc_order1);

    {
        Order order;

        AWT_ASSERT(storage.Find(btc_key1, order));
        AWT_ASSERT(order == btc_order1);

        AWT_ASSERT(!storage.Find(btc_key2, order));
    }

    AWT_ASSERT(!storage.TryInsert(btc_order1));

    AWT_ASSERT(storage.TryInsert(btc_order2));

    {
        Order order;

        AWT_ASSERT(storage.Find(btc_key1, order));
        AWT_ASSERT(order == btc_order1);

        AWT_ASSERT(storage.Find(btc_key2, order));
        AWT_ASSERT(order == btc_order2);
    }

    Order btc_order1_updated = btc_order1;

    btc_order1_updated.filled = "0.0015"_d;
    btc_order1_updated.updateTime = Clock::now();

    storage.Update(btc_order1_updated);

    {
        Order order =
        {
            btc_market_id,
            std::get<1>(btc_key1)
        };

        AWT_ASSERT(storage.Find(order));
        AWT_ASSERT(order == btc_order1_updated);
    }

    {
        Order order =
        {
            btc_market_id,
            std::get<1>(btc_key2)
        };

        AWT_ASSERT(storage.Find(order));
        AWT_ASSERT(order == btc_order2);
    }

    storage.Insert(trx_order1);

    {
        Order order;

        AWT_ASSERT(storage.Find(trx_key1, order));
        AWT_ASSERT(order == trx_order1);
    }

    try
    {
        storage.Insert(btc_order1);

        AWT_FAILM("It does not throw.");
    }
    catch (const sqlite::SQLiteException& e)
    {
        context.out << e.GetMessage() << std::endl;
    }

    try
    {
        btc_order1_updated.id = 3;
        
        storage.Update(btc_order1_updated);

        AWT_FAILM("It does not throw.");
    }
    catch (const sqlite::SQLiteException& e)
    {
        context.out << e.GetMessage() << std::endl;
    }

    Order btc_order1_found;
    Order btc_order2_found;

    AWT_ASSERT(storage.Find(btc_key1, btc_order1_found));

    AWT_ASSERT(!storage.Find(std::make_tuple(btc_market_id, 5), btc_order2_found));

    AWT_ASSERT(storage.Find(btc_key2, btc_order2_found));

    //Builds UPDATE orders SET status=?7, filled=?11 WHERE marketId=?1 AND id=?2;
    sqlite::Updater up = storage.CreateUpdater(std::make_tuple(&Order::status, &Order::filled));

    Order btc_order2_updated = btc_order2;

    btc_order2_updated.status = OrderStatus::Closed;
    btc_order2_updated.filled = "0.008"_d;

    up.Update(btc_order2_updated);

    Order btc_order2_updated_found;

    AWT_ASSERT(storage.Find(btc_key2, btc_order2_updated_found));

    AWT_ASSERT(btc_order2_updated_found == btc_order2_updated);

    storage.Delete(btc_key1);

    {
        Order order;

        AWT_ASSERT(!storage.Find(btc_key1, order));
        AWT_ASSERT(storage.Find(btc_key2, order));
    }

    storage.Delete(btc_order2);

    {
        Order order;

        AWT_ASSERT(!storage.Find(btc_key1, order));
        AWT_ASSERT(!storage.Find(btc_key2, order));
    }
}

namespace
{
    void CheckMax(sqlite::Database& db, const std::string& market_id, OrderId expected_max_id)
    {
        const char max_query[] = "SELECT MAX(id) FROM orders WHERE marketId=?;";

        {
            sqlite::Statement rs(db, max_query);

            sqlite::Bind(rs, 0, market_id);

            OrderId max_db_id = -1;

            if (!rs.Next())
            {
                rs.RaiseError("An empty recordset when a scalar is expected.");
            }

            if (!rs.IsNull(0))
            {
                sqlite::Get(rs, 0, max_db_id);
            }

            AWT_ASSERT_EQUAL(expected_max_id, max_db_id);
        }
    }
}

AWT_TEST(SetStorageMax)
{
    DbContainer c(context);

    sqlite::SetStorage storage(c.m_db, "orders", std::make_tuple(&Order::marketId, &Order::id));

    storage.Create();
    storage.Prepare();

    CheckMax(c.db(), btc_market_id, -1);

    storage.Insert(btc_order1);
    storage.Insert(btc_order2);
    storage.Insert(trx_order1);

    CheckMax(c.db(), btc_market_id, 2);

    CheckMax(c.db(), trx_market_id, 1);
}
