#include "DbContainer.h"
#include "ExchangeModel.h"
#include "SQLiteWrapper/AutoincrementTableInstantiator.h"
#include "SQLiteWrapper/IndexInstantiator.h"
#include "SQLiteWrapper/Set.h"

using namespace swtest;
using namespace exchange::data;

AWL_TEST(Index)
{
    DbContainer c(context);

    const std::string table_name = "orders";
    const std::string index_name = "orders_index";

    sqlite::AutoincrementTableInstantiator table_instantiator(c.m_db, table_name, &v4::Order::clientId);

    table_instantiator.Create();

    auto key_ptrs = std::make_tuple(&v4::Order::exchangeId, &v4::Order::marketId, &v4::Order::accountType);

    sqlite::IndexInstantiator index_instantiator(c.m_db, table_name, index_name, key_ptrs);

    index_instantiator.Create();
}
