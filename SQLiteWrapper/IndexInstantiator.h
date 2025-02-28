#pragma once

#include "SQLiteWrapper/Database.h"
#include "SQLiteWrapper/FieldListBuilder.h"
#include "SQLiteWrapper/Helpers.h"
#include "SQLiteWrapper/Element.h"
#include "SQLiteWrapper/Set.h"

#include "Awl/StringFormat.h"

#include <memory>

namespace sqlite
{
    template <class Value, class... Keys>
    class IndexInstantiator : private awl::Observer<Element>
    {
    private:

        using Record = Value;
        using KeyTuple = std::tuple<Keys...>;
        using PtrTuple = std::tuple<Keys Value::*...>;

    public:

        IndexInstantiator(const std::shared_ptr<Database>& db, std::string table_name, std::string index_name, PtrTuple id_ptrs, bool unique = false) :
            m_db(db),
            tableName(std::move(table_name)),
            indexName(std::move(index_name)),
            idPtrs(id_ptrs),
            m_unique(unique)
        {
            m_db->Subscribe(this);
        }

        void Create() override
        {
            if (!m_db->IndexExists(indexName))
            {
                std::ostringstream out;

                out << "CREATE ";
                
                if (m_unique)
                {
                    out << "UNIQUE ";
                }

                out << "INDEX '" << indexName << "' ON '" << tableName << "' (";

                {
                    FieldListBuilder<Record> field_builder(out, MakeCommaSeparator());

                    field_builder.SetFilter(helpers::FindTransparentFieldIndices(idPtrs));

                    helpers::ForEachColumn<Record>(field_builder);
                }
                
                out << ");";

                const std::string query = out.str();

                m_db->logger().debug(awl::format() << "Creating index '" << indexName << "': \n" << query);

                m_db->Exec(query);
            }
            else
            {
                m_db->logger().debug(awl::format() << "Index '" << indexName << "' already exists.");
            }
        }

        void Delete() override
        {
            m_db->DropIndex(indexName);
        }

        using SetType = Set<Value, Keys...>;

        SetType MakeSet() const
        {
            return SetType(m_db, tableName, idPtrs);
        }

    private:

        std::shared_ptr<Database> m_db;

        const std::string tableName;
        const std::string indexName;

        const PtrTuple idPtrs;

        const bool m_unique;
    };
}
