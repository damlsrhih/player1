#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QMediaPlayer>
#include <QMediaPlaylist>
#include <QListWidgetItem>
#include <QTimer>
#include "song.h"

namespace Ui {
class Widget;
}

/*
 * ----- update log 2023.06.04 15:00 hu 初始化和添加、播放音乐 -----
 * 主窗口类Widget
 *  定义使用<common.h>中的日志宏LOG，后续在每个函数中调用LOG
 *  添加初始化窗口函数
 *  项目.pro配置使用multimedia模块
 *  声明一个媒体播放器成员，需包含头文件<QMediaPlayer>
 *  添加一个初始化多媒体函数
 *
 * 界面UI
 *  添加一个添加音乐按钮，转到槽处理点击信号
 *  添加一个控件标签，显示歌曲名，添加音乐时显示歌曲文件名
 *  添加一个播放音乐按钮，转到槽处理点击信号，读取播放状态，切换播放和暂停
 *
 * ----- update log 2023.06.04 15:50 hu 播放状态和进度显示 -----
 * 界面添加一个水平滑动条显示播放进度，标签显示当前播放时间和歌曲总时长
 *      滑动条转到槽，处理释放信号，以设置媒体播放器的播放进度，跳转播放
 * 类中自定义槽函数，连接和处理媒体播放器对象的播放状态变化信号
 * 类中自定义槽函数，连接和处理媒体播放器的播放进度变化信号
 *
 * ----- update log 2023.06.04 16:30 hu 媒体播放列表和播放模式 -----
 * 类中声明一个媒体播放列表成员，需包含头文件<QMediaPlaylist>
 * 界面添加上一首和下一首按钮，转到槽处理切歌
 * 类中定义槽函数，连接和处理媒体播放列表的当前媒体变化信号，即切歌信号
 * 界面添加一个按钮，显示播放模式，转到槽处理点击信号，以切换后台媒体播放列表的播放模式
 * 类中自定义槽函数，连接和处理后台媒体播放列表的播放模式变化信号
 *
 * ----- update log 2023.06.04 17:15 界面歌曲列表控件、歌词列表控件 -----
 * 界面添加列表窗口控件作为歌曲列表控件
 *      添加音乐时显示所添加的歌曲文件名
 *      切歌信号中，高亮显示歌曲列表控件的当前歌曲
 *      转到槽，处理列表项双击信号，切换播放所双击的那首歌
 * 界面添加歌词列表控件，todo：后续解析歌词后，播放时显示歌词
 *
 * ----- update log 2023.06.06 19:42 控件优化和图标设置 -----
 * 在初始化窗口函数中设置控件样式
 * 添加一个Qt资源文件，管理控件所用图标的图片资源
 * 布局管理
 *
 * ----- update log 2023.06.06 20:40 歌曲抽象和并发 -----
 * 封装歌曲类，编写测试案例测试歌曲类
 * 封装歌曲管理类，编写测试案例测试歌曲管理类
 *
 * 封装消息队列单例类，编写测试案例测试单例类
 * 封装工作类，编写测试案例测试单例类
 *
 * ----- update log 2023.06.08 20:16 工作对象和子线程 -----
 * 初始化一个工作对象和子线程，处理解析
 *  主窗口类包含一个子线程成员，定义初始化子线程和工作对象的函数
 *  主窗口声明添加歌曲信号，连接和发送给工作对象
 *  主窗口定义槽函数，连接和处理工作对象的解析结束信号
 *  主窗口析构时等待子线程退出
 *
 * 解析流程：
 *      主窗口添加音乐时，发送添加歌曲信号，参数传递mp3路径
 *      工作对象接收添加歌曲信号并读取文件，解析后存储到消息队列，并发送解析结束信号通知主窗口
 *      主窗口连接和处理解析结束信号，读取消息队列中的歌曲，存储到歌曲对象管理员，
 *          后续从歌曲管理员中读取歌曲信息显示到界面
 *
 * 切歌信号中，读取歌曲信息并显示到界面标签
 * 定义刷新所有歌词函数，切歌时调用
 * 定义刷新显示当前歌词函数，用来同步歌词
 *
 * 使用定时器，同步歌词
 *  声明一个定时器成员
 *  定义初始化定时器函数（构造时调用）
 *      设置超时间隔，
 *      连接超时信号到自定义槽函数，
 *      开启定时器（析构时停止）
 *  自定义槽函数，处理超时信号，调用歌词同步函数
 *
 * ----- update log 2023.06.10 -----
 * 自定义日志输出类Log2File
 *  定义宏LOG3, 使用Log2File进行输出
 * 编写函数测试，调用LOG3
 *
 * 定义数据库管理类DatabaseManager
 *
 * 存储歌曲
 *  工作类定义存储歌曲槽函数，存储歌曲到数据库
 *  主窗口声明存储歌曲信号，
 *      连接到工作对象，初始化工作对象时连接，
 *      处理歌曲解析结束信号时发送
 *
 * 恢复歌曲
 *  工作类定义查询歌曲槽函数，
 *      从数据库查询，存储到消息队列，然后通知主窗口
 *  主窗口声明查询歌曲信号，
 *      连接到工作对象的槽函数，初始化工作对象时连接
 *      发送查询信号
 *          定义一个恢复播放列表函数，其中发送该信号
 *          启动时初始化工作对象后，调用恢复函数
 *
 * 给解析结束信号函数和槽函数加个参数，
 *  bool值表示是否从数据库中查询到的，true表示是从数据库查询，false表示不是，
 *  信号函数
 *      设置参数默认值false，以兼容之前文件解析时没有传递参数的代码
 *      在新的查询歌曲槽函数中，传递参数true表示从数据库中查询
 *
 *  存储歌曲时判断，以避免重复存储从数据库中查询到的歌
 *
 * ----- update log 2023.06.11 -----
 * 自定义日志流类，重载输出运算符，处理日志流类和所输出的数据
 *  定义LOG4替换
 *  针对自定义类型数据的输出，单独重载日志流类和自定义类型的输出运算符
*/
class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = nullptr);
    ~Widget();

    void initWindow();  //初始化窗口函数
    void initMedia();   //初始化多媒体函数
    void initLayout();  //布局
    void initWorker();  //初始化工作对象和子线程
    void initTimer();   //初始化定时器

    void updateAllLyrics(const QMap<qint64, QString>& lyrics);     //刷新所有歌词, 参数接收歌词
    void updateCurrentLyrics(); //同步显示当前歌词

    void resumePlaylist();

    //测试案例，测试歌曲类
    void testSong();
    void testWorker();
    void testLog2File();

signals:
    void addSong(const QUrl& mp3Url); //添加歌曲信号
    void saveSong(Song* song);        //存储歌曲信号
    void querySong();                 //查询歌曲信号

public slots:
    //连接和处理媒体播放器对象的播放状态变化信号
    void handle_mediaPlayer_stateChanged(QMediaPlayer::State newState);
    //连接和处理媒体播放器的播放进度变化信号
    void handle_mediaPlayer_positionChanged(qint64 position);
    //连接和处理媒体播放列表的当前媒体变化信号，即切歌信号
    void handle_mediaPlaylist_currentMediaChanged(const QMediaContent & media);
    //连接和处理后台媒体播放列表的播放模式变化信号
    void handle_mediaPlaylist_playbackModeChanged(QMediaPlaylist::PlaybackMode mode);

    //连接和处理工作对象的解析结束信号
    void handle_worker_getASongFinished(bool fromDatabase);

    //连接和处理定时器的超时信号
    void handle_timer_timeout();

private slots:
    void on_pushButton_add_clicked();

    void on_pushButton_play_clicked();

    void on_horizontalSlider_position_sliderReleased();

    void on_pushButton_previous_clicked();

    void on_pushButton_next_clicked();

    void on_pushButton_playbackMode_clicked();

    void on_listWidget_playlist_itemDoubleClicked(QListWidgetItem *item);

private:
    Ui::Widget *ui;
    QMediaPlayer * m_mediaPlayer;   //声明一个媒体播放器成员
    QMediaPlaylist * m_mediaPlaylist; //声明一个媒体播放列表成员
    QThread * m_thread;             //子线程，工作线程，处理界面交互以外的工作
    QTimer * m_timer;               //定时器，用来同步歌词
};

#endif // WIDGET_H
