#include "SQLiteWrapper/SequenceTable.h"
#include "SQLiteWrapper/Bind.h"
#include "SQLiteWrapper/Database.h"
#include "SQLiteWrapper/Exception.h"
#include "SQLiteWrapper/Get.h"
#include "SQLiteWrapper/Statement.h"
#include "SQLiteWrapper/TableInstantiator.h"

#include "Awl/Reflection.h"
#include "Awl/ScopeGuard.h"

#include <format>
#include <functional>
#include <tuple>

namespace
{
    struct SequenceRecord
    {
        std::string name;
        int64_t value;

        AWL_REFLECT(name, value)
    };

    sqlite::TableInstantiator sequence_instantiator("sequences", std::make_tuple(&SequenceRecord::name),
        std::function<void(sqlite::TableBuilder<SequenceRecord>&)>([](sqlite::TableBuilder<SequenceRecord>& builder)
        {
            builder.setColumnConstraint(&SequenceRecord::value,
                "NOT NULL CHECK(typeof(value) = 'integer' AND value >= 0)");
        }));

    const char* logQuery(sqlite::Database& db, const char* query)
    {
        db.logger()->trace("Preparing sequence query: {}", query);
        return query;
    }
}

namespace sqlite
{
    SequenceTable::SequenceTable(const std::shared_ptr<Database>& db) :
        _db(db),
        _createStatement(*_db, logQuery(*_db,
            "INSERT INTO sequences(name, value) VALUES(?1, ?2) ON CONFLICT(name) DO NOTHING;")),
        _nextStatement(*_db, logQuery(*_db,
            "UPDATE sequences SET value = value + 1 "
            "WHERE name = ?1 AND value < 9223372036854775807 RETURNING value;"))
    {}

    Scheme SequenceTable::createScheme()
    {
        Scheme scheme;
        scheme.subscribe(&sequence_instantiator);
        return scheme;
    }

    void SequenceTable::createSequence(const std::string& name, const int64_t initial_value)
    {
        if (initial_value < 0)
        {
            throw SQLiteException("Sequence initial value must be nonnegative.");
        }

        const auto reset = awl::make_scope_guard([this]
        {
            _createStatement.reset(false);
            _createStatement.clearBindings();
        });
        sqlite::bind(_createStatement, 0, name);
        sqlite::bind(_createStatement, 1, initial_value);
        _createStatement.exec();
    }

    int64_t SequenceTable::nextId(const std::string& name)
    {
        const auto reset = awl::make_scope_guard([this]
        {
            _nextStatement.reset(false);
            _nextStatement.clearBindings();
        });
        sqlite::bind(_nextStatement, 0, name);

        if (!_nextStatement.next())
        {
            throw SQLiteException(std::format("Sequence '{}' does not exist or has exhausted its IDs.", name));
        }

        int64_t id;
        sqlite::get(_nextStatement, 0, id);

        // Finish UPDATE RETURNING before handing the ID to the caller (including commit errors).
        if (_nextStatement.next())
        {
            throw SQLiteException("Sequence update returned more than one row.");
        }

        return id;
    }
}
