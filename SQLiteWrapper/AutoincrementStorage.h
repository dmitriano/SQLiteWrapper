#pragma once

#include "SQLiteWrapper/NaturalStorage.h"

namespace sqlite
{
    template <class Value, class Int> requires std::is_integral_v<Int>
    class AutoincrementStorage
    {
    public:

        AutoincrementStorage(const std::shared_ptr<Database>& db, std::string table_name, Int Value::* id_ptr) :
            m_storage(db, std::move(table_name), std::make_tuple(id_ptr))
        {}

        AutoincrementStorage(const AutoincrementStorage&) = delete;
        AutoincrementStorage(AutoincrementStorage&&) = default;

        AutoincrementStorage& operator = (const AutoincrementStorage&) = delete;
        AutoincrementStorage& operator = (AutoincrementStorage&&) = default;

        void Create() override
        {
            m_storage.Create();
        }

        void Prepare() override
        {
            m_storage.Prepare();
        }

        Iterator<Value> begin()
        {
            return m_storage.begin();
        }

        IteratorSentinel<Value> end()
        {
            return m_storage.end();
        }

        void Insert(const Value& val)
        {
            m_storage.Insert(val);
        }

        bool TryInsert(const Value& val)
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
    };
}
