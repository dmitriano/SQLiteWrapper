#include "SQLiteWrapper/TransactionGuard.h"
#include "SQLiteWrapper/Database.h"

#include <exception>
#include <format>

namespace sqlite
{
    TransactionGuard::TransactionGuard(Database& db) :
        _db(db)
    {
        ++_db._transactionLevel;

        _savepoint = std::format("sp{}", _db._transactionLevel);

        try
        {
            _db.savePoint(_savepoint.c_str());
            _active = true;
        }
        catch (const std::exception& e)
        {
            _db.logger().error("Failed to create savepoint '{}': {}", _savepoint, e.what());

            --_db._transactionLevel;

            throw;
        }
        catch (...)
        {
            _db.logger().error("Failed to create savepoint '{}'.", _savepoint);

            --_db._transactionLevel;

            throw;
        }
    }

    TransactionGuard::~TransactionGuard() noexcept
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

    void TransactionGuard::commit()
    {
        if (_active)
        {
            try
            {
                _db.release(_savepoint.c_str());
            }
            catch (const std::exception& e)
            {
                _db.logger().error("Failed to release savepoint '{}': {}", _savepoint, e.what());

                finish();

                throw;
            }
            catch (...)
            {
                _db.logger().error("Failed to release savepoint '{}'.", _savepoint);

                finish();

                throw;
            }

            finish();
        }
    }

    void TransactionGuard::rollback()
    {
        if (_active)
        {
            try
            {
                _db.logger().error("Rolling back to savepoint '{}'.", _savepoint);
                _db.rollbackTo(_savepoint.c_str());
            }
            catch (const std::exception& e)
            {
                _db.logger().error("Failed to rollback to savepoint '{}': {}", _savepoint, e.what());

                finish();

                throw;
            }
            catch (...)
            {
                _db.logger().error("Failed to rollback to savepoint '{}'.", _savepoint);

                finish();

                throw;
            }

            finish();
        }
    }

    void TransactionGuard::finish() noexcept
    {
        --_db._transactionLevel;
        _active = false;
    }
}
