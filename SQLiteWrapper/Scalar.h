#pragma once

#include "SQLiteWrapper/Exception.h"
#include "SQLiteWrapper/Get.h"
#include "SQLiteWrapper/HeterogeneousIterator.h"

namespace sqlite
{
    // Returns an iterator pointing to the first element.
    inline HeterogeneousIterator NextScalar(Statement& st)
    {
        // It restes the statement in its destructor.
        HeterogeneousIterator i(st);

        if (!i.Next())
        {
            st.RaiseError("An empty recordset when a scalar is expected.");
        }

        return i;
    }
    
    // A recordset with a single row and a value that is not null.
    // For retrieving the results of MIN, MAX, COUNT, etc...,
    template <typename T>
    void SelectScalar(Statement& st, T& val)
    {
        HeterogeneousIterator i = NextScalar(st);

        i.Get(val);
    }

    // A recordset with a single row and a value that can be null.
    // MIN, MAX, COUNT can also be null when there are not records
    // satisfying where clause.
    // But there is still exactly one record in the recordset.
    template <typename T>
    void SelectOptionalScalar(Statement& st, std::optional<T>& opt)
    {
        HeterogeneousIterator i = NextScalar(st);

        if (!st.IsNull(0))
        {
            T val;
            
            i.Get(val);

            opt = val;
        }
        else
        {
            opt = {};
        }
    }

    // One or zero rows in the recordset.
    // It can be a result of a query like "SELECT * FROM t1 WHERE t1.id=5;"
    // where t1.id is a unique key.
    template <typename T>
    void SelectOptionalRecord(Statement& st, std::optional<T>& opt)
    {
        HeterogeneousIterator i(st);

        if (i.Next())
        {
            T val;

            i.Get(val);

            opt = val;

            if (i.Next())
            {
                st.RaiseError("One or zero rows expected.");
            }
        }
        else
        {
            opt = {};
        }
    }
}
