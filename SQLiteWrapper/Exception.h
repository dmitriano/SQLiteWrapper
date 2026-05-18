#pragma once

#include "Awl/Exception.h"

#include <format>
#include <typeinfo>

namespace sqlite
{
    class SQLiteException : public awl::Exception
    {
    public:

        explicit SQLiteException(std::string message) : SQLiteException(0, std::move(message)) {}
        
        SQLiteException(int code, std::string message) : _code(code), _message(std::move(message)) {}

        const char* what() const throw() override
        {
            return _message.c_str();
        }

        awl::String message() const override
        {
            return std::format(_T("SQlite error code: {}, {}"), _code, awl::fromAString(_message));
        }

        int code() const
        {
            return _code;
        }

    private:

        const int _code;

        const std::string _message;
    };
}

