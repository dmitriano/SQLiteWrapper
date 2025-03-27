#include "DbContainer.h"

#include "Tests/Experimental/MapStorage.h"

#include <vector>
#include <optional>

#include "Awl/IntRange.h"
#include "Awl/StdConsole.h"

using namespace swtest;

namespace
{
    struct Precision
    {
        uint8_t base;
        uint8_t quote;
        uint8_t amount;
        uint8_t price;

        AWL_REFLECT(base, quote, amount, price)
    };

    AWL_MEMBERWISE_EQUATABLE(Precision);

    struct MarketInfo
    {
        Precision precision;

        AWL_REFLECT(precision)
    };

    AWL_MEMBERWISE_EQUATABLE(MarketInfo)
}

AWL_TEST(MapStorage)
{
    DbContainer c(context);

    sqlite::MapStorage<std::string, MarketInfo> ms(c.db(), "market_info");
    ms.CreateTable();
    ms.Prepare();

    Precision precision_sample{ 1, 2, 3, 4 };
    Precision precision_result{ 5, 6, 7, 8 };

    MarketInfo mi_sample{ precision_sample };
    MarketInfo mi_result{ precision_result };

    const std::string id = "abc";
    const std::string wrong_id = "xyz";

    auto assert_does_not_exist = [&ms, &wrong_id]()
    {
        MarketInfo mi;

        AWL_ASSERT(!ms.Find(wrong_id, mi));
    };

    auto assert_exists = [&ms](const std::string& id, const MarketInfo& expected)
    {
        MarketInfo mi;

        AWL_ASSERT(ms.Find(id, mi));

        AWL_ASSERT(mi == expected);
    };

    assert_does_not_exist();

    ms.Insert(id, mi_sample);

    assert_does_not_exist();
    assert_exists(id, mi_sample);

    ms.Update(id, mi_result);

    assert_does_not_exist();
    assert_exists(id, mi_result);

    ms.Insert(wrong_id, mi_sample);
    assert_exists(wrong_id, mi_sample);
}