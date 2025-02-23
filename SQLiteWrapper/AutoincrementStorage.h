#pragma once

#include "SQLiteWrapper/NaturalStorage.h"

namespace sqlite
{
    template <class Value, class Int> requires std::is_integral_v<Int>
    class SetStorage : public awl::Observer<Element>
    {
    private:

        using Record = Value;
        using KeyTuple = std::tuple<Int>;
        using PtrTuple = std::tuple<Int Value::*>;

    public:

        SetStorage(const std::shared_ptr<Database>& db, std::string table_name, Int Value::* id_ptr) :
            m_db(db),
            tableName(std::move(table_name)),
            idPtr(id_ptr),
            rowIdIndex(FindRowIdIndex())
        {
            m_db->Subscribe(this);
        }

        SetStorage(const SetStorage&) = delete;
        SetStorage(SetStorage&&) = default;

        ~SetStorage()
        {
            m_db->Unsubscribe(this);
        }

        SetStorage& operator = (const SetStorage&) = delete;
        SetStorage& operator = (SetStorage&&) = default;

        void Create() override
        {
            if (!m_db->TableExists(tableName))
            {
                TableBuilder<Record> builder(tableName);

                builder.AddColumns();

                builder.SetPrimaryKeyTuple(std::make_tuple(idPtr));

                const std::string query = builder.Build();

                m_db->Exec(query);
            }
        }

        void Prepare() override
        {
            insertFilter = IndexFilter{};

            IndexFilter value_filter;

            for (size_t i = 0; i < helpers::GetFieldCount<Record>(); ++i)
            {
                if (!idIndices.contains(i))
                {
                    value_filter.insert(i);
                }

                if (insertFilter && i != rowIdIndex)
                {
                    insertFilter->insert(i);
                }
            }

            insertStatement = Statement(*m_db, BuildParameterizedInsertQuery<Record>(tableName, insertFilter));

            updateStatement = Statement(*m_db, BuildParameterizedUpdateQuery<Record>(tableName, std::move(value_filter), idIndices));

            selectStatement = Statement(*m_db, BuildParameterizedSelectQuery<Record>(tableName, {}, idIndices));

            deleteStatement = Statement(*m_db, BuildParameterizedDeleteQuery<Record>(tableName, idIndices));

            iterateStatement = Statement(*m_db, BuildTrivialSelectQuery<Value>(tableName));
        }

        Iterator<Value> begin()
        {
            return iterateStatement;
        }

        IteratorSentinel<Value> end()
        {
            return IteratorSentinel<Value>{};
        }

        void Insert(const Value& val)
        {
            BindInsertFields(insertStatement, val);

            insertStatement.Exec();
        }

        bool TryInsert(const Value& val)
        {
            BindInsertFields(insertStatement, val);

            return insertStatement.TryExec();
        }

        bool Find(Value& val)
        {
            BindKeyFromValue(selectStatement, val);

            return SelectValue(val);
        }

        bool Find(const KeyTuple& ids, Value& val)
        {
            BindKey(selectStatement, ids);

            return SelectValue(val);
        }

        void Update(const Value& val)
        {
            Bind(updateStatement, 0, val);

            updateStatement.Exec();

            m_db->EnsureAffected(1);
        }

        template <class... Field>
        Updater<Record> CreateUpdater(std::tuple<Field Value::*...> field_ptrs) const
        {
            IndexFilter value_filter = helpers::FindTransparentFieldIndices(field_ptrs);

            Statement stmt(*m_db, BuildParameterizedUpdateQuery<Record>(tableName, value_filter, idIndices));

            return Updater<Record>(*m_db, std::move(stmt), idIndices, value_filter);
        }

        void TryDelete(const KeyTuple& ids)
        {
            BindKey(deleteStatement, ids);

            deleteStatement.Exec();
        }

        void Delete(const KeyTuple& ids)
        {
            TryDelete(ids);

            m_db->EnsureAffected(1);
        }

        void TryDelete(const Value& val)
        {
            BindKeyFromValue(deleteStatement, val);

            deleteStatement.Exec();
        }

        void Delete(const Value& val)
        {
            TryDelete(val);

            m_db->EnsureAffected(1);
        }

    private:

        size_t FindRowIdIndex() const
        {
            size_t index = noIndex;

            awl::for_each(idPtrs, [this, &index](auto& id_ptr)
                {
                    const size_t member_index = helpers::FindFieldIndex(id_ptr);

                    const auto& member_names = Value::get_member_names();

                    const std::string& member_name = member_names[member_index];

                    if (member_name == rowIdFieldName)
                    {
                        index = helpers::FindTransparentFieldIndex(id_ptr);
                    }
                });

            return index;
        }

        IndexFilter FindKeyIndices() const
        {
            return helpers::FindTransparentFieldIndices(idPtrs);
        }

        void BindKey(Statement& stmt, const KeyTuple& ids)
        {
            auto i = idIndices.begin();

            awl::for_each(ids, [&stmt, &i](auto& field_val)
                {
                    // This requires the indeces to be std::vector but not std::set.
                    const size_t id_index = *i++;

                    Bind(stmt, id_index, field_val);
                });
        }

        void BindValue(Statement& stmt, const Value& val, IndexFilter filter)
        {
            helpers::ForEachFieldValue(val, [this, &stmt, &filter](auto& field, auto field_index)
                {
                    if (filter.contains(field_index))
                    {
                        Bind(stmt, field_index, field);
                    }
                });
        }

        void BindKeyFromValue(Statement& stmt, const Value& val)
        {
            BindValue(stmt, val, idIndices);
        }

        void BindInsertFields(Statement& stmt, const Value& val)
        {
            if (insertFilter)
            {
                BindValue(stmt, val, *insertFilter);
            }
            else
            {
                Bind(stmt, 0, val);
            }
        }

        bool SelectValue(Value& val)
        {
            const bool exists = selectStatement.Next();

            if (exists)
            {
                Get(selectStatement, 0, val);
            }

            selectStatement.Reset();

            return exists;
        }

        std::shared_ptr<Database> m_db;

        const std::string tableName;

        const PtrTuple idPtr;
        const IndexFilter idIndices;
        OptionalIndexFilter insertFilter;

        size_t rowIdIndex = noIndex;

        Statement insertStatement;
        Statement updateStatement;
        Statement selectStatement;
        Statement deleteStatement;
        Statement iterateStatement;
    };
}
