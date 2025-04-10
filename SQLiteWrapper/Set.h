#pragma once

#include "Awl/StringFormat.h"
#include "Awl/Separator.h"

#include "SQLiteWrapper/Helpers.h"
#include "SQLiteWrapper/TableBuilder.h"
#include "SQLiteWrapper/QueryBuilder.h"
#include "SQLiteWrapper/Statement.h"
#include "SQLiteWrapper/Bind.h"
#include "SQLiteWrapper/Get.h"
#include "SQLiteWrapper/Updater.h"
#include "SQLiteWrapper/Iterator.h"

#include <deque>
#include <limits>
#include <string>
#include <iostream>
#include <algorithm>

namespace sqlite
{
    template <class Value, class... Keys>
    class Set
    {
    private:

        using Record = Value;
        using KeyTuple = std::tuple<Keys...>;
        using PtrTuple = std::tuple<Keys Value::*...>;

    public:

        Set(const std::shared_ptr<Database>& db, std::string table_name, PtrTuple id_ptrs) :
            m_db(db),
            tableName(std::move(table_name)),
            idPtrs(std::move(id_ptrs)),
            idIndices(FindKeyIndices())
        {
            insertStatement = MakeStatement("insert",  BuildParameterizedInsertQuery<Record>(tableName));

            IndexFilter value_filter = MakeValueFilter();

            // A table containins only key columns can't be updated.
            if (!value_filter.empty())
            {
                updateStatement = MakeStatement("update", BuildParameterizedUpdateQuery<Record>(tableName, std::move(value_filter), idIndices));

                selectStatement = MakeStatement("select", BuildParameterizedSelectQuery<Record>(tableName, {}, idIndices));
            }

            deleteStatement = MakeStatement("delete", BuildParameterizedDeleteQuery<Record>(tableName, idIndices));

            iterateStatement = MakeStatement("iterate", BuildTrivialSelectQuery<Value>(tableName));
        }

        Set(const Set&) = delete;
        Set(Set&&) = default;
        
        Set& operator = (const Set&) = delete;
        Set& operator = (Set&&) = default;

        void Close()
        {
            insertStatement.Close();
            updateStatement.Close();
            selectStatement.Close();
            deleteStatement.Close();
            iterateStatement.Close();
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

            Statement stmt = MakeStatement("update", BuildParameterizedUpdateQuery<Record>(tableName, value_filter, idIndices));

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

        template <class Value1, class Int> requires std::is_integral_v<Int>
        friend class AutoincrementSet;

        IndexFilter MakeValueFilter() const
        {
            IndexFilter value_filter;

            for (size_t i = 0; i < helpers::GetFieldCount<Record>(); ++i)
            {
                if (!idIndices.contains(i))
                {
                    value_filter.insert(i);
                }
            }

            return value_filter;
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
            Bind(stmt, 0, val);
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

        Statement MakeStatement(const std::string log_prefix, const std::string& query) const
        {
            m_db->logger().debug(awl::format() << "Set " << log_prefix << ": " << query);

            return Statement(*m_db, query);
        };

        std::shared_ptr<Database> m_db;

        // Used by CreateUpdater
        const std::string tableName;

        const PtrTuple idPtrs;

        const IndexFilter idIndices;

        Statement insertStatement;
        Statement updateStatement;
        Statement selectStatement;
        Statement deleteStatement;
        Statement iterateStatement;
    };
}
