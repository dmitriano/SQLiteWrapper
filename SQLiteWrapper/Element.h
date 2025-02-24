#pragma once

namespace sqlite
{
    class Element
    {
    public:

        virtual void Create() = 0;

        virtual void Open() = 0;

        virtual void Close() = 0;

        virtual void Delete() = 0;

        virtual ~Element() = default;
    };
}
