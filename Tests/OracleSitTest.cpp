#include "DbContainer.h"
#include "SQLiteWrapper/Bind.h"
#include "SQLiteWrapper/Get.h"

#include "Awl/Reflection.h"

#include <variant>
#include <string>
#include <chrono>
#include <optional>
#include <array>
#include <iostream>

using namespace swtest;

namespace
{
    using Clock = std::chrono::system_clock;

    using DateType = std::chrono::time_point<Clock, std::chrono::seconds>;

    struct TimestampType
    {
        std::chrono::time_point<Clock, std::chrono::nanoseconds> time;
        int tz_hour;
        int tz_minute;

        AWL_REFLECT(time, tz_hour, tz_minute);
    };

    AWL_MEMBERWISE_EQUATABLE(TimestampType)

    using NumberType = int64_t;

    struct CommonHeader
    {
        int64_t BeginSit;
        int64_t EndSit;

        AWL_REFLECT(BeginSit, EndSit);
    };
    
    struct DbaUser
    {
        std::string AccountStatus;
        std::optional<std::string> AllShard;
        std::optional<std::string> AuthenticationType;
        std::optional<std::string> Common;
        DateType Created;
        std::optional<std::string> DefaultCollation;
        std::string DefaultTablespace;
        std::optional<std::string> EditionsEnabled;
        std::optional<DateType> ExpiryDate;
        std::optional<std::string> ExternalName;
        std::optional<std::string> Implicit;
        std::optional<std::string> Inherited;
        std::optional<std::string> InitialRsrcConsumerGroup;
        std::optional<TimestampType> LastLogin;
        std::optional<std::string> LocalTempTablespace;
        std::optional<DateType> LockDate;
        std::optional<std::string> OracleMaintained;
        //std::optional<std::string> Password;
        std::optional<DateType> PasswordChangeDate;
        std::optional<std::string> PasswordVersions;
        std::string Profile;
        std::optional<std::string> ProxyOnlyConnect;
        std::string TemporaryTablespace;
        std::string Username;
        NumberType UserId;

        AWL_REFLECT(
            AccountStatus,
            AllShard,
            AuthenticationType,
            Common,
            Created,
            DefaultCollation,
            DefaultTablespace,
            EditionsEnabled,
            ExpiryDate,
            ExternalName,
            Implicit,
            Inherited,
            InitialRsrcConsumerGroup,
            LastLogin,
            LocalTempTablespace,
            LockDate,
            OracleMaintained,
            //Password,
            PasswordChangeDate,
            PasswordVersions,
            Profile,
            ProxyOnlyConnect,
            TemporaryTablespace,
            Username,
            UserId);
    };

    struct UniqueUser
    {
        CommonHeader Header;
        DbaUser User;

        AWL_REFLECT(Header, User);
    };

    struct Other
    {
        int Something;

        AWL_REFLECT(Something);
    };

    struct UniqueUser1
    {
        Other It;

        int64_t BeginSit;
        int64_t EndSit;

        DbaUser User;

        AWL_REFLECT(It, BeginSit, EndSit, User);
    };

    static_assert(sqlite::helpers::GetFieldCount<DbaUser>() == 24);
    static_assert(sqlite::helpers::GetFieldCount<CommonHeader>() == 2);
    static_assert(sqlite::helpers::GetFieldCount<UniqueUser>() == 24 + 2);

    struct DbaUser2
    {
        int formerRowId; //RowId is not allowed anymore.
        std::string AccountStatus;
        std::optional<std::string> AllShard;
        std::optional<std::string> AuthenticationType;
        std::optional<std::string> Common;
        DateType Created;
        std::optional<std::string> DefaultCollation;
        std::string DefaultTablespace;
        std::optional<std::string> EditionsEnabled;
        std::optional<DateType> ExpiryDate;
        std::optional<std::string> ExternalName;
        std::optional<std::string> Implicit;
        std::optional<std::string> Inherited;
        std::optional<std::string> InitialRsrcConsumerGroup;
        std::optional<TimestampType> LastLogin;
        std::optional<std::string> LocalTempTablespace;
        std::optional<DateType> LockDate;
        std::optional<std::string> OracleMaintained;
        //std::optional<std::string> Password;
        std::optional<DateType> PasswordChangeDate;
        std::optional<std::string> PasswordVersions;
        std::string Profile;
        std::optional<std::string> ProxyOnlyConnect;
        std::string TemporaryTablespace;
        std::string Username;
        NumberType UserId;

        AWL_REFLECT(
            formerRowId,
            AccountStatus,
            AllShard,
            AuthenticationType,
            Common,
            Created,
            DefaultCollation,
            DefaultTablespace,
            EditionsEnabled,
            ExpiryDate,
            ExternalName,
            Implicit,
            Inherited,
            InitialRsrcConsumerGroup,
            LastLogin,
            LocalTempTablespace,
            LockDate,
            OracleMaintained,
            //Password,
            PasswordChangeDate,
            PasswordVersions,
            Profile,
            ProxyOnlyConnect,
            TemporaryTablespace,
            Username,
            UserId);
    };
}

namespace
{
    template <class User>
    void CreateTableRecursiveTest(const awl::testing::TestContext & context, const char * name, bool header)
    {
        DbContainer c(context);
        Database & db = c.db();

        TableBuilder<User> builder(name, true);

        builder.AddColumns();

        if (header)
        {
            builder.AddLine("PRIMARY KEY (Header_BeginSit, Header_EndSit)");
        }
        else
        {
            builder.AddLine("PRIMARY KEY (BeginSit, EndSit)");
        }

        const std::string query = builder.Build();

        context.out << awl::FromAString(query);

        db.Exec(query);
    }
}

AWL_TEST(DatabaseTest)
{
    AWL_UNUSED_CONTEXT;
    
    DbContainer c(context);

    c.FillDatabase();

    Database & db = c.db();

    db.Exec("select * from myTable");

    db.Exec("delete from myTable");

    db.Exec("drop table myTable");
}

AWL_TEST(SimpleQueryTest)
{
    AWL_UNUSED_CONTEXT;

    DbContainer c(context);

    c.FillDatabase();

    Database & db = c.db();

    Statement rs(db, "select * from myTable");

    size_t count = 0;

    while (rs.Next())
    {
        ++count;
    }

    AWL_ASSERT_EQUAL(c.m_ages.size(), count);
}

AWL_TEST(WhereTest)
{
    AWL_UNUSED_CONTEXT;

    DbContainer c(context);

    c.FillDatabase();

    Database & db = c.db();

    Statement rs(db, "select FirstName, Age from myTable where Age <= ?");

    std::sort(c.m_ages.begin(), c.m_ages.end());

    size_t i = c.m_ages.size() / 2;

    const size_t val = c.m_ages[i];

    while (i < c.m_ages.size() - 1 && c.m_ages[i + 1] == val)
    {
        ++i;
    }

    Bind(rs, 0, val);

    size_t count = 0;

    while (rs.Next())
    {
        AWL_ASSERT(rs.IsText(0));
        AWL_ASSERT_FALSE(rs.IsNull(0));

        AWL_ASSERT(rs.IsInt(1));
        AWL_ASSERT_FALSE(rs.IsNull(1));

        std::string firstName;
        sqlite::Get(rs, 0, firstName);

        size_t age;
        sqlite::Get(rs, 1, age);

        AWL_ASSERT_EQUAL(c.m_ages[count], age);

        ++count;
    }

    AWL_ASSERT_EQUAL(i + 1, count);
}

AWL_TEST(CreateTableTestWithRowId)
{
    DbContainer c(context);
    Database & db = c.db();

    TableBuilder<DbaUser2> builder("DbaUser", true);

    builder.SetColumnConstraint(&DbaUser2::UserId, "PRIMARY KEY AUTOINCREMENT");
    builder.SetColumnConstraint(&DbaUser2::ExternalName, "UNIQUE");
    builder.SetColumnConstraint(&DbaUser2::AccountStatus, "DEFAULT 'ok'");

    const std::string query = builder.Create();

    context.out << awl::FromAString(query);

    db.Exec(query);
}

AWL_TEST(CreateTableWithoutRowIdTest)
{
    DbContainer c(context);
    Database & db = c.db();

    TableBuilder<DbaUser> builder("DbaUser");

    builder.SetColumnConstraint(&DbaUser::UserId, "NOT NULL PRIMARY KEY");

    const std::string query = builder.Create();

    context.out << awl::FromAString(query);

    db.Exec(query);
}

AWL_TEST(CreateTableWithMulticolumnPKTest)
{
    DbContainer c(context);
    Database & db = c.db();

    TableBuilder<DbaUser> builder("DbaUser");

    builder.SetPrimaryKey(&DbaUser::UserId, &DbaUser::TemporaryTablespace);

    const std::string query = builder.Create();

    context.out << awl::FromAString(query);

    db.Exec(query);
}

AWL_TEST(CreateTableRecursiveTest)
{
    CreateTableRecursiveTest<UniqueUser>(context, "UniqueUser", true);
    CreateTableRecursiveTest<UniqueUser1>(context, "UniqueUser1", false);
}
