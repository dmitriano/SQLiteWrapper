#pragma once

#include "SQLiteWrapper/TableInstantiator.h"
#include "SQLiteWrapper/AutoincrementSet.h"

#include <memory>

namespace sqlite
{
    template <class Value, class Int> requires std::is_integral_v<Int>
    class AutoincrementTableInstantiator
    {
    public:

        AutoincrementTableInstantiator(const std::shared_ptr<Database>& db, std::string table_name, Int Value::* id_ptr,
            std::function<void(TableBuilder<Value>&)> add_constraints = {})
            :
            m_instantiator(db, std::move(table_name), std::make_tuple(id_ptr), add_constraints)
        {}

        void Create()
        {
            m_instantiator.Create();
        }

        using SetType = AutoincrementSet<Value, Int>;

        SetType MakeSet() const
        {
            return SetType(m_instantiator.m_db, m_instantiator.tableName, std::get<0>(m_instantiator.idPtrs));
        }

    private:

        TableInstantiator<Value, Int> m_instantiator;
    };
}
