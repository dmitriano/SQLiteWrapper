#pragma once

#include "SQLiteWrapper/Database.h"

#include <exception>
#include <format>
#include <string>
#include <utility>

namespace sqlite
{
    class TransactionGuard
    {
    public:

        explicit TransactionGuard(Database& db, std::string savepoint = {}) :
            _db(db)
        {
            ++_db._transactionLevel;

            if (savepoint.empty())
            {
                savepoint = std::format("sp{}", _db._transactionLevel);
            }

            _savepoint = std::move(savepoint);

            try
            {
                _db.savePoint(_savepoint.c_str());
                _active = true;
            }
            catch (...)
            {
                --_db._transactionLevel;

                throw;
            }
        }

        ~TransactionGuard() noexcept
        {
            if (_active)
            {
                try
                {
                    rollback();
                }
                catch (const std::exception& e)
                {
                    _db.logger().error("Failed to rollback to savepoint '{}': {}", _savepoint, e.what());
                }
                catch (...)
                {
                    _db.logger().error("Failed to rollback to savepoint '{}'.", _savepoint);
                }
            }
        }

        TransactionGuard(const TransactionGuard&) = delete;

        TransactionGuard& operator = (const TransactionGuard&) = delete;

        TransactionGuard(TransactionGuard&&) = delete;

        TransactionGuard& operator = (TransactionGuard&&) = delete;

        void commit()
        {
            if (_active)
            {
                try
                {
                    _db.release(_savepoint.c_str());
                }
                catch (...)
                {
                    finish();

                    throw;
                }

                finish();
            }
        }

    private:

        void rollback()
        {
            if (_active)
            {
                try
                {
                    _db.logger().error("Rolling back to savepoint '{}'.", _savepoint);
                    _db.rollbackTo(_savepoint.c_str());
                }
                catch (...)
                {
                    finish();

                    throw;
                }

                finish();
            }
        }

        void finish() noexcept
        {
            --_db._transactionLevel;
            _active = false;
        }

        Database& _db;

        std::string _savepoint;

        bool _active = false;
    };
}
