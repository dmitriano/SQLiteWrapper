#pragma once

#include <string>

namespace sqlite
{
    class Database;

    class TransactionGuard
    {
    public:

        explicit TransactionGuard(Database& db);

        ~TransactionGuard() noexcept;

        TransactionGuard(const TransactionGuard&) = delete;

        TransactionGuard& operator = (const TransactionGuard&) = delete;

        TransactionGuard(TransactionGuard&&) = delete;

        TransactionGuard& operator = (TransactionGuard&&) = delete;

        void commit();

    private:

        void rollback();

        void finish() noexcept;

        Database& _db;

        std::string _savepoint;

        bool _active = false;
    };
}
