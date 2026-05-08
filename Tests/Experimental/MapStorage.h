#pragma once

#include "Awl/LegacyFormat.h"

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

        MapStorage(sqlite::Database& db, std::string table_name) : _db(db), tableName(table_name)
        {}

        void CreateTable()
        {
            if (!_db.tableExists(tableName))
            {
                sqlite::TableBuilder<Record> builder(tableName);

                builder.setColumnConstraint(&Record::id, "INTEGER NOT NULL PRIMARY KEY");

                _db.exec(builder.create());
            }
        }

        void Prepare()
        {
            sqlite::IndexFilter id_filter{ 0 };

            sqlite::IndexFilter value_filter;
                
            for (size_t i = 1; i < sqlite::helpers::fieldCount<Record>(); ++i)
            {
                value_filter.insert(i);
            }

            insertStatement = sqlite::Statement(_db, sqlite::buildParameterizedInsertQuery<Record>(tableName));

            updateStatement = sqlite::Statement(_db, sqlite::buildParameterizedUpdateQuery<Record>(tableName, value_filter, id_filter));

            selectStatement = sqlite::Statement(_db, sqlite::buildParameterizedSelectQuery<Record>(tableName, value_filter, id_filter));
        }

        void insert(const Key& id, const Value& val)
        {
            sqlite::bind(insertStatement, 0, id);
            sqlite::bind(insertStatement, 1, val);

            insertStatement.exec();
        }

        bool find(const Key& id, Value& val)
        {
            sqlite::bind(selectStatement, 0, id);

            const bool exists = selectStatement.next();
            
            if (exists)
            {
                sqlite::get(selectStatement, 0, val);
            }

            selectStatement.reset();

            return exists;
        }

        void update(const Key& id, const Value& val)
        {
            sqlite::bind(updateStatement, 0, id);
            sqlite::bind(updateStatement, 1, val);

            updateStatement.exec();
        }

    private:

        sqlite::Database& _db;
        
        std::string tableName;

        sqlite::Statement insertStatement;
        sqlite::Statement updateStatement;
        sqlite::Statement selectStatement;
    };
}

