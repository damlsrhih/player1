#include "log.h"

Log2File::Log2File()
{
    //年year 月Month 日day 时Hour 分minute 秒second 周几ddd
    qDebug() << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss ddd");

    //QString fileName = "log_" + QDateTime::currentDateTime().toString("yyyyMMdd") + ".txt";
    QString fileName = QString("log_%1.txt").arg(QDateTime::currentDateTime().toString("yyyyMMdd"));
    qDebug() << "fileName: " << fileName;

    //设置文件名，然后以追加和文本模式打开
    m_file.setFileName(fileName);
    if (false == m_file.open(QIODevice::Append | QIODevice::Text))
    {
        qDebug() << "open log file failed: " << fileName;
        return;
    }

    qDebug() << "open log file succeed: " << fileName;

    //用流对象管理文件对象，用来后续写入日志
    m_stream.setDevice(&m_file);
    //m_stream.setCodec("utf-8");

    //test();
}

Log2File::~Log2File()
{
    m_stream << flush;  //刷新缓冲区中的数据到文件中
    m_stream.reset();   //重置流对象
    m_file.close();     //关闭文件对象
}


Log2File& Log2File::instance()
{
    static Log2File instance;
    return instance;
}

//测试输出
void Log2File::test()
{
    m_stream << "test1 log to file\n";
    m_stream << "test2 打印日志到文件\n";
    m_stream << QString("test3 打印日志到文件，转QString") << "\n";

    m_stream << "test4 time info: "
             << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss ddd")
             << "\n";

    m_stream << "test5 code info: "
             << __FILE__ << " "
             << __BASE_FILE__ << " "
             << __LINE__ << " "
             << __PRETTY_FUNCTION__ << " "
             << "\n";

    m_stream << flush; //刷新缓冲区中的数据到文件中。
}

//todo: 可以考虑额外加个换行符
LogStream& operator<<(LogStream& lstream, const QString & data)
{
    Log2File::instance().stream() << data;
    return lstream;
}

LogStream& operator<<(LogStream& lstream, const std::string & data)
{
    Log2File::instance().stream() << QString::fromStdString(data);
    return lstream;
}

LogStream& operator<<(LogStream& lstream, const char * data)
{
    Log2File::instance().stream() << QString(data); //QString::fromLocal8Bit(data);
    return lstream;
}

LogStream& operator<<(LogStream& lstream, int data)
{
    Log2File::instance().stream() << QString::number(data);
    return lstream;
}

LogStream& operator<<(LogStream& lstream, double data)
{
    Log2File::instance().stream() << QString::number(data);
    return lstream;
}

LogStream& operator<<(LogStream& lstream, const Song& song)
{
    Log2File::instance().stream() << " " << song.name()
                                  << " " << song.artist()
                                  << " " << song.album()
                                  << " 歌词行数: " << song.lyrics().size()
                                  << " " << song.url().path()
                                  << "\n";
    return lstream;
}

LogStream& operator<<(LogStream& lstream, qint64 data)
{
    Log2File::instance().stream() << QString::number(data);
    return lstream;
}
