#pragma once

namespace sqlite
{
    class Element
    {
    public:

        virtual void Create() = 0;

        virtual void Prepare() = 0;

        virtual ~Element() = default;
    };
}
