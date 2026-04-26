#pragma once

#include "SQLiteWrapper/Database.h"
#include "SQLiteWrapper/FieldListBuilder.h"
#include "SQLiteWrapper/Helpers.h"
#include "SQLiteWrapper/Element.h"
#include "SQLiteWrapper/Statement.h"
#include "SQLiteWrapper/QueryBuilder.h"

#include "Awl/LegacyFormat.h"

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
            m_db->subscribe(this);
        }

        void create() override
        {
            if (!m_db->indexExists(indexName))
            {
                std::ostringstream out;

                out << "CREATE ";
                
                if (m_unique)
                {
                    out << "UNIQUE ";
                }

                out << "INDEX '" << indexName << "' ON '" << tableName << "' (";

                {
                    FieldListBuilder<Record> field_builder(out, makeCommaSeparator());

                    field_builder.setFilter(helpers::findTransparentFieldIndices(idPtrs));

                    helpers::forEachColumn<Record>(field_builder);
                }
                
                out << ");";

                const std::string query = out.str();

                m_db->logger().debug(awl::format() << "Creating index '" << indexName << "': \n" << query);

                m_db->exec(query);

                m_db->invalidateScheme();
            }
            else
            {
                m_db->logger().debug(awl::format() << "Index '" << indexName << "' already exists.");
            }
        }

        void deleteElement() override
        {
            m_db->dropIndex(indexName);
        }

        Statement makeSelectStatement() const
        {
            // Where clause with sequential indices.
            const std::string query = buildParameterizedSelectQuery<Record>(tableName, {}, helpers::findTransparentFieldIndices(idPtrs), true);

            m_db->logger().debug(awl::format() << "'" << indexName << "' IndexInstantiator select query: " << query);

            return Statement(*m_db, query);
        }

    private:

        std::shared_ptr<Database> m_db;

        const std::string tableName;
        const std::string indexName;

        const PtrTuple idPtrs;

        const bool m_unique;
    };
}

