#pragma once

#include "Awl/StringFormat.h"

#include "SQLiteWrapper/TableBuilder.h"
#include "SQLiteWrapper/QueryBuilder.h"
#include "SQLiteWrapper/Statement.h"
#include "SQLiteWrapper/Bind.h"
#include "SQLiteWrapper/Get.h"

#include <deque>
#include <limits>
#include <string>
#include <iostream>
#include <algorithm>

namespace sqlite
{
    template <class Key, class Value>
    class MapStorage
    {
    private:

        struct Record
        {
            Key id;
            Value value;

            AWL_REFLECT(id, value)
        };

    public:

        MapStorage(sqlite::Database& db, std::string table_name) : m_db(db), tableName(table_name)
        {
        }

        void CreateTable()
        {
            if (!m_db.TableExists(tableName))
            {
                sqlite::TableBuilder<Record> builder(tableName);

                builder.SetColumnConstraint(&Record::id, "INTEGER NOT NULL PRIMARY KEY");

                m_db.Exec(builder.Create());
            }
        }

        void Prepare()
        {
            sqlite::IndexFilter id_filter{ 0 };

            sqlite::IndexFilter value_filter;
                
            for (size_t i = 1; i < sqlite::helpers::GetFieldCount<Record>(); ++i)
            {
                value_filter.emplace(i);
            }

            insertStatement = sqlite::Statement(m_db, sqlite::BuildParameterizedInsertQuery<Record>(tableName));

            updateStatement = sqlite::Statement(m_db, sqlite::BuildParameterizedUpdateQuery<Record>(tableName, value_filter, id_filter));

            selectStatement = sqlite::Statement(m_db, sqlite::BuildParameterizedSelectQuery<Record>(tableName, value_filter, id_filter));
        }

        void Insert(const Key& id, const Value& val)
        {
            sqlite::Bind(insertStatement, 0, id);
            sqlite::Bind(insertStatement, 1, val);

            insertStatement.Exec();
        }

        bool Find(const Key& id, Value& val)
        {
            sqlite::Bind(selectStatement, 0, id);

            const bool exists = selectStatement.Next();
            
            if (exists)
            {
                sqlite::Get(selectStatement, 0, val);
            }

            selectStatement.Reset();

            return exists;
        }

        void Update(const Key& id, const Value& val)
        {
            sqlite::Bind(updateStatement, 0, id);
            sqlite::Bind(updateStatement, 1, val);

            updateStatement.Exec();
        }

    private:

        sqlite::Database& m_db;
        
        std::string tableName;

        sqlite::Statement insertStatement;
        sqlite::Statement updateStatement;
        sqlite::Statement selectStatement;
    };
}
