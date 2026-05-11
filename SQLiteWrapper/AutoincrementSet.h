#pragma once

#include "SQLiteWrapper/Set.h"

#include <type_traits>

namespace sqlite
{
    template <class Value, class Int> requires std::is_integral_v<Int>
    class AutoincrementSet
    {
    public:

        AutoincrementSet(const std::shared_ptr<Database>& db, std::string table_name, Int Value::* id_ptr) :
            _storage(db, std::move(table_name), std::make_tuple(id_ptr))
        {
            insertWithoutIdStatement = _storage.makeStatement("autoinsert", buildParameterizedInsertQuery<Value>(_storage.tableName, _storage.valueFilter()));
        }

        AutoincrementSet(const AutoincrementSet&) = delete;
        AutoincrementSet(AutoincrementSet&&) = default;

        AutoincrementSet& operator = (const AutoincrementSet&) = delete;
        AutoincrementSet& operator = (AutoincrementSet&&) = default;

        void close()
        {
            _storage.close();

            insertWithoutIdStatement.close();
        }

        Iterator<Value> begin()
        {
            return _storage.begin();
        }

        IteratorSentinel<Value> end()
        {
            return _storage.end();
        }

        void insert(Value& val)
        {
            _storage.bindValue(insertWithoutIdStatement, val, _storage.valueFilter());

            insertWithoutIdStatement.exec();

            assignLastRowId(val);
        }

        // It may still violate some constraint like UNIQUE index on other columns.
        bool tryinsert(Value& val)
        {
            _storage.bindValue(insertWithoutIdStatement, val, _storage.valueFilter());

            const bool success = insertWithoutIdStatement.exec();

            if (success)
            {
                assignLastRowId(val);
            }

            return success;
        }

        void insertWithId(const Value& val)
        {
            _storage.insert(val);
        }

        bool tryinsertWithId(const Value& val)
        {
            return _storage.tryinsert(val);
        }

        bool find(Value& val)
        {
            return _storage.find(val);
        }

        bool find(Int id, Value& val)
        {
            return _storage.find(std::make_tuple(id), val);
        }

        void update(const Value& val)
        {
            _storage.update(val);
        }

        template <class... Field>
        Updater<Value> createUpdater(std::tuple<Field Value::*...> field_ptrs) const
        {
            return _storage.createUpdater(field_ptrs);
        }

        void tryDeleteRecord(Int id)
        {
            _storage.tryDeleteRecord(std::make_tuple(id));
        }

        void deleteElement(Int id)
        {
            _storage.deleteElement(std::make_tuple(id));
        }

        void tryDeleteRecord(const Value& val)
        {
            _storage.tryDeleteRecord(val);
        }

        void deleteElement(const Value& val)
        {
            _storage.deleteElement(val);
        }

        void clear()
        {
            _storage.clear();
        }

    private:

        void assignLastRowId(Value& val) const
        {
            Int Value::* id_ptr = std::get<0>(_storage.idPtrs);

            // TODO: What about signed/unsigned?
            val.*id_ptr = static_cast<Int>(_storage._db->lastRowId());
        }

        Set<Value, Int> _storage;

        Statement insertWithoutIdStatement;
    };
}
