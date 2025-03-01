#pragma once

#include "SQLiteWrapper/Exception.h"
#include "SQLiteWrapper/Get.h"
#include "SQLiteWrapper/HeterogeneousIterator.h"

namespace sqlite
{
    inline void NextScalar(Statement& st)
    {
        if (!st.Next())
        {
            st.RaiseError("An empty recordset when a scalar is expected.");
        }
    }
    
    // A recordset with a single row and a value that is not null.
    // For retrieving the results of MIN, MAX, COUNT, etc...,
    template <typename T>
    void SelectScalar(Statement & st, T & val)
    {
        NextScalar(st);

        Get(st, 0, val);
    }

    // A recordset with a single row and a value that can be null.
    // MIN, MAX, COUNT can also be null when there are not records
    // satisfying where clause.
    // But there is still exactly one record in the recordset.
    template <typename T>
    void SelectOptionalScalar(Statement& st, std::optional<T>& opt)
    {
        NextScalar(st);

        if (!st.IsNull(0))
        {
            T val;
            
            Get(st, 0, val);

            opt = val;
        }
        else
        {
            opt = {};
        }
    }
}
