#include "worker.h"
#include "song.h"
#include "common.h"
#include "database_manager.h"
#include <QFileInfo>

Worker::Worker(QObject *parent) : QObject(parent)
{
    LOG << "\n";
}

Worker::~Worker()
{
    LOG << "\n";
}

/*解析歌曲信息槽函数，参数接收mp3文件路径
 * 从mp3文件所在目录查找同名lrc歌词文件，读取解析
 * 构造一个歌曲对象，循环按行读取文件数据，解析并存储信息：
 *  1.解析歌名、歌手、专辑
 *  2.解析歌词
 * 有的歌词文件可能没有信息，简单地使用mp3Url文件名来设置歌曲名
 * 返回时存储到消息队列，发送解析结束信号

光辉岁月.lrc歌词文件内部数据：
[ti:光辉岁月 (国语)]
[ar:BEYOND]
[al:光辉岁月]
[by:]
[offset:0]
[00:00.00]光辉岁月 - BEYOND
[00:07.07]词：周治平/何启弘
[00:14.14]曲：黄家驹
[00:21.21]编曲：Beyond
[00:28.28]一生要走多远的路程
[00:31.42]
[00:32.75]经过多少年
[00:34.89]
[00:35.99]才能走到终点
*/
void Worker::getASong(const QUrl& mp3Url)
{
    if (!QFileInfo(mp3Url.path()).isFile())
    {
        LOG << "不可用的mp3文件路径：" << mp3Url.path() << "\n";
        return;
    }

    LOG << "构造一个歌曲对象" << "\n";
    Song * song = new Song(mp3Url, QFileInfo(mp3Url.path()).baseName(), QString(), QString());

    LOG << "将路径后缀.mp3替换为.lrc，然后判断歌词文件是否存在," << "\n";
    QString lrcFile = mp3Url.path().replace(".mp3", ".lrc");
    bool ret = QFileInfo(lrcFile).isFile();
    if (!ret)
    {
        LOG << "歌词文件不存在: " << lrcFile << "\n";
        LOG << "生成歌曲：" << *song << "\n";
        MessageQueue::getInstance().push(song); //存储到消息队列
        emit getASongFinished(); //todo: 主窗口接收信号并从消息队列中读取歌曲
        return;
    }

    LOG << "读取歌词文件：" << lrcFile << "\n";
    QFile qfile(lrcFile);
    ret = qfile.open(QIODevice::ReadOnly | QIODevice::Text); //只读模式加文本模式打开文件
    if (!ret)
    {
        LOG << "歌词文件打开失败：" << lrcFile << "\n";
        LOG << "生成歌曲：" << *song << "\n";
        MessageQueue::getInstance().push(song);
        emit getASongFinished();
        return;
    }

    LOG << "按行读取歌词文件，解析并存储歌名等信息" << "\n";
    //使用QTextStream按行遍历文件，读取每行内容
    QTextStream stream(&qfile);
    QString line;   //存储一行数据
    QString text;

    while (!stream.atEnd()) //循环读取，直到文件末尾结束
    {
        line = stream.readLine(); //读取一行数据, 如果中文乱码，考虑用数据库课件案例中的utf8ToGbk/gbkToUtf8转换
        LOG << "line: " << line << "\n";
        if (line.isEmpty()) { continue; }

        //解析歌名、歌手、专辑
        if (line.startsWith("[ti:")) //如果以[ti:开头说明后面是歌曲名
        {
            text = line.mid(4); //截取偏移量4开始的子串
            text.chop(1);   //去掉最后的']'
            song->name(text); //存储到歌曲中
            LOG << "读取并存储歌名：" << text << "\n";
        }
        else if (line.startsWith("[ar:"))
        {
            text = line.mid(4);
            text.chop(1);
            song->artist(text);
            LOG << "读取并存储歌手：" << text << "\n";
        }
        else if (line.startsWith("[al:"))
        {
            text = line.mid(4);
            text.chop(1);
            song->album(text);
            LOG << "读取并存储专辑：" << text << "\n";
        }
    }

    qfile.close();

    //歌词解析，单独封装函数解析
    readLyrics(song);

    LOG << "歌曲解析结束, 生成歌曲：" << *song << "\n";
    MessageQueue::getInstance().push(song);
    emit getASongFinished();
}

/*
 * 解析歌词函数，参数接收歌曲对象指针，
 *      根据歌曲url读取歌词文件，
 *      解析歌词信息存储到歌曲对象中

    歌词解析规则：
    按行读取，形如: [00:28.28]一生要走多远的路程
    1）根据']'第一次分割行内容，得：
        A [分钟数:秒数.毫秒数
        B 歌曲信息或歌词文本

    2）根据':'第二次分割，分割A得：
        A1 [分钟数
        A2 秒数.毫秒数

    3）A1去掉'['，转换成整型的分钟数
    4）A2转换成浮点数类型的秒数
    5）(A1 * 60 + A2) * 1000 = 这一行对应的播放时间进度（毫秒级）
    6）B文本可以直接存储和使用
*/
void Worker::readLyrics(Song * song)
{
    if (!song || song->url().isEmpty())
    {
        LOG << "歌曲不存在，或者歌曲文件路径为空" << "\n";
        return;
    }

    QUrl mp3Url = song->url();
    LOG << "将路径后缀.mp3替换为.lrc，然后判断歌词文件是否存在," << "\n";
    QString lrcFile = mp3Url.path().replace(".mp3", ".lrc");
    bool ret = QFileInfo(lrcFile).isFile();
    if (!ret)
    {
        LOG << "歌词文件不存在: " << lrcFile << "\n";
        return;
    }

    LOG << "读取歌词文件：" << lrcFile << "\n";
    QFile qfile(lrcFile);
    ret = qfile.open(QIODevice::ReadOnly | QIODevice::Text); //只读模式加文本模式打开文件
    if (!ret)
    {
        LOG << "歌词文件打开失败：" << lrcFile << "\n";
        return;
    }

    //使用QTextStream按行遍历文件，读取每行内容
    QTextStream stream(&qfile);
    QString line;   //存储一行数据
    QString text;
    QStringList lineContents;   //歌词行分割后的内容数组 {时间戳, 歌词文本}
    QStringList timeContents;   //时间戳分割后的内容数组 {分钟数, 秒数.毫秒数}
    QMap<qint64, QString> lyrics;   //局部歌词容器，临时存储歌词

    while (!stream.atEnd()) //循环读取，直到文件末尾结束
    {
        line = stream.readLine(); //读取一行数据, 如果中文乱码，考虑用数据库课件案例中的utf8ToGbk/gbkToUtf8转换
        LOG << "line: " << line << "\n";
        if (line.isEmpty()) { continue; }

        //解析歌词信息
        lineContents = line.split(']'); // { [时间戳, 信息文本 }
        timeContents = lineContents[0].split(':'); //{ [分钟数, 秒数.毫秒数 }

        //排除非歌词的信息行，将歌词行中的"[分钟数"转换成整型的数值,如果转换失败说明不是歌词行
        bool ok = false;    //获取转换是否成功
        QString text = timeContents[0].mid(1);
        int minutes = text.toInt(&ok); //字符串转数值，传入参数接收是否转成功
        if (!ok) { continue; }

        double seconds = timeContents[1].toDouble(); //字符串转浮点数
        //存储到歌词容器中
        lyrics.insert((minutes * 60 + seconds) * 1000, lineContents[1]);
        LOG << "insert lyric: " << (minutes * 60 + seconds) * 1000 << " " << lineContents[1] << "\n";
    }

    qfile.close(); //用完关闭文件
    LOG << "解析歌词结束，歌词行数：" << lyrics.size() << "\n";
    song->lyrics(lyrics);   //设置到歌曲中
}

//处理存储歌曲信号的槽函数，持久化存储到数据库
void Worker::handle_saveSong(Song* song)
{
    LOG << "\n";
    if (song == nullptr) { return; }

    if (!DatabaseManager::getInstance().insertSong(*song))
    {
        LOG << "持久化存储歌曲失败：" << *song << "\n";
        return;
    }

    LOG << "持久化存储歌曲成功：" << *song << "\n";
}

//定义一个查询歌曲槽函数，从数据库查询，查询后存储到消息队列，并通知主窗口
void Worker::handle_querySong()
{
    QVector< Song* > songs;
    if (false == DatabaseManager::getInstance().querySongs(songs))
    {
        LOG << "查询歌曲失败" << "\n";
        return;
    }

    LOG << "查询到的歌曲数量：" << songs.size() << "\n";
    for (auto song : songs)
    {
        LOG << "查询到歌曲，读取歌词并存储到消息队列：" << song->url().path() << "\n";
        readLyrics(song);
        MessageQueue::getInstance().push(song);
    }

    LOG << "通知主窗口查询结束" << "\n";
    emit getASongFinished(true);
}
