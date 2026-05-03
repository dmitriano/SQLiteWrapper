#pragma once

#include "Awl/LegacyFormat.h"
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
            idIndices(findKeyIndices())
        {
            insertStatement = makeStatement("insert",  buildParameterizedInsertQuery<Record>(tableName));

            IndexFilter value_filter = valueFilter();

            // A table containins only key columns can't be updated.
            if (!value_filter.empty())
            {
                updateStatement = makeStatement("update", buildParameterizedUpdateQuery<Record>(tableName, std::move(value_filter), idIndices));

                selectStatement = makeStatement("select", buildParameterizedSelectQuery<Record>(tableName, {}, idIndices));
            }

            deleteStatement = makeStatement("delete", buildParameterizedDeleteQuery<Record>(tableName, idIndices));

            iterateStatement = makeStatement("iterate", buildTrivialSelectQuery<Value>(tableName));
        }

        Set(const Set&) = delete;
        Set(Set&&) = default;
        
        Set& operator = (const Set&) = delete;
        Set& operator = (Set&&) = default;

        void close()
        {
            insertStatement.close();
            updateStatement.close();
            selectStatement.close();
            deleteStatement.close();
            iterateStatement.close();
        }

        Iterator<Value> begin()
        {
            return iterateStatement;
        }

        IteratorSentinel<Value> end()
        {
            return IteratorSentinel<Value>{};
        }

        void insert(const Value& val)
        {
            bindInsertFields(insertStatement, val);

            insertStatement.exec();
        }

        bool tryinsert(const Value& val)
        {
            bindInsertFields(insertStatement, val);

            return insertStatement.tryExec();
        }

        bool find(Value& val)
        {
            bindKeyFromValue(selectStatement, val);

            return selectValue(val);
        }

        bool find(const KeyTuple& ids, Value& val)
        {
            bindKey(selectStatement, ids);

            return selectValue(val);
        }

        void update(const Value& val)
        {
            bind(updateStatement, 0, val);

            updateStatement.exec();

            m_db->ensureAffected(1);
        }

        template <class... Field>
        Updater<Record> createUpdater(std::tuple<Field Value::*...> field_ptrs) const
        {
            IndexFilter value_filter = helpers::findTransparentFieldIndices(field_ptrs);

            Statement stmt = makeStatement("update", buildParameterizedUpdateQuery<Record>(tableName, value_filter, idIndices));

            return Updater<Record>(*m_db, std::move(stmt), idIndices, value_filter);
        }

        void tryDeleteRecord(const KeyTuple& ids)
        {
            bindKey(deleteStatement, ids);

            deleteStatement.exec();
        }

        void deleteElement(const KeyTuple& ids)
        {
            tryDeleteRecord(ids);

            m_db->ensureAffected(1);
        }

        void tryDeleteRecord(const Value& val)
        {
            bindKeyFromValue(deleteStatement, val);

            deleteStatement.exec();
        }

        void deleteElement(const Value& val)
        {
            tryDeleteRecord(val);

            m_db->ensureAffected(1);
        }

    private:

        template <class Value1, class Int> requires std::is_integral_v<Int>
        friend class AutoincrementSet;

        IndexFilter valueFilter() const
        {
            IndexFilter value_filter;

            for (size_t i = 0; i < helpers::fieldCount<Record>(); ++i)
            {
                if (!idIndices.contains(i))
                {
                    value_filter.insert(i);
                }
            }

            return value_filter;
        }

        IndexFilter findKeyIndices() const
        {
            return helpers::findTransparentFieldIndices(idPtrs);
        }

        void bindKey(Statement& stmt, const KeyTuple& ids)
        {
            auto i = idIndices.begin();

            awl::for_each(ids, [&stmt, &i](auto& field_val)
            {
                // This requires the indeces to be std::vector but not std::set.
                const size_t id_index = *i++;

                sqlite::bind(stmt, id_index, field_val);
            });
        }

        void bindValue(Statement& stmt, const Value& val, IndexFilter filter)
        {
            helpers::forEachFieldValue(val, [this, &stmt, &filter](auto& field, auto field_index)
            {
                if (filter.contains(field_index))
                {
                    sqlite::bind(stmt, field_index, field);
                }
            });
        }

        void bindKeyFromValue(Statement& stmt, const Value& val)
        {
            bindValue(stmt, val, idIndices);
        }

        void bindInsertFields(Statement& stmt, const Value& val)
        {
            sqlite::bind(stmt, 0, val);
        }

        bool selectValue(Value& val)
        {
            const bool exists = selectStatement.next();

            if (exists)
            {
                sqlite::get(selectStatement, 0, val);
            }

            selectStatement.reset();

            return exists;
        }

        Statement makeStatement(const std::string log_prefix, const std::string& query) const
        {
            m_db->logger().debug(_T("Set {}: {}"), log_prefix, query);

            return Statement(*m_db, query);
        };

        std::shared_ptr<Database> m_db;

        // Used by createUpdater
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

