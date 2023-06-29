#include "song.h"
#include <QDebug>

Song::Song()
{

}

//重载输出Song类对象的输出运算符函数，输出流类型使用QDebug&
//头文件声明友元，源文件里定义函数
QDebug& operator<<(QDebug& debug, const Song& song)
{
    debug << song.m_name << ", "
          << song.m_artist << ", "
          << song.m_album << ", "
          << "歌词行数: " << song.m_lyrics.size();
    return debug;
}

//输出到文件，使用QTextStream
QTextStream& operator<<(QTextStream& stream, const Song& song)
{
    stream << song.m_name << ", "
           << song.m_artist << ", "
           << song.m_album << ", "
           << "歌词行数: " << song.m_lyrics.size();
    return stream;
}
