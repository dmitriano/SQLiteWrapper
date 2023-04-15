#include "DbContainer.h"
#include "SQLiteWrapper/Bind.h"
#include "SQLiteWrapper/Get.h"

#include "ModelTypes.h"

using namespace swtest;

namespace
{
    struct CommonHeader
    {
        int64_t BeginSit;
        int64_t EndSit;

        AWL_STRINGIZABLE(BeginSit, EndSit);
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

        AWL_STRINGIZABLE(
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

        AWL_STRINGIZABLE(Header, User);
    };

    struct Other
    {
        int Something;

        AWL_STRINGIZABLE(Something);
    };

    struct UniqueUser1
    {
        Other It;

        int64_t BeginSit;
        int64_t EndSit;

        DbaUser User;

        AWL_STRINGIZABLE(It, BeginSit, EndSit, User);
    };

    static_assert(sqlite::helpers::GetFieldCount<DbaUser>() == 24);
    static_assert(sqlite::helpers::GetFieldCount<CommonHeader>() == 2);
    static_assert(sqlite::helpers::GetFieldCount<UniqueUser>() == 24 + 2);

    struct DbaUser2
    {
        int rowId;
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

        AWL_STRINGIZABLE(
            rowId,
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
        DbContainer c;
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

AWT_TEST(DatabaseTest)
{
    AWT_UNUSED_CONTEXT;
    
    DbContainer c;

    c.FillDatabase();

    Database & db = c.db();

    db.Exec("select * from myTable");

    db.Exec("delete from myTable");

    db.Exec("drop table myTable");
}

AWT_TEST(SimpleQueryTest)
{
    AWT_UNUSED_CONTEXT;

    DbContainer c;

    c.FillDatabase();

    Database & db = c.db();

    Statement rs(db, "select * from myTable");

    size_t count = 0;

    while (rs.Next())
    {
        ++count;
    }

    AWT_ASSERT_EQUAL(c.m_ages.size(), count);
}

AWT_TEST(WhereTest)
{
    AWT_UNUSED_CONTEXT;

    DbContainer c;

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
        AWT_ASSERT(rs.IsText(0));
        AWT_ASSERT_FALSE(rs.IsNull(0));

        AWT_ASSERT(rs.IsInteger(1));
        AWT_ASSERT_FALSE(rs.IsNull(1));

        std::string firstName;
        sqlite::Get(rs, 0, firstName);

        size_t age;
        sqlite::Get(rs, 1, age);

        AWT_ASSERT_EQUAL(c.m_ages[count], age);

        ++count;
    }

    AWT_ASSERT_EQUAL(i + 1, count);
}

AWT_TEST(CreateTableTestWithRowId)
{
    DbContainer c;
    Database & db = c.db();

    TableBuilder<DbaUser2> builder("DbaUser", true);

    builder.SetColumnConstraint(&DbaUser2::UserId, "PRIMARY KEY AUTOINCREMENT");
    builder.SetColumnConstraint(&DbaUser2::ExternalName, "UNIQUE");
    builder.SetColumnConstraint(&DbaUser2::AccountStatus, "DEFAULT 'ok'");

    const std::string query = builder.Create();

    context.out << awl::FromAString(query);

    db.Exec(query);
}

AWT_TEST(CreateTableWithoutRowIdTest)
{
    DbContainer c;
    Database & db = c.db();

    TableBuilder<DbaUser> builder("DbaUser");

    builder.SetColumnConstraint(&DbaUser::UserId, "NOT NULL PRIMARY KEY");

    const std::string query = builder.Create();

    context.out << awl::FromAString(query);

    db.Exec(query);
}

AWT_TEST(CreateTableWithMulticolumnPKTest)
{
    DbContainer c;
    Database & db = c.db();

    TableBuilder<DbaUser> builder("DbaUser");

    builder.SetPrimaryKey(&DbaUser::UserId, &DbaUser::TemporaryTablespace);

    const std::string query = builder.Create();

    context.out << awl::FromAString(query);

    db.Exec(query);
}

AWT_TEST(CreateTableRecursiveTest)
{
    CreateTableRecursiveTest<UniqueUser>(context, "UniqueUser", true);
    CreateTableRecursiveTest<UniqueUser1>(context, "UniqueUser1", false);
}
