#pragma once

/* 2023.06.10 定义日志输出类，单例类，输出日志信息到文件
 *  文件类QFile打开文件、文本流类QTextStream写入文件
 *      针对自定义类型的写入，考虑重载输出运算符，处理QTextStream和自定义类型
 *      打印时间信息，使用QDateTime类
 * ----- update log 2023.06.11 -----
 * 自定义日志流类，重载输出运算符，处理日志流类和所输出的数据
 */
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QDebug>
#include "song.h"

//日志类，打印日志到文件
class Log2File //LogToFile
{
private:
    Log2File();

    ~Log2File();

private:
    QFile m_file;           //打开日志文件的文件对象
    QTextStream m_stream;   //向日志文件写入日志信息的流对象

public:
    static Log2File& instance();

    //返回一个流对象引用的接口，外部用来写入日志
    QTextStream& stream() { return m_stream; }

    //测试输出
    void test();
};

//日志流类，接收日志数据，调用日志类输出
//  重载输出运算符，返回本类型的引用以继续输出
class LogStream
{
private:
    LogStream() {}
    LogStream(const LogStream& other) {}
    ~LogStream() {}

public:
    static LogStream& instance()
    {
        static LogStream instance;
        return instance;
    }

    friend LogStream& operator<<(LogStream& lstream, const QString & data);
    friend LogStream& operator<<(LogStream& lstream, const std::string & data);
    friend LogStream& operator<<(LogStream& lstream, const char * data);
    friend LogStream& operator<<(LogStream& lstream, int data);
    friend LogStream& operator<<(LogStream& lstream, double data);
    friend LogStream& operator<<(LogStream& lstream, const Song& song);
    friend LogStream& operator<<(LogStream& lstream, qint64 data);

    void test()
    {
        qDebug() << "LogStream test";
        *this << QString("QString 中文测试") << "\n";
        *this << std::string("std string 中文测试") << "\n";
        *this << "const char* 中文测试" << "\n";
        *this << 20230611 << "\n";
        *this << 20230611.1445 << "\n";
    }
};
