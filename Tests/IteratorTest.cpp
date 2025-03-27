#include "DbContainer.h"
#include "ExchangeModel.h"
#include "Tests/TableHelper.h"

#include "SQLiteWrapper/Set.h"
#include "SQLiteWrapper/Iterator.h"

using namespace swtest;
using namespace exchange;
using namespace exchange::data;

AWL_TEST(IteratorEmptyRange)
{
    DbContainer c(context);

    auto storage = MakeSet(c.m_db, "orders", std::make_tuple(&v5::Order::exchangeId, &v5::Order::marketId, &v5::Order::accountType));

    {
        sqlite::Statement st(*c.m_db, "SELECT * FROM orders;");

        auto range = sqlite::make_range<v5::Order>(st);

        AWL_ASSERT(range.begin() == range.end());
    }
}
