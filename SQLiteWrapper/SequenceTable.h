#pragma once

#include "SQLiteWrapper/Scheme.h"
#include "SQLiteWrapper/Statement.h"

#include <cstdint>
#include <memory>
#include <string>

namespace sqlite
{
    // Synchronous named counters. Serialize calls sharing a Database connection.
    // Increments participate in the caller's transaction and can be rolled back.
    class SequenceTable
    {
    public:

        explicit SequenceTable(const std::shared_ptr<Database>& db);

        // Create the table through the usual schema mechanism before using it.
        static Scheme createScheme();

        // Existing sequences keep their value, regardless of initial_value.
        void createSequence(const std::string& name, int64_t initial_value = 0);

        // Increment atomically and return the new ID. Missing/exhausted counters throw.
        int64_t nextId(const std::string& name);

    private:

        const std::shared_ptr<Database> _db;

        Statement _createStatement;
        Statement _nextStatement;
    };
}
