#ifndef WORKER_H
#define WORKER_H

#include <QObject>
#include <QQueue> //Qt队列容器
#include <QMutex>   //Qt互斥量
#include <QMutexLocker> //互斥量管理类，构造时加锁，析构时解锁（自动加解锁）
#include "song.h"
#include "common.h"

/*消息队列单例类，用于存储数据（歌曲对象指针），共享给多个线程访问读写
 * todo: 抽象一个消息类Message，内部封装消息类型、具体的消息体等属性
 *  内部一个队列和互斥量
 *  入队一个歌曲指针的接口
 *  出队一个歌曲指针的接口
 *  判断是否为空的接口
 *      取代手动调用加锁lock和解锁unlock，使用自动加解锁的类对象QMutexLocker
 *      传入一个QMuext指针构造一个局部QMutexLocker对象，构造时自动加锁，析构（函数返回）时自动解锁
 *      这种以局部对象自动管理资源的方式称为RAII(Resource Acquisition Is Initialization) 资源获取即初始化
 *  查询数据个数接口size
*/
class MessageQueue
{
private:
    MessageQueue() {}
    MessageQueue(const MessageQueue& other) {}
    ~MessageQueue() { /*todo: 清空消息队列中的数据*/ }

public:
    static MessageQueue& getInstance()
    {
        static MessageQueue instance;
        return instance;
    }

private:
    QQueue <Song*> m_queue; //存储数据的队列
    QMutex m_mutex; //互斥量，保护外部多线程访问共享数据m_queue，
    //互斥量本身由系统进行访问保护（同一时刻只会有一个线程获取到互斥量（加锁成功））

public:
    //入队一个歌曲指针的接口
    void push(Song * song)
    {
        if (!song) { return; }

        LOG << *song << "\n";
        m_mutex.lock();
        m_queue.push_back(song);
        m_mutex.unlock();
    }

    /*
     * 出队一个歌曲指针接口，注意队列为空的情况
     *  第一次判断如果为空直接返回（避免没有数据时加解锁的开销）
     *  加锁
     *      第二次判断不为空再将队头元素取出（防止加锁过程中正好被别的线程取完数据成为空队列）
     *  解锁
     *  返回歌曲对象指针
     */
    Song * pop()
    {
        LOG << m_queue.size() << "\n";
        Song * song = nullptr;

        if (m_queue.isEmpty()) { return song; }

        m_mutex.lock();
        if (!m_queue.isEmpty())
        {
            song = m_queue.front(); //存储队头元素
            m_queue.pop_front();    //弹出队头元素
        }

        m_mutex.unlock();
        return song;
    }

    /*
     * 判断是否为空的接口
     * 1.手动加解锁
     * 2.使用RAII机制自动加解锁：使用QMutexLocker生成局部对象
     */
    bool empty()
    {
//        //1.手动加解锁
//        m_mutex.lock();
//        bool ret = m_queue.empty();
//        m_mutex.unlock();
//        return ret;

        //2.使用RAII机制自动加解锁
        QMutexLocker locker(&m_mutex);
        return m_queue.empty();
    }

    int size()
    {
        QMutexLocker locker(&m_mutex);
        return m_queue.size();
    }
};

/*工作类，定义界面交互以外的任务处理逻辑，比如解析歌曲信息
 * 解析歌曲信息槽函数，参数接收mp3文件路径
 * 声明一个解析结束信号，解析完成后通知主窗口处理
 * 即负责处理主窗口分配的工作任务，处理结束后通知对方 -- 通过信号槽通信
 *
 * 封装一个函数读取解析歌词时间戳和文本
 *
 * -----update log 2023.06.10 -----
 * 添加存储歌曲槽函数，持久化存储到数据库
 *  todo：暂时存储路径、歌名等信息，后续将歌词也存储到数据库
 * 定义一个查询歌曲槽函数，从数据库查询，查询后存储到消息队列，并通知主窗口
 *
 * 给解析结束信号函数和槽函数加个参数，
 *  bool值表示是否从数据库中查询到的，true表示是从数据库查询，false表示不是，信号函数设置参数默认值false
 *  避免重复存储从数据库中查询到的歌
 */
class Worker : public QObject
{
    Q_OBJECT
public:
    explicit Worker(QObject *parent = nullptr);
    ~Worker();

signals:
    void getASongFinished(bool fromDatabase = false); //声明一个解析结束信号，解析完成后通知主窗口处理

public slots:
    void getASong(const QUrl& mp3Url);  //解析歌曲信息槽函数，参数接收mp3文件路径
    void readLyrics(Song * song);   //解析歌词，从参数歌曲中按歌曲文件路径解析歌词文件
    void handle_saveSong(Song* song);         //处理存储歌曲信号的槽函数，持久化存储到数据库
    void handle_querySong();
};

#endif // WORKER_H
