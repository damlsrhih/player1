#include "widget.h"
#include "ui_widget.h"
#include "common.h"
#include "song.h"
#include "worker.h"
#include <QFileDialog>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QThread>

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);

    LOG << "\n";
    initWindow();
    initMedia();

    //testSong();
    //testWorker();

    initWorker();
    initTimer();

    resumePlaylist();

    testLog2File();
}

Widget::~Widget()
{
    LOG;

    if (m_timer) { m_timer->stop(); }

    if (m_thread)
    {
        m_thread->quit();   //退出
        m_thread->wait(5000);   //等它退出完，最多等五秒
    }
    delete ui;
}

/*
 * 添加初始化窗口函数
*/
void Widget::initWindow()
{
    LOG << "\n";
    setWindowFlag(Qt::WindowStaysOnTopHint); //窗口置顶
    setWindowTitle("mp3 player");
    setWindowIcon(QIcon(":/icons/music.png"));

    //ui->pushButton_playbackMode->setText("循环播放");
    ui->pushButton_playbackMode->setIcon(QIcon(":/icons/loop.png")); //根据图片路径构造一个图标对象，设置给控件

    ui->listWidget_playlist->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff); //隐藏垂直滚动条
    ui->listWidget_playlist->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); //隐藏水平滚动条
    ui->listWidget_playlist->setStyleSheet("background-color:transparent"); //设置背景透明
    //ui->listWidget_playlist->setFrameShape(QListWidget::NoFrame);   //设置无边框

    ui->listWidget_lyrics->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff); //隐藏垂直滚动条
    ui->listWidget_lyrics->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); //隐藏水平滚动条
    ui->listWidget_lyrics->setStyleSheet("background-color:transparent"); //设置背景透明
    //ui->listWidget_lyrics->setFrameShape(QListWidget::NoFrame);   //设置无边框

    initLayout();
}

/*
 * 初始化多媒体函数
*/
void Widget::initMedia()
{
    LOG << "\n";
    m_mediaPlayer = new QMediaPlayer(this);
    connect(m_mediaPlayer, &QMediaPlayer::stateChanged, this, &Widget::handle_mediaPlayer_stateChanged);
    connect(m_mediaPlayer, &QMediaPlayer::positionChanged, this, &Widget::handle_mediaPlayer_positionChanged);

    m_mediaPlaylist = new QMediaPlaylist(this);
    m_mediaPlaylist->setPlaybackMode(QMediaPlaylist::Loop); //设置循环播放模式

    //将媒体播放列表对象设置给媒体播放器对象，使得媒体播放器后续自动从播放列表对象中读取和播放歌曲等
    m_mediaPlayer->setPlaylist(m_mediaPlaylist);

    connect(m_mediaPlaylist, &QMediaPlaylist::currentMediaChanged, this, &Widget::handle_mediaPlaylist_currentMediaChanged);
    connect(m_mediaPlaylist, &QMediaPlaylist::playbackModeChanged, this, &Widget::handle_mediaPlaylist_playbackModeChanged);

}

/*
 * 界面添加一个添加音乐按钮，转到槽处理点击信号
 * 1.使用QFileDialog打开文件选择对话框，选择一个音乐媒体文件，目前版本使用.mp3
 * 2.如果选中，返回文件路径，设置给媒体播放器对象
 * 3.显示歌曲文件名到歌名标签
 *
 * ------ update log 2023.06.04 ------
 * 做些预防工作，对文件路径做转换处理，再使用，避免后续内部传递转换过程中不匹配
 * 修改第2步，将选择的媒体文件（按顺序）添加到媒体播放列表中
 * 取消第3步，todo：将歌名显示放到后面的切歌信号槽函数中
 *
 * 将歌曲（按顺序）添加显示到界面歌曲列表
 *  构造一个列表控件元素，设置歌名，再设置给列表控件
 *
 * ------ update log 2023.06.08 ------
 * 判断歌曲管理员中是否已保存，有的话不重复添加
 *
 * 取消设置歌曲路径给后台媒体播放列表，
 * 取消显示到界面歌曲列表控件，
 *  改由歌曲解析结束槽函数中显示和设置（切歌时也注意更新显示）
 *
 * 发送添加歌曲信号，让工作对象开始打工
 */
void Widget::on_pushButton_add_clicked()
{
    LOG << "\n";
    //参数1-父对象，参数2-窗口标题，参数3-打开路径，参数4-可选文件格式
    QStringList files = QFileDialog::getOpenFileNames(this,
                                                      "标题-添加音乐",
                                                      QDir::currentPath(),
                                                      "mp3文件(*.mp3)");
    if (files.isEmpty())
    {
        LOG << "添加失败，选择文件为空" << "\n";
        return;
    }

    //将选择的媒体文件添加到媒体播放列表中
    for (auto file : files)
    {
        file = QMediaContent(QUrl(file)).canonicalUrl().path();
//        m_mediaPlaylist->addMedia(QMediaContent(QUrl(file)));
//        QListWidgetItem * item = new QListWidgetItem(QFileInfo(file).baseName());
//        ui->listWidget_playlist->addItem(item);

        LOG << "file: " << file << "\n";
        if (SongManager::getInstance().contains(QUrl(file)))
        {
            LOG << "歌曲管理员中已存储，不重复添加歌曲：" << file << "\n";
            continue;
        }

        //发出添加歌曲信号
        emit addSong(QUrl(file));
    }
}

/*
 * 界面添加一个播放音乐按钮，转到槽处理点击信号
 *  媒体播放器的状态 { 停止状态, 播放状体, 暂停状态 }
 *      enum State { StoppedState, PlayingState, PausedState};
 *  根据播放状态切换播放和暂停
 *      如果当前不是播放状态，调用媒体播放器的播放函数
 *      否则暂停
 *
 *  todo: 显示播放状态到界面控件 -> 通过媒体播放器的状态变化信号
*/
void Widget::on_pushButton_play_clicked()
{
    LOG << "\n";
    if (m_mediaPlayer->state() == QMediaPlayer::State::PlayingState)
    {
        m_mediaPlayer->pause();
    }
    else
    {
        m_mediaPlayer->play();
    }
}

/* 自定义槽函数，连接和处理媒体播放器对象的播放状态变化信号
 * void stateChanged(QMediaPlayer::State newState); //播放状态变化信号，播放中、暂停、停止
 *      初始化媒体函数中连接
 * ----- update log 2023.06.06 19:59 -----
 * 更新显示图标
*/
void Widget::handle_mediaPlayer_stateChanged(QMediaPlayer::State newState)
{
    LOG << newState << "\n";
    //QString text = (newState == QMediaPlayer::State::PlayingState)? "播放" : "暂停";
    //ui->pushButton_play->setText(text);

    QString path = (newState == QMediaPlayer::State::PlayingState)? ":/icons/play.png" : ":/icons/pause.png";
    ui->pushButton_play->setIcon(QIcon(path));
}

/*
 * 界面添加一个水平滑动条显示播放进度，标签显示当前播放时间和歌曲总时长
 *
 * 自定义槽函数，连接和处理媒体播放器的播放进度变化信号
 *  void positionChanged(qint64 position); //播放进度变化信号
 *  参数接收媒体播放器当前播放时间进度（毫秒级）
 *  在初始化媒体函数中连接
 *
 * 滑动条：前端进度条的值 = 后台当前播放进度 / 后台歌曲时长 * 前端进度条最大值
 *  计算当前进度占总时长的比例，
 *  计算滑动条最大值乘以该比例的值，即滑动条当前值，设置给滑动条
 *
 * 时间标签 00:00/04:02
 *  时间标签文本: 当前分钟数:当前秒数/歌曲时长分钟数:歌曲时长秒数
*/
void Widget::handle_mediaPlayer_positionChanged(qint64 position)
{
    LOG << position << "\n";
    qint64 seconds = position / 1000;   //秒级的进度
    qint64 duration = m_mediaPlayer->duration() / 1000; //秒级的歌曲时长

    int sliderMax = ui->horizontalSlider_position->maximum(); //进度条的最大值

    //前端进度条的值 = 后台当前播放进度 / 后台歌曲时长 * 前端进度条最大值
    ui->horizontalSlider_position->setValue((double)seconds / duration * sliderMax);

    //时间标签文本: 当前分钟数:当前秒数/歌曲时长分钟数:歌曲时长秒数
    //使用QString占位符处理时间标签文本（不足补0）：QString("%1").arg(进度数值, 显示位数2, 进制10, 填充字符QChar('0'))
    QString text = QString("%1:%2/%3:%4").arg(seconds / 60, 2, 10, QChar('0'))
                                         .arg(seconds % 60, 2, 10, QChar('0'))
                                         .arg(duration / 60, 2, 10, QChar('0'))
                                         .arg(duration % 60, 2, 10, QChar('0'));

    ui->label_position->setText(text);
}

/*
 * 界面添加一个水平滑动条显示播放进度，标签显示当前播放时间和歌曲总时长
 *  滑动条转到槽，处理释放信号，以设置媒体播放器的播放进度，跳转播放
 *
 *  播放进度调节: 界面滑动条的变化(松开信号) -> 媒体播放器的播放进度的设置（跳转播放）
 *  进度计算：后台播放进度 = 前端进度条 / 前端进度条最大值 * 后台歌曲时长
*/
void Widget::on_horizontalSlider_position_sliderReleased()
{
    LOG << "\n";
    int sliderValue = ui->horizontalSlider_position->value();
    int sliderMaxValue = ui->horizontalSlider_position->maximum();
    //后台播放进度 = 前端进度条 / 前端进度条最大值 * 后台歌曲时长
    m_mediaPlayer->setPosition((double)sliderValue / sliderMaxValue * m_mediaPlayer->duration());
}

/*
 * 处理上一首
 *  打印上一首的歌曲索引
 *  调用上一首的接口next
*/
void Widget::on_pushButton_previous_clicked()
{
    LOG << "上一首歌曲序号：" << m_mediaPlaylist->previousIndex() << "\n";
    m_mediaPlaylist->previous();
}

/*
 * 处理下一首
 *  打印下一首的歌曲索引
 *  调用下一首的接口next
*/
void Widget::on_pushButton_next_clicked()
{
    LOG << "下一首歌曲序号：" << m_mediaPlaylist->nextIndex() << "\n";
    m_mediaPlaylist->next();
}

/*
 * 定义槽函数，连接和处理媒体播放列表的当前媒体变化信号，即切歌信号
 *  在初始化多媒体函数中连接
 * 1.如果参数媒体对象为空，清空歌曲名标签，返回
 * 2.读取当前媒体，显示歌名
 * 3.高亮显示界面歌曲列表控件的当前歌曲
 *      后台播放列表歌曲当前项显示到前端：后台媒体播放列表当前项（切歌信号中） -> 设置前端歌曲列表当前项
 *      前提是按相同顺序将每首歌曲添加到后台列表和前端列表，这样它们每首歌索引相同
 *
 * ------update log 2023.06.08 ------
 * 读取歌曲管理员中的歌曲信号，显示到界面标签
 * 显示所有歌词
*/
void Widget::handle_mediaPlaylist_currentMediaChanged(const QMediaContent & media)
{
    LOG << "\n";
    if (media.isNull())
    {
        ui->label_song->clear();
        return;
    }

    ui->listWidget_playlist->setCurrentRow(m_mediaPlaylist->currentIndex());

    Song & songRef = SongManager::getInstance().song(media.canonicalUrl());
    ui->label_song->setText(songRef.name() + " - " + songRef.artist());

    updateAllLyrics(songRef.lyrics());
}

/*
 * 点击播放模式按钮，调节到下一个模式
*/
void Widget::on_pushButton_playbackMode_clicked()
{
    LOG << "\n";
    QMediaPlaylist::PlaybackMode mode = m_mediaPlaylist->playbackMode();
    int nextMode = (mode + 1) % 5;
    m_mediaPlaylist->setPlaybackMode(QMediaPlaylist::PlaybackMode(nextMode));
}

/*
 * 连接和处理后台媒体播放列表的播放模式变化信号
 *  初始化多媒体时连接
 *  播放模式：enum PlaybackMode { CurrentItemOnce, CurrentItemInLoop, Sequential, Loop, Random };
 *  显示当前播放模式到界面
 * ----- update log 2023.06.06 20:02 -----
 * 更新显示图标
*/
void Widget::handle_mediaPlaylist_playbackModeChanged(QMediaPlaylist::PlaybackMode mode)
{
    LOG << mode << "\n";
    //static QStringList text = { "单曲播放", "单曲循环", "顺序播放", "循环播放", "随机播放" };
    //ui->pushButton_playbackMode->setText(text[mode]);

    static QStringList path =
    {
        ":/icons/currentItemOnce.png",
        ":/icons/currentItemLoop.png",
        ":/icons/sequential.png",
        ":/icons/Loop.png",
        ":/icons/random.png"
    };

    ui->pushButton_playbackMode->setIcon(QIcon(path[mode]));
}

/*
 * 处理列表项双击信号，切换播放所双击的那首歌
 *  前端歌曲列表双击当前行（双击信号） -> 设置媒体播放列表当前索引, 并调用媒体播放器的播放函数
*/
void Widget::on_listWidget_playlist_itemDoubleClicked(QListWidgetItem *item)
{
    m_mediaPlaylist->setCurrentIndex(ui->listWidget_playlist->currentRow());
    m_mediaPlayer->play();
}

/*
 * 设置界面布局
 *  四层水平布局
 *  一个总的垂直布局
 *      歌曲列表控件 歌词控件
 *      按钮：添加、上一首、播放、下一首、播放模式
 *      歌曲信息标签
 *      播放进度条和进度标签
 */
void Widget::initLayout()
{
    QHBoxLayout * h1 = new QHBoxLayout();
    h1->addWidget(ui->listWidget_playlist, 1);  //指定占比1
    h1->addWidget(ui->listWidget_lyrics, 3);    //指定占比3

    QHBoxLayout * h2 = new QHBoxLayout();
    h2->addWidget(ui->pushButton_add);
    h2->addWidget(ui->pushButton_previous);
    h2->addWidget(ui->pushButton_play);
    h2->addWidget(ui->pushButton_next);
    h2->addWidget(ui->pushButton_playbackMode);

    QHBoxLayout * h3 = new QHBoxLayout();
    h3->addWidget(ui->label_song);

    QHBoxLayout * h4 = new QHBoxLayout();
    h4->addWidget(ui->horizontalSlider_position, 5);    //占比5
    h4->addWidget(ui->label_position, 1, Qt::AlignRight); //占比1，居右显示

    QVBoxLayout * v = new QVBoxLayout();
    v->addLayout(h1);
    v->addLayout(h2);
    v->addLayout(h3);
    v->addLayout(h4);

    setLayout(v);
}

//测试函数，
//测试歌曲类、歌曲管理类
void Widget::testSong()
{
    Song s(QUrl("歌曲路径-测试"), "歌名-测试", "歌手-测试", "专辑名-测试");
    LOG << "测试输出歌曲对象: " << s << "\n";

    Song * ps = new Song(s);
    SongManager & smRef = SongManager::getInstance();
    smRef.addSong(ps);
    LOG << "测试调用歌曲管理类相关接口：" << smRef.size() << "\n";
    LOG << smRef.contains(ps->url());
    LOG << "测试输出歌曲管理员中的第一首歌：" << *(smRef.songs().first()) << "\n";
    LOG << "测试输出第一首歌的歌名：" << smRef.songs().first()->name() << "\n";
}

/*
 * 主窗口中编写测试案例，测试消息队列单例类和工作类
*/
void Widget::testWorker()
{
    MessageQueue & mqRef = MessageQueue::getInstance();
    mqRef.push(new Song(QUrl("歌曲路径-测试消息队列"), "歌名-测试消息队列", "歌手-测试消息队列", "专辑名-测试消息队列"));
    LOG << "测试消息队列数据个数：" << mqRef.size() << "\n";
    LOG << "测试消息队列是否为空：" << mqRef.empty() << "\n";
    Song * s = mqRef.pop();
    LOG << "输出消息队列取出的数据：" << *s << "\n";
    delete s;

    Worker w;
    w.getASong(QUrl("测试-歌曲路径"));
    QString path = "../music/光辉岁月 - BEYOND.mp3"; //访问上一级目录中的music目录中的歌曲文件
    w.getASong(QUrl(path));
}

/*初始化工作对象和子线程
 *  实例化子线程成员和工作对象（委托给子线程，并启动子线程执行）
 *  连接子线程的结束信号finished => 自带释放槽函数deleteLater
 *  连接子线程的结束信号finished => 工作对象的释放槽函数deleteLater
 *  连接主窗口的添加歌曲信号addSong => 工作对象的歌曲解析槽函数getASong
 *  连接工作对象的解析结束信号getASongFinished => 主窗口的处理槽函数handle_worker_getASongFinished
 *
 * -----update log 2023.06.10-----
 *  连接存储歌曲信号和槽函数
 *  连接查询歌曲信号和槽函数
 */
void Widget::initWorker()
{
    LOG << "\n";
    m_thread = new QThread(this);
    Worker * wo = new Worker();

    connect(m_thread, &QThread::finished, m_thread, &QThread::deleteLater);
    connect(m_thread, &QThread::finished, wo, &Worker::deleteLater);
    connect(this, &Widget::addSong, wo, &Worker::getASong);
    connect(wo, &Worker::getASongFinished, this, &Widget::handle_worker_getASongFinished);
    connect(this, &Widget::saveSong, wo, &Worker::handle_saveSong);
    connect(this, &Widget::querySong, wo, &Worker::handle_querySong);

    wo->moveToThread(m_thread);
    m_thread->start();
}

/*连接和处理工作对象的解析结束信号
 * 循环读取消息队列
 *  获取歌曲对象
 *      存储到歌曲管理员
 *      设置给后台媒体播放列表
 *      显示到界面歌曲列表
 *
 * -----update log 2023.06.10-----
 * 发送存储歌曲信号
 *
 * 给槽函数和对应的信号函数加个参数，bool值表示是否从数据库中查询到的，true表示是从数据库查询，false表示不是
 */
void Widget::handle_worker_getASongFinished(bool fromDatabase)
{
    MessageQueue & mqRef = MessageQueue::getInstance();
    LOG << "消息队列元素个数：" << mqRef.size() << ", 是否从数据库中查询：" << fromDatabase << "\n";
    Song * song = nullptr;

    while (!mqRef.empty())
    {
        song = mqRef.pop();
        if (song != nullptr) //不为空则存储
        {
            LOG << "读取到歌曲对象，存储到歌曲管理员，由其负责释放：" << *song << "\n";
            SongManager::getInstance().addSong(song);
            m_mediaPlaylist->addMedia(song->url());
            ui->listWidget_playlist->addItem(new QListWidgetItem(song->name() + " - " + song->artist()));

            if (!fromDatabase)
            {
                emit saveSong(song);
            }
        }
    }
}

/*
 * 刷新显示全部歌词（切歌时调用）
 * 1.清空歌词列表控件的旧内容
 * 2.判断歌词容器是否为空，为空显示无歌词
 * 3.读取歌词容器的文本，构造到列表元素里，设置给列表控件
 *     循环遍历歌词容器的文本部分
 *      构造一个QListWidgetItem元素
 *      设置元素文本为歌词文本，并设置文本居行中间
 *      添加到列表控件
*/
void Widget::updateAllLyrics(const QMap<qint64, QString>& lyrics)
{
    LOG << "\n";
    ui->listWidget_lyrics->clear();
    if (lyrics.isEmpty())
    {
        QListWidgetItem * item = new QListWidgetItem("无歌词");
        item->setTextAlignment(Qt::AlignCenter); //设置该行文本居中显示
        ui->listWidget_lyrics->addItem(item);
        return;
    }

    LOG << "更新所有歌词，第一行的文本：" << lyrics.first() << "\n";
    for (auto text : lyrics) //获取lyrics中的每行歌词文本到text
    {
        //LOG << "text: " << text << "\n";
        QListWidgetItem * item = new QListWidgetItem(text);
        item->setTextAlignment(Qt::AlignCenter); //设置该行文本居中显示
        ui->listWidget_lyrics->addItem(item);
    }
}

/*
 *歌词实时刷新函数
 * 播放状态下，获取媒体播放器中的歌曲路径url，再去歌曲管理员查询歌词
 * 根据媒体播放器的当前播放进度，查找对应进度的歌词文本
 *  歌词信息中，时间戳是该行歌词的起始播放时间
 *  循环遍历歌词容器，从头往后找，比较播放进度和时间戳，播放进度大于等于某一行（的起始时间），并且小于下一行
 *
 * 同步显示
 *  高亮显示
 *  滚动居中
*/
void Widget::updateCurrentLyrics()
{
    //LOG << "todo: 更新当前歌词" << "\n";
    if (m_mediaPlayer->state() != QMediaPlayer::PlayingState)
    {
        //LOG << "当前不是播放状态" << "\n";
        return;
    }

    QUrl url = m_mediaPlayer->currentMedia().canonicalUrl();
    const QMap<qint64, QString> & lyrics = SongManager::getInstance().lyrics(url);
    if (lyrics.isEmpty()) { return; }

    //LOG << "实时刷新歌词，媒体播放器当前进度：" << m_mediaPlayer->position() << "\n";
    qint64 position = m_mediaPlayer->position();
    int index = 0; //存储当前进度对应的歌词文件的索引

    //循环遍历歌词容器，从头往后找，
    for (auto it = lyrics.begin(); it != lyrics.end(); it++)
    {
        //end前面的最后一个元素
        if (lyrics.end() == (it + 1)) { break; }

        //比较播放进度和时间戳，播放进度大于等于某一行（的起始时间），并且小于下一行
        if (position >= it.key() && position < (it + 1).key())
        {
//            LOG << "当前歌词时间范围和播放器进度：[" << it.key() << ", " << (it + 1).key() << ") "
//                << m_mediaPlayer->position() << " "
//                << it.value()
//                << "\n";
            break;
        }

        index++; //如果不是这一行，就继续比较下一行
    }

    ui->listWidget_lyrics->setCurrentRow(index); //界面控件设置当前行，高亮显示
    QListWidgetItem * item = ui->listWidget_lyrics->item(index); //获取当前行元素
    ui->listWidget_lyrics->scrollToItem(item, QAbstractItemView::PositionAtCenter); //列表滚动到该行，并垂直居中
}

//连接和处理定时器的超时信号, 调用歌词同步/实时刷新函数
void Widget::handle_timer_timeout()
{
    //LOG << "\n";
    updateCurrentLyrics();
}

/*
 * 定义初始化定时器函数，
 *      设置超时间隔，
 *      连接超时信号到自定义槽函数，
 *      开启定时器
*/
void Widget::initTimer()
{
    m_timer = new QTimer(this);
    m_timer->setInterval(100);  //设置超时间隔100毫秒
    connect(m_timer, &QTimer::timeout, this, &Widget::handle_timer_timeout);
    m_timer->start();
}

void Widget::testLog2File()
{
//    LOG << "test LOG3";
//    LOG3 << "test LOG3\n";
//    LOG3 << "test LOG3 again\n";
//    LOG3 << "测试输出中文\n";
//    LOG3 << QString("测试输出中文\n");

    LogStream::instance().test();
    LogStream::instance() << "测试调用日志流类的单例进行输出" << "\n";
    LogStream::instance() << "测试调用日志流类的单例进行输出 " << __FILE__ << ":" << __LINE__ << " " << __PRETTY_FUNCTION__ << "\n";
}

void Widget::resumePlaylist()
{
    LOG << "恢复播放列表，发送查询歌曲信号" << "\n";
    emit querySong();
}
