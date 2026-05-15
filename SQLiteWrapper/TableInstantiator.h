#pragma once

#include "SQLiteWrapper/Database.h"
#include "SQLiteWrapper/TableBuilder.h"
#include "SQLiteWrapper/Element.h"
#include "SQLiteWrapper/Set.h"

#include "Awl/Observer.h"

#include <memory>
#include <functional>
#include <cassert>

namespace sqlite
{
    template <class Value, class... Keys>
    class TableInstantiator : public awl::Observer<Element>
    {
    private:

        using Record = Value;
        using KeyTuple = std::tuple<Keys...>;
        using PtrTuple = std::tuple<Keys Value::*...>;

    public:

        TableInstantiator(const std::shared_ptr<Database>& db, std::string table_name, PtrTuple id_ptrs,
            std::function<void(TableBuilder<Record>&)> add_constraints = {})
        :
            _db(db),
            tableName(std::move(table_name)),
            idPtrs(id_ptrs),
            addConstraints(std::move(add_constraints))
        {}

        TableInstantiator(std::string table_name, PtrTuple id_ptrs,
            std::function<void(TableBuilder<Record>&)> add_constraints = {})
        :
            tableName(std::move(table_name)),
            idPtrs(id_ptrs),
            addConstraints(std::move(add_constraints))
        {}

        void create(Database& db) override
        {
            if (!db.tableExists(tableName))
            {
                TableBuilder<Record> builder(tableName);

                builder.setPrimaryKeyTuple(idPtrs);

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

        using SetType = Set<Value, Keys...>;

        SetType makeSet() const
        {
            assert(_db != nullptr);

            return makeSet(_db);
        }

        SetType makeSet(const std::shared_ptr<Database>& db) const
        {
            return SetType(db, tableName, idPtrs);
        }

    private:

        std::shared_ptr<Database> _db;

        const std::string tableName;

        const PtrTuple idPtrs;

        std::function<void(TableBuilder<Record>&)> addConstraints;
    };
}

