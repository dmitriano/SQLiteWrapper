#pragma once

#include "Awl/Exception.h"
#include "Awl/LegacyFormat.h"

#include <typeinfo>

namespace sqlite
{
    class SQLiteException : public awl::Exception
    {
    public:

        explicit SQLiteException(std::string message) : SQLiteException(0, std::move(message)) {}
        
        SQLiteException(int code, std::string message) : _code(code), _Message(std::move(message)) {}

        const char* what() const throw() override
        {
            return _Message.c_str();
        }

        awl::String message() const override
        {
            return (awl::format() << _T("SQlite error code: ") << _code << _T(", ") << awl::fromAString(_Message));
        }

        int code() const
        {
            return _code;
        }

    private:

        const int _code;

        const std::string _Message;
    };
}

