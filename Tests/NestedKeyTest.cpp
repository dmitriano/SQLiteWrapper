#include "SQLiteWrapper/Helpers.h"

#include "Awl/Testing/UnitTest.h"

#include <cstdint>
#include <string>
namespace
{
    struct NestedKey
    {
        std::string exchangeId;
        std::string marketId;
        int64_t orderId;

        AWL_REFLECT(exchangeId, marketId, orderId)
    };

    struct NestedOrder
    {
        int64_t clientId;
        NestedKey id;
        int status;

        AWL_REFLECT(clientId, id, status)
    };
}

AWL_TEST(NestedKey)
{
    AWL_UNUSED_CONTEXT;

    sqlite::IndexFilter indices = sqlite::helpers::findTransparentFieldIndices(std::make_tuple(&NestedOrder::id));

    AWL_ASSERT_EQUAL(3u, indices.size());
    AWL_ASSERT(indices.contains(1));
    AWL_ASSERT(indices.contains(2));
    AWL_ASSERT(indices.contains(3));
}
