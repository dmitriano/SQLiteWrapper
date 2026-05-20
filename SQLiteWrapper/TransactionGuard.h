#pragma once

#include <memory>
#include <string>
#include <utility>

namespace sqlite
{
    class Database;

    class TransactionGuard
    {
    public:

        explicit TransactionGuard(std::shared_ptr<Database> db);

        ~TransactionGuard() noexcept;

        TransactionGuard(const TransactionGuard&) = delete;

        TransactionGuard& operator = (const TransactionGuard&) = delete;

        TransactionGuard(TransactionGuard&& other) noexcept :
            _db(std::move(other._db)),
            _savepoint(std::move(other._savepoint))
        {
            other._db.reset();
        }

        TransactionGuard& operator = (TransactionGuard&& other) noexcept
        {
            if (this != &other)
            {
                _db = std::move(other._db);
                other._db.reset();
                _savepoint = std::move(other._savepoint);
            }

            return *this;
        }

        void commit();

    private:

        void rollback();

        void finish() noexcept;

        std::shared_ptr<Database> _db;

        std::string _savepoint;
    };
}
