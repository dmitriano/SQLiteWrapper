#include "DbContainer.h"
#include "ExchangeModel.h"
#include "SQLiteWrapper/AutoincrementTableInstantiator.h"
#include "SQLiteWrapper/IndexInstantiator.h"

using namespace swtest;
using namespace exchange::data;

AWL_TEST(InstantiatorIndex)
{
    DbContainer c(context);

    const std::string table_name = "orders";
    const std::string index_name = "orders_index";

    sqlite::AutoincrementTableInstantiator table_instantiator(c.m_db, table_name, &v4::Order::clientId);

    table_instantiator.Create();

    auto key_ptrs = std::make_tuple(&v4::Order::exchangeId, &v4::Order::marketId, &v4::Order::accountType);

    sqlite::IndexInstantiator index_instantiator(c.m_db, table_name, index_name, key_ptrs);

    index_instantiator.Create();

    auto order_set = index_instantiator.MakeSet();
}

namespace
{
    struct OrderList
    {
        int64_t id;

        int64_t ownerId;

        std::string ownerType;

        AWL_REFLECT(id, ownerId, ownerType)
    };

    struct OrderLink
    {
        int64_t listId;

        int64_t orderId;

        AWL_REFLECT(listId, orderId)
    };
}

AWL_TEST(InstantiatorConstraints)
{
    DbContainer c(context);

    sqlite::AutoincrementTableInstantiator orders_instantiator(c.m_db, "orders", &v4::Order::clientId);

    orders_instantiator.Create();

    sqlite::AutoincrementTableInstantiator lists_instantiator(c.m_db, "order_lists", &OrderList::id);

    lists_instantiator.Create();

    std::function<void(sqlite::TableBuilder<OrderLink>&)> add_constraints = [](sqlite::TableBuilder<OrderLink>& builder)
        {
            builder.SetColumnConstraint(&OrderLink::listId, "NOT NULL REFERENCES order_lists(id)");
            builder.SetColumnConstraint(&OrderLink::orderId, "NOT NULL REFERENCES orders(clientId)");
        };

    sqlite::TableInstantiator links_instantiator(c.m_db, "order_links", std::make_tuple(&OrderLink::listId, &OrderLink::orderId), add_constraints);

    links_instantiator.Create();

    auto links_set = links_instantiator.MakeSet();
}
