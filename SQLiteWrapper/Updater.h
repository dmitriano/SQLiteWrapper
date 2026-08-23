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
            _db(db), _s(std::move(s)), _idIndices(id_indices), _valueIndices(value_indices)
        {}

        Updater(const Updater&) = delete;

        Updater& operator = (const Updater&) = delete;

        Updater(Updater&& other) = default;

        Updater& operator = (Updater&& other) = default;

        void update(const Struct& val)
        {
            helpers::forEachFieldValue(val, [this](auto& field, auto field_index)
            {
                if (_idIndices.contains(field_index) || _valueIndices.contains(field_index))
                {
                    sqlite::bind(_s, field_index, field);
                }
            });

            exec();
        }

        template <class... Ids, class... Values>
        void update(std::tuple<Ids...> ids, std::tuple<Values...> values)
        {
            if (std::tuple_size_v<std::tuple<Ids...>> != _idIndices.size())
            {
                throw std::runtime_error("Wrong number of keys.");
            }
            
            if (std::tuple_size_v<std::tuple<Values...>> != _valueIndices.size())
            {
                throw std::runtime_error("Wrong number of values.");
            }

            //Bind IDs
            {
                auto i = _idIndices.begin();

                awl::for_each(ids, [this, &i](auto& field)
                {
                    const size_t field_index = *i++;

                    sqlite::bind(_s, field_index, field);
                });
            }

            //Bind Values
            {
                auto i = _valueIndices.begin();

                awl::for_each(values, [this, &i](auto& field)
                {
                    const size_t field_index = *i++;

                    sqlite::bind(_s, field_index, field);
                });
            }

            exec();
        }

    private:

        void exec()
        {
            _s.exec();

            _db.ensureAffected(1);
        }

        Database& _db;

        Statement _s;

        const IndexFilter _idIndices;

        const IndexFilter _valueIndices;
    };
}
