#pragma once

#include "SQLiteWrapper/Database.h"
#include "SQLiteWrapper/FieldListBuilder.h"
#include "SQLiteWrapper/Helpers.h"
#include "SQLiteWrapper/Element.h"
#include "SQLiteWrapper/Statement.h"
#include "SQLiteWrapper/QueryBuilder.h"

#include "Awl/Observer.h"

#include <memory>
#include <cassert>
#include <sstream>

namespace sqlite
{
    template <class Value, class... Keys>
    class IndexInstantiator : public awl::Observer<Element>
    {
    private:

        using Record = Value;
        using KeyTuple = std::tuple<Keys...>;
        using PtrTuple = std::tuple<Keys Value::*...>;

    public:

        IndexInstantiator(const std::shared_ptr<Database>& db, std::string table_name, std::string index_name, PtrTuple id_ptrs, bool unique = false) :
            _db(db),
            tableName(std::move(table_name)),
            indexName(std::move(index_name)),
            idPtrs(id_ptrs),
            _unique(unique)
        {}

        IndexInstantiator(std::string table_name, std::string index_name, PtrTuple id_ptrs, bool unique = false) :
            tableName(std::move(table_name)),
            indexName(std::move(index_name)),
            idPtrs(id_ptrs),
            _unique(unique)
        {}

        void create(DatabaseRef db_ref) override
        {
            Database& db = db_ref.get();

            if (!db.indexExists(indexName))
            {
                std::ostringstream out;

                out << "CREATE ";
                
                if (_unique)
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

                db.logger()->debug(_T("Creating index '{}': \n{}"), indexName, query);

                db.exec(query);

                db.invalidateScheme();
            }
            else
            {
                db.logger()->debug(_T("Index '{}' already exists."), indexName);
            }
        }

        void deleteElement(DatabaseRef db_ref) override
        {
            Database& db = db_ref.get();

            db.dropIndex(indexName);
        }

        Statement makeSelectStatement() const
        {
            assert(_db != nullptr);

            return makeSelectStatement(*_db);
        }

        Statement makeSelectStatement(Database& db) const
        {
            // Where clause with sequential indices.
            const std::string query = buildParameterizedSelectQuery<Record>(tableName, {}, helpers::findTransparentFieldIndices(idPtrs), true);

            db.logger()->debug(_T("'{}' IndexInstantiator select query: {}"), indexName, query);

            return Statement(db, query);
        }

    private:

        std::shared_ptr<Database> _db;

        const std::string tableName;
        const std::string indexName;

        const PtrTuple idPtrs;

        const bool _unique;
    };
}

