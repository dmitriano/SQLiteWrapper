#pragma once

#include "SQLiteWrapper/Database.h"
#include "SQLiteWrapper/TableBuilder.h"
#include "SQLiteWrapper/Element.h"
#include "SQLiteWrapper/AutoincrementSet.h"

#include "Awl/LegacyFormat.h"

#include <memory>
#include <functional>

namespace sqlite
{
    template <class Value, class Int> requires std::is_integral_v<Int>
    class AutoincrementTableInstantiator : private awl::Observer<Element>
    {
    private:

        using Record = Value;

    public:

        AutoincrementTableInstantiator(const std::shared_ptr<Database>& db, std::string table_name, Int Value::* id_ptr,
            std::function<void(TableBuilder<Value>&)> add_constraints = {})
        :
            m_db(db),
            tableName(std::move(table_name)),
            idPtr(id_ptr),
            addConstraints(std::move(add_constraints))
        {
            m_db->subscribe(this);
        }

        void create() override
        {
            if (!m_db->tableExists(tableName))
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

        using SetType = AutoincrementSet<Value, Int>;

        SetType makeSet() const
        {
            return SetType(m_db, tableName, idPtr);
        }

    private:

        std::shared_ptr<Database> m_db;

        const std::string tableName;

        Int Value::* const idPtr;

        std::function<void(TableBuilder<Record>&)> addConstraints;
    };
}

