#pragma once

#include "SQLiteWrapper/Database.h"
#include "SQLiteWrapper/TableBuilder.h"
#include "SQLiteWrapper/Element.h"

#include "Awl/StringFormat.h"

#include <memory>

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

        TableInstantiator(const std::shared_ptr<Database>& db, std::string table_name, PtrTuple id_ptrs) :
            m_db(db),
            tableName(std::move(table_name)),
            idPtrs(id_ptrs)
        {
            m_db->Subscribe(this);
        }

        void Create() override
        {
            if (!m_db->TableExists(tableName))
            {
                TableBuilder<Record> builder(tableName);

                builder.SetPrimaryKeyTuple(idPtrs);

                AddConstraints(builder);

                const std::string query = builder.Create();

                m_db->logger().debug(awl::format() << "Creating table '" << tableName << "': \n" << query);

                m_db->Exec(query);
            }
            else
            {
                m_db->logger().debug(awl::format() << "Table '" << tableName << "' already exists.");
            }
        }

        void Delete() override
        {
            m_db->DropTable(tableName);
        }

        // Adds constraints like REFERENCES, NULL, UNIQUE, etc...
        virtual void AddConstraints(TableBuilder<Record>& builder)
        {
            static_cast<void>(builder);
        }

    private:

        std::shared_ptr<Database> m_db;
        const PtrTuple idPtrs;

        const std::string tableName;
    };
}
