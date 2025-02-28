#pragma once

#include "SQLiteWrapper/TableInstantiator.h"

#include <memory>

namespace sqlite
{
    template <class Value, class Int> requires std::is_integral_v<Int>
    class AutoincrementTableInstantiator
    {
    public:

        AutoincrementTableInstantiator(const std::shared_ptr<Database>& db, std::string table_name, Int Value::* id_ptr) :
            m_instantiator(db, std::move(table_name), std::make_tuple(id_ptr))
        {
        }

        void Create()
        {
            m_instantiator.Create();
        }

    private:

        TableInstantiator<Value, Int> m_instantiator;
    };
}
