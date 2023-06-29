#include "database_manager.h"
#include "common.h"
#include <QSqlQuery>
#include <QSqlError>

DatabaseManager::DatabaseManager()
{
    LOG << "\n";
    init();
}

DatabaseManager::DatabaseManager(const DatabaseManager& other)
{
    LOG << "\n";
}

//移除连接
DatabaseManager::~DatabaseManager()
{
    LOG << "\n";
    close();
    QSqlDatabase::removeDatabase(m_connection.connectionName());
}

DatabaseManager& DatabaseManager::getInstance()
{
    static DatabaseManager instance;
    return instance;
}

//建库等内部初始化
bool DatabaseManager::init()
{
    LOG << "\n";
    LOG << "添加连接" << "\n";
    m_connection = QSqlDatabase::addDatabase(m_databaseType, m_connectionName);
    if (!m_connection.isValid())
    {
        LOG << "数据库类型和连接不可用：" << m_databaseType << " " << m_connectionName << "\n";
        return false;
    }

    LOG << "成功创建连接：" << m_databaseType << " " << m_connectionName << "\n";
    LOG << "设置数据库名: " << m_databaseName << "\n";
    m_connection.setDatabaseName(m_databaseName);

    LOG << "打开数据库" << "\n";
    if (!open())
    {
        LOG << "打开数据库失败" << "\n";
        return false;
    }

    LOG << "打开数据库成功，建表" << "\n";
    return initTable();
}

//建表
bool DatabaseManager::initTable()
{
    LOG << "\n";
    bool ret = m_connection.isOpen();
    if (!ret)
    {
        LOG << "数据库未打开" << "\n";
        return ret;
    }

    QString sql = "create table if not exists ";
    sql += m_databaseTableName;
    sql += "(id integer primary key autoincrement, "
           "url text unique not null,"
           "name text,"
           "artist text,"
           "album text);";

    QSqlQuery query(m_connection);
    ret = query.exec(sql);
    if (!ret)
    {
        LOG << "执行数据库语句失败，错误信息：" << query.lastError().text() << "\n";
        return ret;
    }

    LOG << "建表成功" << "\n";
    return true;
}

//定义用户所需相关接口，打开和关闭数据库的接口，执行接口（定义一个查询歌曲接口，参数存储查询到的歌曲）
bool DatabaseManager::open()
{
    LOG << "\n";
    m_connection.open();
    if (!m_connection.isOpen())
    {
        LOG << "打开失败，错误信息：" << m_connection.lastError().text() << "\n";
        return false;
    }

    LOG << "打开成功" << "\n";
    return true;
}

void DatabaseManager::close()
{
    LOG << "关闭数据库" << "\n";
    m_connection.close();
}

//新增歌曲，参数接收歌曲对象
bool DatabaseManager::insertSong(const Song& song)
{
    bool ret = m_connection.isOpen();
    if (!ret)
    {
        LOG << "数据库未打开" << "\n";
        return ret;
    }

    LOG << "新增歌曲到数据库" << "\n";
    QString sql = "insert into ";
    sql += m_databaseTableName;
    sql += "(url, name, artist, album) values(";
    sql += "'" + song.url().path() + "'"; //加个单引号包含字符串变量，防止为空影响语句执行
    sql += ", '" + song.name() + "'";
    sql += ", '" + song.artist() + "'";
    sql += ", '" + song.album() + "'";
    sql += ");";

    QSqlQuery query(m_connection);
    ret = query.exec(sql);
    if (!ret)
    {
        LOG << "执行语句失败：" << sql << ", 错误信息：" << m_connection.lastError().text() << "\n";
        return false;
    }

    LOG << "新增语句执行成功" << "\n";
    return true;
}

//查询歌曲，参数存储歌曲对象
bool DatabaseManager::querySongs(QVector< Song* >& result)
{
    LOG << "查询所有歌曲的接口" << "\n";
    bool ret = m_connection.isOpen();
    if (!ret)
    {
        LOG << "数据库未打开" << "\n";
        return ret;
    }

    QString sql = "select * from songs;";
    QSqlQuery query(m_connection);
    ret = query.exec(sql);
    if (!ret)
    {
        LOG << "执行语句失败：" << sql << ", 错误信息：" << m_connection.lastError().text() << "\n";
        return ret;
    }

    LOG << "查询成功，解析结果，存储到参数里" << "\n";
    QUrl url;
    QString name, artist, album;
    //循环检索查询到的结果集合，next获取一条数据，再用query.value读取
    while (query.next())
    {
        url = QUrl(query.value("url").toString());
        name = query.value("name").toString();
        artist = query.value("artist").toString();
        album = query.value("album").toString();

        LOG << "读取一首歌曲：" << url.path() << " " << name << " " << artist << " " << album << "\n";
        result.push_back(new Song(url, name, artist, album));
    }

    return true;
}
