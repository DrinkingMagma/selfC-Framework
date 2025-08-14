#include "_ooci.h"
using namespace idc;

struct ST_User
{
    long id;
    char name[31];
    double weight;
    char birthday[20];
    char notes[2001];
} st_user;


void creat_table(SqlStatement &stmt);
void select_table(SqlStatement &stmt);
void insert_table(SqlStatement &stmt, ST_User &user);
void delete_table(SqlStatement &stmt, long id);
void save_file_to_clob(SqlStatement &stmt);
void save_file_to_blob(SqlStatement &stmt);
void save_clob_to_file(SqlStatement &stmt);
void save_blob_to_file(SqlStatement &stmt);
void EXIT(int sig);

int main(int argc,char *argv[])
{
    // 创建数据库连接类对象
    Connection conn;

    // 登录数据库
    if(conn.connecttodb("lzy/lzyou1011@ORCL", "Simplified Chinese_China.AL32UTF8") != 0)
    {
        printf("connect database failed: %s\n", conn.message());
        return -1;
    }
    printf("connect database success.\n");

    // 创建SQL语句执行类对象
    SqlStatement stmt;
    // 指定stmt对象使用的数据库连接
    stmt.connect(&conn);

    // creat_table(stmt);
    
    // select_table(stmt);

    // ST_User user;
    // memset(&user, 0, sizeof(user));
    // user.id = 1002;
    // strcpy(user.name, "张三");
    // user.weight = 75.5;
    // strcpy(user.birthday, "1990-05-20 10:30:00");
    // strcpy(user.notes, "这是一个测试用户，用于演示插入功能。");

    // insert_table(stmt, user);

    // delete_table(stmt, 1001);
    // save_file_to_clob(stmt);
    // save_file_to_blob(stmt);
    save_clob_to_file(stmt);
    save_blob_to_file(stmt);
    
    select_table(stmt);
    
    // 手动提交事务
    conn.commit();

    return 0;
}


void creat_table(SqlStatement &stmt)
{
    stmt.prepare("\
        CREATE TABLE users(\
            id NUMBER(10) PRIMARY KEY,\
            name VARCHAR2(30),\
            weight NUMBER(8,2),\
            birthday DATE,\
            notes VARCHAR2(2000),\
            picture BLOB)\
    ");

    if(stmt.execute() != 0)
    {
        printf("create table failed: %s\n", stmt.message());
        EXIT(-1);
    }
    printf("create table succeed.\n");
}

void select_table(SqlStatement &stmt)
{
    stmt.prepare("\
        SELECT id, name, weight, to_char(birthday, 'yyyy-mm-dd hh24:mi:ss'), notes\
        FROM users\
    ");

    stmt.bindout(1, st_user.id);
    stmt.bindout(2, st_user.name);
    stmt.bindout(3, st_user.weight);
    stmt.bindout(4, st_user.birthday);
    stmt.bindout(5, st_user.notes, 2000);

    if(stmt.execute() != 0)
    {
        printf("select table failed: %s\n", stmt.message());
        EXIT(-1);
    }

    while (true)
    {
        memset(&st_user, 0, sizeof(st_user));

        // 返回0表示成功获取下一条数据，返回1403表示无记录，返回其他值表示失败
        if(stmt.next() != 0)
            break;

        printf("%ld, %s, %f, %s, %s\n", st_user.id, st_user.name, st_user.weight, st_user.birthday, st_user.notes);
    }

    printf("current select users rows: %ld\n", stmt.rpc());
}

void insert_table(SqlStatement &stmt, ST_User &user)
{
    // 准备插入语句（使用参数绑定）
    // empty_clob()和empty_blob()表示空的CLOB和BLOB对象，只有先存在空的对象才能进行绑定和更新
    stmt.prepare(
        "INSERT INTO users(id, name, weight, birthday, notes, text, picture) VALUES(:1, :2, :3, TO_DATE(:4, 'yyyy-mm-dd hh24:mi:ss'), :5, empty_clob(), empty_blob())"
    );

    // 绑定输入参数
    stmt.bindin(1, user.id);                     // 绑定 id
    stmt.bindin(2, user.name, 30);               // 绑定 name，长度30
    stmt.bindin(3, user.weight);                 // 绑定 weight
    stmt.bindin(4, user.birthday, 19);           // 绑定 birthday，格式 'yyyy-mm-dd hh24:mi:ss'，共19字符
    stmt.bindin(5, user.notes, 2000);            // 绑定 notes，最大2000字符

    // 执行插入
    if (stmt.execute() != 0)
    {
        printf("insert failed: %s\n", stmt.message());
        EXIT(-1);
    }

    printf("insert success: id=%ld, name=%s\n", user.id, user.name);
}

void delete_table(SqlStatement &stmt, long id)
{
    // 准备 DELETE 语句，使用绑定变量 :1
    stmt.prepare("DELETE FROM users WHERE id = :1");

    // 绑定输入参数 :1 为 id
    stmt.bindin(1, id);

    // 执行删除操作
    if (stmt.execute() != 0)
    {
        printf("delete failed: %s\n", stmt.message());
        EXIT(-1);
    }

    // 获取影响的行数
    long affected_rows = stmt.rpc();  // rpc() 返回本次操作影响的行数

    if (affected_rows == 0)
    {
        printf("delete warning: no record found with id=%ld\n", id);
    }
    else
    {
        printf("delete success: deleted %ld row(s) where id=%ld\n", affected_rows, id);
    }
}


void save_file_to_clob(SqlStatement &stmt)
{
    ST_User user;
    memset(&user, 0, sizeof(user));
    user.id = 1003;
    strcpy(user.name, "张三");
    user.weight = 75.5;
    strcpy(user.birthday, "1990-05-20 10:30:00");
    strcpy(user.notes, "这是一个测试用户，用于演示插入功能。");
    insert_table(stmt, user);

    // for update：加行锁，防止其他会话修改，同时运行我们更新CLOB
    stmt.prepare("select text from users where id = :1 for update");
    stmt.bindin(1, user.id);
    stmt.bindclob();

    if(stmt.execute() != 0)
    {
        printf("%s failed: %s\n", stmt.sql(), stmt.message());
        EXIT(-1);
    }
    if(stmt.next() != 0)
        EXIT(-1);

    if(stmt.filetolob("./text_in.txt") != 0)
    {
        printf("stmt.filetolob() failed: %s\n", stmt.message());
        EXIT(-1);
    }
    printf("file saved to clob\n");
}

void save_file_to_blob(SqlStatement &stmt)
{
    ST_User user;
    memset(&user, 0, sizeof(user));
    user.id = 1004;
    strcpy(user.name, "张三");
    user.weight = 75.5;
    strcpy(user.birthday, "1990-05-20 10:30:00");
    strcpy(user.notes, "这是一个测试用户，用于演示插入功能。");
    insert_table(stmt, user);

    // for update：加行锁，防止其他会话修改，同时运行我们更新CLOB
    stmt.prepare("select picture from users where id = :1 for update");
    stmt.bindin(1, user.id);
    stmt.bindblob();

    if(stmt.execute() != 0)
    {
        printf("%s failed: %s\n", stmt.sql(), stmt.message());
        EXIT(-1);
    }
    if(stmt.next() != 0)
        EXIT(-1);

    if(stmt.filetolob("./picture_in.jpeg") != 0)
    {
        printf("stmt.filetolob() failed: %s\n", stmt.message());
        EXIT(-1);
    }
    printf("file saved to blob\n");
}


void save_clob_to_file(SqlStatement &stmt)
{
    stmt.prepare("select text from users where id = 1003");
    stmt.bindclob();

    if(stmt.execute() != 0)
    {
        printf("stmt.execute(\"%s\") failed: %s\n", stmt.sql(), stmt.message());
        EXIT(-1);
    }

    if(stmt.next() != 0)
        EXIT(-1);

    if(stmt.lobtofile("./text_out.txt") != 0)
    {
        printf("stmt.lobtofile(\"%s\") failed: %s\n", "text_out.txt", stmt.message());
        EXIT(-1);
    }
    printf("stmt.lobtofile(\"%s\") ok.\n", "text_out.txt");
}

void save_blob_to_file(SqlStatement &stmt)
{
    stmt.prepare("select picture from users where id = 1004");
    stmt.bindblob();

    if(stmt.execute() != 0)
    {
        printf("stmt.execute(\"%s\") failed: %s\n", stmt.sql(), stmt.message());
        EXIT(-1);
    }

    if(stmt.next() != 0)
        EXIT(-1);

    if(stmt.lobtofile("./picture_out.jpeg") != 0)
    {
        printf("stmt.lobtofile(\"%s\") failed: %s\n", "picture_out.jpeg", stmt.message());
        EXIT(-1);
    }
    printf("stmt.lobtofile(\"%s\") ok.\n", "picture_out.jpeg");
}

void EXIT(int sig)
{
    printf("signal = %d, Program Exit...\n", sig);
    exit(0);
}