#include "SQLiteWrapper/TransactionGuard.h"
#include "SQLiteWrapper/Database.h"

#include <exception>
#include <format>
#include <stdexcept>

namespace sqlite
{
    TransactionGuard::TransactionGuard(std::shared_ptr<Database> db) :
        _db(std::move(db))
    {
        if (!_db)
        {
            throw std::invalid_argument("TransactionGuard database must not be null.");
        }

        ++_db->_transactionLevel;

        _savepoint = std::format("sp{}", _db->_transactionLevel);

        try
        {
            _db->savePoint(_savepoint.c_str());
        }
        catch (const std::exception& e)
        {
            _db->logger().error("Failed to create savepoint '{}': {}", _savepoint, e.what());

            --_db->_transactionLevel;

            throw;
        }
        catch (...)
        {
            _db->logger().error("Failed to create savepoint '{}'.", _savepoint);

            --_db->_transactionLevel;

            throw;
        }
    }

    TransactionGuard::~TransactionGuard() noexcept
    {
        // A rollback failure is fatal here: a noexcept destructor will terminate if rollback throws.
        rollback();
    }

    void TransactionGuard::commit()
    {
        if (_db)
        {
            try
            {
                _db->release(_savepoint.c_str());
            }
            catch (const std::exception& e)
            {
                _db->logger().error("Failed to release savepoint '{}': {}", _savepoint, e.what());

                finish();

                throw;
            }
            catch (...)
            {
                _db->logger().error("Failed to release savepoint '{}'.", _savepoint);

                finish();

                throw;
            }

            finish();
        }
    }

    void TransactionGuard::rollback()
    {
        if (_db)
        {
            try
            {
                _db->logger().error("Rolling back to savepoint '{}'.", _savepoint);
                _db->rollbackTo(_savepoint.c_str());
            }
            catch (const std::exception& e)
            {
                _db->logger().error("Failed to rollback to savepoint '{}': {}", _savepoint, e.what());

                finish();

                throw;
            }
            catch (...)
            {
                _db->logger().error("Failed to rollback to savepoint '{}'.", _savepoint);

                finish();

                throw;
            }

            finish();
        }
    }

    void TransactionGuard::finish() noexcept
    {
        --_db->_transactionLevel;
        _db.reset();
    }
}
