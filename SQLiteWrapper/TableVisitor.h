#pragma once

#include "SQLiteWrapper/TableBuilder.h"

namespace sqlite
{
    template <class Struct>
    class TableVisitor
    {
    public:

        // There are no additional constraints by default.
        void operator() (TableBuilder<Struct>&) {}
    };
}
