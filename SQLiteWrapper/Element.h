#pragma once

namespace sqlite
{
    class Element
    {
    public:

        virtual void create() = 0;

        virtual void deleteElement() = 0;

        virtual ~Element() = default;
    };
}
