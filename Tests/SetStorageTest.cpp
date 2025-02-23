#include "DbContainer.h"
#include "ExchangeModel.h"

#include "SQLiteWrapper/SetStorage.h"


#include "Awl/IntRange.h"
#include "Awl/StdConsole.h"

#include <vector>

using namespace swtest;
using namespace exchange;
using namespace exchange::data;
using awl::testing::Assert;

AWL_TEST(FieldIndex)
{
    AWL_UNUSED_CONTEXT;

    AWL_ASSERT(sqlite::helpers::FindFieldIndex(&Market::id) == 1);

    AWL_ASSERT(sqlite::helpers::FindTransparentFieldIndex(&Market::id) == 6);
}

AWL_TEST(SetStorageMarket)
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

        AWL_ASSERT(!ms.Find(wrong_id, m));
    };

    auto assert_exists = [&ms](const std::string& id, const Market& expected)
    {
        Market m;

        AWL_ASSERT(ms.Find(id, m));

        AWL_ASSERT(m == expected);
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

AWL_TEST(SetStorageOrder)
{
    DbContainer c(context);

    sqlite::SetStorage storage(c.m_db, "orders", std::make_tuple(&Order::marketId, &Order::id));

    storage.Create();
    storage.Prepare();

    {
        Order order;

        //SELECT listId, clientId, marketId, side, type, status, id, price, stopPrice, amount, filled, cost, createTime, updateTime FROM orders WHERE marketId=?3 AND id=?7;
        AWL_ASSERT(!storage.Find(btc_key1, order));
        AWL_ASSERT(!storage.Find(btc_key2, order));
    }

    storage.Insert(btc_order1);

    {
        Order order;

        AWL_ASSERT(storage.Find(btc_key1, order));
        AWL_ASSERT(order == btc_order1);

        AWL_ASSERT(!storage.Find(btc_key2, order));
    }

    AWL_ASSERT(!storage.TryInsert(btc_order1));

    AWL_ASSERT(storage.TryInsert(btc_order2));

    {
        Order order;

        AWL_ASSERT(storage.Find(btc_key1, order));
        AWL_ASSERT(order == btc_order1);

        AWL_ASSERT(storage.Find(btc_key2, order));
        AWL_ASSERT(order == btc_order2);
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

        AWL_ASSERT(storage.Find(order));
        AWL_ASSERT(order == btc_order1_updated);
    }

    {
        Order order =
        {
            btc_market_id,
            std::get<1>(btc_key2)
        };

        AWL_ASSERT(storage.Find(order));
        AWL_ASSERT(order == btc_order2);
    }

    storage.Insert(trx_order1);

    {
        Order order;

        AWL_ASSERT(storage.Find(trx_key1, order));
        AWL_ASSERT(order == trx_order1);
    }

    try
    {
        storage.Insert(btc_order1);

        AWL_FAILM("It does not throw.");
    }
    catch (const sqlite::SQLiteException& e)
    {
        context.out << e.What() << std::endl;
    }

    try
    {
        btc_order1_updated.id = 3;
        
        storage.Update(btc_order1_updated);

        AWL_FAILM("It does not throw.");
    }
    catch (const sqlite::SQLiteException& e)
    {
        context.out << e.What() << std::endl;
    }

    Order btc_order1_found;
    Order btc_order2_found;

    AWL_ASSERT(storage.Find(btc_key1, btc_order1_found));

    AWL_ASSERT(!storage.Find(std::make_tuple(btc_market_id, 5), btc_order2_found));

    AWL_ASSERT(storage.Find(btc_key2, btc_order2_found));

    //Builds UPDATE orders SET status=?7, filled=?11 WHERE marketId=?1 AND id=?2;
    sqlite::Updater up = storage.CreateUpdater(std::make_tuple(&Order::status, &Order::filled));

    Order btc_order2_updated = btc_order2;

    btc_order2_updated.status = OrderStatus::Closed;
    btc_order2_updated.filled = "0.008"_d;

    up.Update(btc_order2_updated);

    Order btc_order2_updated_found;

    AWL_ASSERT(storage.Find(btc_key2, btc_order2_updated_found));

    AWL_ASSERT(btc_order2_updated_found == btc_order2_updated);

    storage.Delete(btc_key1);

    {
        Order order;

        AWL_ASSERT(!storage.Find(btc_key1, order));
        AWL_ASSERT(storage.Find(btc_key2, order));
    }

    storage.Delete(btc_order2);

    {
        Order order;

        AWL_ASSERT(!storage.Find(btc_key1, order));
        AWL_ASSERT(!storage.Find(btc_key2, order));
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

            AWL_ASSERT_EQUAL(expected_max_id, max_db_id);
        }
    }
}

AWL_TEST(SetStorageMax)
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

AWL_TEST(OrderStorageGetBind)
{
    DbContainer c(context);

    sqlite::SetStorage storage(c.m_db, "orders", std::make_tuple(&data::Order2::accountType, &data::Order2::marketId, &data::Order2::id));

    storage.Create();
    storage.Prepare();

    using TestOrderKey = std::tuple<AccountType2, std::string, data::OrderId>;

    TestOrderKey btc_key(data::AccountType2::Spot, btc_market_id, 15);

    {
        auto val = static_cast<std::underlying_type_t<data::AccountType2>>(std::get<0>(btc_key));

        auto signed_val = sqlite::helpers::MakeSigned(val);

        AWL_ASSERT_EQUAL(0u, sqlite::helpers::MakeUnsigned(signed_val));
    }

    const data::Order2 sample_order =
    {
        "binance",
        btc_market_id,
        std::get<2>(btc_key),
        -1,
        "dcb1aa07-7448-4e8e-8f88-713dd4feb159",

        data::AccountType2::Spot,

        data::OrderSide::Sell,
        data::OrderType::StopLossLimit,
        data::OrderStatus::Closed,
        "45441.5"_d,
        data::Decimal::zero(),
        "0.0015"_d,
        data::Decimal::zero(),
        "68.14884"_d,
        data::Decimal::zero(),

        data::Clock::now(),
        {}
    };

    const std::vector<data::Order2> sample_v{ sample_order };

    context.logger.debug(awl::format() << "Inserting order: " << sample_order.id);

    storage.Insert(sample_order);

    const auto count = std::ranges::distance(storage);

    AWL_ASSERT_EQUAL(1u, count);

    AWL_ASSERT(std::ranges::equal(storage, sample_v));

    context.logger.debug(awl::format() << count << " orders:");

    for (const data::Order2& order : storage)
    {
        context.logger.debug(awl::format() << "Order: " << order.id);
    }

    data::Order2 found_order;

    AWL_ASSERT(storage.Find(btc_key, found_order));

    context.logger.debug(awl::format() << "Loaded order: " << awl::format::endl << found_order.id);

    AWL_ASSERT(found_order == sample_order);

    storage.Delete(btc_key);
}
