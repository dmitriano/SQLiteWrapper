#pragma once

#include "SQLiteWrapper/TableBuilder.h"
#include "SQLiteWrapper/QueryBuilder.h"
#include "SQLiteWrapper/Statement.h"
#include "SQLiteWrapper/Bind.h"
#include "SQLiteWrapper/Get.h"

namespace sqlite
{
    template <class Struct>
    class Updater
    {
    public:

        Updater(Database& db, Statement s, IndexFilter id_indices, IndexFilter value_indices) :
            m_db(db), m_s(std::move(s)), m_idIndices(id_indices), m_valueIndices(value_indices)
        {
        }

        Updater(const Updater&) = delete;

        Updater& operator = (const Updater&) = delete;

        Updater(Updater&& other) = default;

        Updater& operator = (Updater&& other) = default;

        void Update(const Struct& val)
        {
            helpers::ForEachFieldValue(val, [this](auto& field, auto field_index)
            {
                if (m_idIndices.contains(field_index) || m_valueIndices.contains(field_index))
                {
                    Bind(m_s, field_index, field);
                }
            });

            Exec();
        }

        template <class... Ids, class... Values>
        void Update(std::tuple<Ids...> ids, std::tuple<Values...> values)
        {
            if (std::tuple_size_v<std::tuple<Ids...>> != m_idIndices.size())
            {
                throw std::runtime_error("Wrong number of keys.");
            }
            
            if (std::tuple_size_v<std::tuple<Values...>> != m_valueIndices.size())
            {
                throw std::runtime_error("Wrong number of values.");
            }

            //Bind IDs
            {
                auto i = m_idIndices.begin();

                awl::for_each(ids, [this, &i](auto& field)
                {
                    const size_t field_index = *i++;

                    Bind(m_s, field_index, field);
                });
            }

            //Bind Values
            {
                auto i = m_valueIndices.begin();

                awl::for_each(values, [this, &i](auto& field)
                {
                    const size_t field_index = *i++;

                    Bind(m_s, field_index, field);
                });
            }

            Exec();
        }

    private:

        void Exec()
        {
            m_s.Exec();

            m_db.EnsureAffected(1);
        }

        Database& m_db;

        Statement m_s;

        const IndexFilter m_idIndices;

        const IndexFilter m_valueIndices;
    };
}
