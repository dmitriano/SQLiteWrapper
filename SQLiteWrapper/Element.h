#pragma once

namespace sqlite
{
    class Database;

    class Element
    {
    public:

        virtual void create(Database& db) = 0;

        virtual void drop(Database& db) = 0;

        virtual ~Element() = default;
    };
}
