#pragma once

#include "SQLiteWrapper/Database.h"
#include "SQLiteWrapper/TableBuilder.h"
#include "SQLiteWrapper/Element.h"
#include "SQLiteWrapper/Set.h"

#include "Awl/LegacyFormat.h"

#include <memory>
#include <functional>

namespace sqlite
{
    template <class Value, class... Keys>
    class TableInstantiator : private awl::Observer<Element>
    {
    private:

        using Record = Value;
        using KeyTuple = std::tuple<Keys...>;
        using PtrTuple = std::tuple<Keys Value::*...>;

    public:

        TableInstantiator(const std::shared_ptr<Database>& db, std::string table_name, PtrTuple id_ptrs,
            std::function<void(TableBuilder<Record>&)> add_constraints = {})
        :
            m_db(db),
            tableName(std::move(table_name)),
            idPtrs(id_ptrs),
            addConstraints(std::move(add_constraints))
        {
            m_db->subscribe(this);
        }

        void create() override
        {
            if (!m_db->tableExists(tableName))
            {
                TableBuilder<Record> builder(tableName);

                builder.setPrimaryKeyTuple(idPtrs);

                if (addConstraints)
                {
                    // Adds constraints like REFERENCES, NULL, UNIQUE, etc...
                    addConstraints(builder);
                }

                const std::string query = builder.create();

                m_db->logger().debug(awl::format() << "Creating table '" << tableName << "': \n" << query);

                m_db->exec(query);

                m_db->invalidateScheme();
            }
            else
            {
                m_db->logger().debug(awl::format() << "Table '" << tableName << "' already exists.");
            }
        }

        void deleteElement() override
        {
            m_db->dropTable(tableName);
        }

        using SetType = Set<Value, Keys...>;

        SetType makeSet() const
        {
            return SetType(m_db, tableName, idPtrs);
        }

    private:

        std::shared_ptr<Database> m_db;

        const std::string tableName;

        const PtrTuple idPtrs;

        std::function<void(TableBuilder<Record>&)> addConstraints;
    };
}

