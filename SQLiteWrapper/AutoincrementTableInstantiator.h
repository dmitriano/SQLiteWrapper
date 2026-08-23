#pragma once

#include "SQLiteWrapper/Database.h"
#include "SQLiteWrapper/TableBuilder.h"
#include "SQLiteWrapper/Element.h"
#include "SQLiteWrapper/AutoincrementSet.h"

#include "Awl/Observer.h"

#include <memory>
#include <functional>
#include <cassert>

namespace sqlite
{
    template <class Value, class Int> requires std::is_integral_v<Int>
    class AutoincrementTableInstantiator : public awl::Observer<Element>
    {
    private:

        using Record = Value;

    public:

        AutoincrementTableInstantiator(const std::shared_ptr<Database>& db, std::string table_name, Int Value::* id_ptr,
            std::function<void(TableBuilder<Value>&)> add_constraints = {})
        :
            _db(db),
            tableName(std::move(table_name)),
            idPtr(id_ptr),
            addConstraints(std::move(add_constraints))
        {}

        AutoincrementTableInstantiator(std::string table_name, Int Value::* id_ptr,
            std::function<void(TableBuilder<Value>&)> add_constraints = {})
        :
            tableName(std::move(table_name)),
            idPtr(id_ptr),
            addConstraints(std::move(add_constraints))
        {}

        void create(Database& db) override
        {
            if (!db.tableExists(tableName))
            {
                TableBuilder<Record> builder(tableName);

                // The ROWID chosen for the new row is at least one larger than the largest ROWID that has ever before existed in that same table.
                // For this to apply, we need to explicilty use AUTOINCREMENT keyword:
                builder.setColumnConstraint(idPtr, "NOT NULL PRIMARY KEY AUTOINCREMENT");

                if (addConstraints)
                {
                    // Adds constraints like REFERENCES, NULL, UNIQUE, etc...
                    addConstraints(builder);
                }

                const std::string query = builder.create();

                db.logger()->debug(_T("Creating table '{}': \n{}"), tableName, query);

                db.exec(query);

                db.invalidateScheme();
            }
            else
            {
                db.logger()->debug(_T("Table '{}' already exists."), tableName);
            }
        }

        void drop(Database& db) override
        {
            db.dropTable(tableName);
        }

        using SetType = AutoincrementSet<Value, Int>;

        SetType makeSet() const
        {
            assert(_db != nullptr);

            return makeSet(_db);
        }

        SetType makeSet(const std::shared_ptr<Database>& db) const
        {
            return SetType(db, tableName, idPtr);
        }

    private:

        std::shared_ptr<Database> _db;

        const std::string tableName;

        Int Value::* const idPtr;

        std::function<void(TableBuilder<Record>&)> addConstraints;
    };
}

