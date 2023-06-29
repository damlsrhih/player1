#ifndef DATABASE_MANAGER_H
#define DATABASE_MANAGER_H

/*使用Qt的数据库模块sql
 *  配置添加sql模块: QT += sql
 *  包含sql模块中相关头文件
 *      sqlite可以直接使用
 *
 * sql模块使用sqlite
 * 1.添加一条数据库连接对象
 * 2.判断连接是否可用
 * 3.针对sqlite指定数据库名
 * 4.打开数据库
 * 5.1 打开失败
 *  移除数据库连接
 * 5.2 打开成功
 *  5.2.1 数据库相关语句操作
 *  5.2.2 关闭数据库
 *  5.2.3 移除数据库连接
 *
 *数据库管理类，负责数据库的初始化、增删改查等操作
 * 实现为单例类
 * 初始化：打开数据库player.db, 创建歌曲表songs
 * 增删改查
*/
#include <QSqlDatabase>
#include <QVector>
#include "song.h"
class DatabaseManager
{
private:
    DatabaseManager();
    DatabaseManager(const DatabaseManager& other);
    ~DatabaseManager();

public:
    static DatabaseManager& getInstance();

private:
    //数据库连接对象
    QSqlDatabase m_connection;

    //声明相关信息
    QString m_databaseType = "QSQLITE"; //数据库类型
    QString m_connectionName = "player_database"; //数据库连接名
    QString m_databaseName = "player.db";   //数据库名
    QString m_databaseTableName = "songs";  //歌曲表名称

    //建库建表等内部初始操作
    bool init();        //驱动是否可用、添加数据库连接、设置数据库名
    bool initTable();   //建表

public:
    //定义用户所需相关接口，
    //  打开和关闭数据库的接口，
    //  执行接口
    //      存储接口，存储歌曲信息，参数接收歌曲对象
    //      查询歌曲接口，参数数组存储查询到的歌曲
    bool open();
    void close();

    bool insertSong(const Song& song);
    bool querySongs(QVector< Song* >& result);
};

#endif // DATABASE_MANAGER_H
