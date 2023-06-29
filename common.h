#pragma once

/*
 * 2023.06.04
 * 定义宏LOG输出行号和函数声明
 *
 * ----- update log 2023.06.08 -----
 * 定义宏LOG2输出文件名、行号、函数
 *
 * ----- update log 2023.06.10 -----
 * 定义宏LOG3输出日志到文件
 *
 * ----- update log 2023.06.11 -----
 * 定义宏LOG4输出日志到文件, 替换其它宏
*/
#include <QDebug>
#include <QFileInfo>
#include "log.h"

//定义一个格式化函数__format, 将系统的文件、行号、函数等宏的信息格式化为指定格式
//文本居左、宽度50、不足位填充空格
static QString __format(QString file, int line, QString function)
{
    file = QFileInfo(file).fileName();
    function = function.mid(function.indexOf(' '), //截取的起始位置，从返回类型后截取类名和函数名
                            function.indexOf('(') - function.indexOf(' ')); //截取的个数
    //QString text = "[" + file + ":" + QString::number(line) + " " + function;
    QString text = QString("[%1:%2 %3").arg(file).arg(line, 3, 10, QChar(' ')).arg(function);
    text = text.leftJustified(50, ' '); //文本居左、宽度50、不足位填充空格
    text = text.left(50);
    return text + "]";
}

#define LOG4 ( LogStream::instance() << "[" << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss ddd ") << __FILE__ << ":" << __LINE__ << " " << __PRETTY_FUNCTION__ << "]   " )
#define LOG3 LOG4
//#define LOG2 LOG4
#define LOG LOG4

//#define LOG3 ( Log2File::instance().stream() << "[" << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss ddd ") << __FILE__ << ":" << __LINE__ << " " << __PRETTY_FUNCTION__ << "]   " )
#define LOG2 ( qDebug() << __format(__FILE__, __LINE__, __PRETTY_FUNCTION__ ) )
//#define LOG (qDebug() << "[" << __LINE__ << " " << __PRETTY_FUNCTION__ << "]" )
