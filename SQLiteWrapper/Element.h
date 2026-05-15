#pragma once

#include <functional>

namespace sqlite
{
    class Database;

    using DatabaseRef = std::reference_wrapper<Database>;

    class Element
    {
    public:

        virtual void create(DatabaseRef db) = 0;

        virtual void drop(DatabaseRef db) = 0;

        virtual ~Element() = default;
    };
}
