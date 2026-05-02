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

AWL_TEST(NestedFieldPath)
{
    AWL_UNUSED_CONTEXT;

    const size_t index = sqlite::helpers::findTransparentFieldIndex(
        sqlite::helpers::fieldPath(&NestedOrder::id, &NestedKey::orderId));

    AWL_ASSERT_EQUAL(3u, index);
}

AWL_TEST(NestedFieldPathSet)
{
    AWL_UNUSED_CONTEXT;

    sqlite::IndexFilter indices = sqlite::helpers::findTransparentFieldIndices(
        sqlite::helpers::fieldPaths(
            sqlite::helpers::fieldPath(&NestedOrder::clientId),
            sqlite::helpers::fieldPath(&NestedOrder::id, &NestedKey::orderId),
            sqlite::helpers::fieldPath(&NestedOrder::status)));

    AWL_ASSERT_EQUAL(3u, indices.size());
    AWL_ASSERT(indices.contains(0));
    AWL_ASSERT(indices.contains(3));
    AWL_ASSERT(indices.contains(4));
}

AWL_TEST(NestedFieldPathSubtree)
{
    AWL_UNUSED_CONTEXT;

    sqlite::IndexFilter indices = sqlite::helpers::findTransparentFieldIndices(
        sqlite::helpers::fieldPath(&NestedOrder::id));

    AWL_ASSERT_EQUAL(3u, indices.size());
    AWL_ASSERT(indices.contains(1));
    AWL_ASSERT(indices.contains(2));
    AWL_ASSERT(indices.contains(3));
}
