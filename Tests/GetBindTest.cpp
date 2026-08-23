#include "DbContainer.h"

#include "SQLiteWrapper/Bind.h"
#include "SQLiteWrapper/Get.h"

using namespace swtest;

namespace
{
    const char create_query[] = "CREATE TABLE bots (name TEXT COLLATE NOCASE, state BLOB);";
    const char insert_query[] = "INSERT INTO bots (name, state) VALUES (?1, ?2);";
    const char select_query[] = "SELECT name, state FROM bots";
}

// A test demonstrating that an empty string IS NOT NULL, but an empty BLOB IS NULL and NULL is not TEXT or BLOB.
AWL_TEST(GetbindNull)
{
    DbContainer c(context);

    c._db->exec(create_query);

    {
        sqlite::Statement insert_statement(*c._db, insert_query);

        // Empty string is not Null.
        insert_statement.bindText(0, "");
        insert_statement.bindBlob(1, {});

        insert_statement.exec();

        insert_statement.bindNull(0);
        insert_statement.bindNull(1);

        insert_statement.exec();
    }

    {
        sqlite::Statement select_statement(*c._db, select_query);

        AWL_ASSERT(select_statement.next());

        AWL_ASSERT(!select_statement.isNull(0));
        AWL_ASSERT(select_statement.isText(0));
        AWL_ASSERT(select_statement.isNull(1));
        AWL_ASSERT(!select_statement.isBlob(1));

        {
            std::string text;
            sqlite::get(select_statement, 0, text);
            AWL_ASSERT(text.empty());
        }

        {
            std::vector<uint8_t> blob;
            sqlite::get(select_statement, 1, blob);
            AWL_ASSERT(blob.empty());
        }

        AWL_ASSERT(select_statement.next());

        AWL_ASSERT(select_statement.isNull(0));
        AWL_ASSERT(!select_statement.isText(0));
        AWL_ASSERT(select_statement.isNull(1));
        AWL_ASSERT(!select_statement.isBlob(1));

        {
            std::vector<uint8_t> blob;
            sqlite::get(select_statement, 1, blob);
            AWL_ASSERT(blob.empty());
        }
    }
}

AWL_TEST(GetBindOptional)
{
    DbContainer c(context);

    c._db->exec(create_query);

    const std::optional<std::string> sample_text = "a";
    const std::optional<std::string> null_text;

    {
        sqlite::Statement insert_statement(*c._db, insert_query);

        sqlite::bind(insert_statement, 0, sample_text);

        insert_statement.exec();

        sqlite::bind(insert_statement, 0, null_text);

        insert_statement.exec();
    }

    {
        sqlite::Statement select_statement(*c._db, select_query);

        AWL_ASSERT(select_statement.next());

        {
            std::optional<std::string> text;
            sqlite::get(select_statement, 0, text);
            AWL_ASSERT(text == sample_text);
        }

        AWL_ASSERT(select_statement.next());

        {
            std::optional<std::string> text;
            sqlite::get(select_statement, 0, text);
            AWL_ASSERT(text == null_text);
        }
    }
}
