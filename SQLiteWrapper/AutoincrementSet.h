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
            m_storage(db, std::move(table_name), std::make_tuple(id_ptr))
        {
            insertWithoutIdStatement = m_storage.makeStatement("autoinsert", buildParameterizedInsertQuery<Value>(m_storage.tableName, m_storage.valueFilter()));
        }

        AutoincrementSet(const AutoincrementSet&) = delete;
        AutoincrementSet(AutoincrementSet&&) = default;

        AutoincrementSet& operator = (const AutoincrementSet&) = delete;
        AutoincrementSet& operator = (AutoincrementSet&&) = default;

        void close()
        {
            m_storage.close();

            insertWithoutIdStatement.close();
        }

        Iterator<Value> begin()
        {
            return m_storage.begin();
        }

        IteratorSentinel<Value> end()
        {
            return m_storage.end();
        }

        void insert(Value& val)
        {
            m_storage.bindValue(insertWithoutIdStatement, val, m_storage.valueFilter());

            insertWithoutIdStatement.exec();

            assignLastRowId(val);
        }

        // It may still violate some constraint like UNIQUE index on other columns.
        bool tryinsert(Value& val)
        {
            m_storage.bindValue(insertWithoutIdStatement, val, m_storage.valueFilter());

            const bool success = insertWithoutIdStatement.exec();

            if (success)
            {
                assignLastRowId(val);
            }

            return success;
        }

        void insertWithId(const Value& val)
        {
            m_storage.insert(val);
        }

        bool tryinsertWithId(const Value& val)
        {
            return m_storage.tryinsert(val);
        }

        bool find(Value& val)
        {
            return m_storage.find(val);
        }

        bool find(Int id, Value& val)
        {
            return m_storage.find(std::make_tuple(id), val);
        }

        void update(const Value& val)
        {
            m_storage.update(val);
        }

        template <class... Field>
        Updater<Value> createUpdater(std::tuple<Field Value::*...> field_ptrs) const
        {
            return m_storage.createUpdater(field_ptrs);
        }

        void tryDeleteRecord(Int id)
        {
            m_storage.tryDeleteRecord(std::make_tuple(id));
        }

        void deleteElement(Int id)
        {
            m_storage.deleteElement(std::make_tuple(id));
        }

        void tryDeleteRecord(const Value& val)
        {
            m_storage.tryDeleteRecord(val);
        }

        void deleteElement(const Value& val)
        {
            m_storage.deleteElement(val);
        }

    private:

        void assignLastRowId(Value& val) const
        {
            Int Value::* id_ptr = std::get<0>(m_storage.idPtrs);

            // TODO: What about signed/unsigned?
            val.*id_ptr = static_cast<Int>(m_storage.m_db->lastRowId());
        }

        Set<Value, Int> m_storage;

        Statement insertWithoutIdStatement;
    };
}
