#pragma once

#include "SQLiteWrapper/SetStorage.h"

#include <type_traits>

namespace sqlite
{
    template <class Value, class Int> requires std::is_integral_v<Int>
    class AutoincrementStorage : private awl::Observer<Element>
    {
    public:

        AutoincrementStorage(const std::shared_ptr<Database>& db, std::string table_name, Int Value::* id_ptr) :
            m_storage(db, std::move(table_name), std::make_tuple(id_ptr))
        {
            // Move the observer.
            static_cast<awl::Observer<Element>&>(*this) = std::move(static_cast<awl::Observer<Element>&>(m_storage));
        }

        AutoincrementStorage(const AutoincrementStorage&) = delete;
        AutoincrementStorage(AutoincrementStorage&&) = default;

        AutoincrementStorage& operator = (const AutoincrementStorage&) = delete;
        AutoincrementStorage& operator = (AutoincrementStorage&&) = default;

        void Create() override
        {
            m_storage.Create();
        }

        void Open() override
        {
            m_storage.Open();

            insertWithoutIdStatement = Statement(*m_storage.m_db, BuildParameterizedInsertQuery<Value>(m_storage.tableName, m_storage.MakeValueFilter()));
        }

        void Close() override
        {
            m_storage.Close();

            insertWithoutIdStatement.Close();
        }

        void Delete() override
        {
            m_storage.Delete();
        }

        Iterator<Value> begin()
        {
            return m_storage.begin();
        }

        IteratorSentinel<Value> end()
        {
            return m_storage.end();
        }

        void Insert(Value& val)
        {
            m_storage.BindValue(insertWithoutIdStatement, val, m_storage.MakeValueFilter());

            insertWithoutIdStatement.Exec();

            Int Value::* id_ptr = std::get<0>(m_storage.idPtrs);

            // TODO: What about signed/unsigned?
            val.*id_ptr = static_cast<Int>(m_storage.m_db->GetLastRowId());
        }

        void InsertWithId(const Value& val)
        {
            m_storage.Insert(val);
        }

        bool TryInsertWithId(const Value& val)
        {
            return m_storage.TryInsert(val);
        }

        bool Find(Value& val)
        {
            return m_storage.Find(val);
        }

        bool Find(Int id, Value& val)
        {
            return m_storage.Find(std::make_tuple(id), val);
        }

        void Update(const Value& val)
        {
            m_storage.Update(val);
        }

        template <class... Field>
        Updater<Value> CreateUpdater(std::tuple<Field Value::*...> field_ptrs) const
        {
            return m_storage.CreateUpdater(field_ptrs);
        }

        void TryDelete(Int id)
        {
            m_storage.TryDelete(std::make_tuple(id));
        }

        void Delete(Int id)
        {
            m_storage.Delete(std::make_tuple(id));
        }

        void TryDelete(const Value& val)
        {
            m_storage.TryDelete(val);
        }

        void Delete(const Value& val)
        {
            m_storage.Delete(val);
        }

    private:

        SetStorage<Value, Int> m_storage;

        Statement insertWithoutIdStatement;
    };
}
