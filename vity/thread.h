#ifndef __VITY_THREAD_H__
#define __VITY_THREAD_H__

#include <thread>
#include <functional>
#include <memory>
#include <pthread.h>
#include <semaphore.h>
#include <atomic>

namespace vity{
/*
信号量
    线程的信号和进程的信号量类似，使用线程的信号量可以高效地完成基于线程的资源计数。信号量实际上是一个非负的整数计数器，
    用来实现对公共资源的控制。在公共资源增加的时候，信号量就增加；公共资源减少的时候，信号量就减少；只有当信号量的值大于0的时候，
    才能访问信号量所代表的公共资源。
    常用头文件：
        sem_t sem_event;
        int sem_init(sem_t *sem, int pshared, unsigned int value);//初始化一个信号量 
        int sem_destroy(sem_t * sem);//销毁信号量
        int sem_post(sem_t * sem);//信号量增加1
        int sem_wait(sem_t * sem);//信号量减少1
        int sem_getvalue(sem_t * sem, int * sval);//获取当前信号量的值
*/
class Semphore{

public:
    Semphore(uint32_t count = 0){
        // 在进程内线程共享 信号量值为10
        if(sem_init(&m_semaphore,0,count)){
            throw std::logic_error("sem_init error");
        }
    }
    ~Semphore(){
        sem_destroy(&m_semaphore);
    }
    // 信号量减少1
    void wait(){
        if(sem_wait(&m_semaphore)){
            std::logic_error("sem_wait error");
        }
    }
    // 信号量增加1
    void notify(){
        if(sem_post(&m_semaphore)){
            std::logic_error("sem_post error");
        }
    }
private:
    Semphore(const Semphore&) = delete;
    Semphore(const Semphore&&) = delete;
    Semphore& operator=(const Semphore&) = delete;
private:
    sem_t m_semaphore;
};

template<class T>
struct ScopedLockImpl{
public:
    ScopedLockImpl(T& mutex)
        :m_mutex(mutex)
    {
        m_mutex.lock();
        m_locked = true;
    }
    ~ScopedLockImpl(){
        unlock();
    }
    void lock(){
        if(!m_locked){
            m_mutex.lock();
            m_locked = true;
        }
    }
    void unlock(){
        if(m_locked){
            m_mutex.unlock();
            m_locked = false;
        }
    }
private:
    T& m_mutex;
    bool m_locked;
};


template<class T>
struct ReadScopedLockImpl{
public:
    ReadScopedLockImpl(T& mutex)
        :m_mutex(mutex)
    {
        m_mutex.rdlock();
        m_locked = true;
    }
    ~ReadScopedLockImpl(){
        unlock();
    }
    void lock(){
        if(!m_locked){
            m_mutex.rdlock();
            m_locked = true;
        }
    }
    void unlock(){
        if(m_locked){
            m_mutex.unlock();
            m_locked = false;
        }
    }
private:
    T& m_mutex;
    bool m_locked;
};

template<class T>
struct WriteScopedLockImpl{
public:
    WriteScopedLockImpl(T& mutex)
        :m_mutex(mutex)
    {
        m_mutex.wrlock();
        m_locked = true;
    }
    ~WriteScopedLockImpl(){
        unlock();
    }
    void lock(){
        if(!m_locked){
            m_mutex.wrlock();
            m_locked = true;
        }
    }
    void unlock(){
        if(m_locked){
            m_mutex.unlock();
            m_locked = false;
        }
    }
private:
    T& m_mutex;
    bool m_locked;
};
/*
互斥锁 
    一把锁 在访问共享资源前对互斥量进行加锁 在访问完成之后释放互斥量
    对互斥量进行加锁之后，其他再次对互斥量进行加锁的线程都会被阻塞 直到当前线程释放该互斥量
    如果释放互斥量是有一个以上的线程阻塞,那么所有的该锁上的阻塞线程都会变成可运行状态 
    第一个变成可运行状态的线程可以对互斥量加锁
        int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr);//互斥初始化
        int pthread_mutex_destroy(pthread_mutex_t *mutex);//销毁互斥
        int pthread_mutex_lock(pthread_mutex_t *mutex);//锁定互斥
        int pthread_mutex_unlock(pthread_mutex_t *mutex);//解锁互斥
        int pthread_mutex_trylock(pthread_mutex_t *mutex);//销毁互斥
        eg.pthread_t mutex;
        pthread_mutex_init(&mutex, NULL);
        pthread_mutex_lock(&mutex);
        ...
        pthread_mutex_unlock(&mutex);
        pthread_mutex_detroy(&mutex);
*/
class Mutex{
public:
    typedef ScopedLockImpl<Mutex> Lock;

    Mutex()
    {
        pthread_mutex_init(&m_mutex,nullptr);
    }

    ~Mutex(){
        pthread_mutex_destroy(&m_mutex);
    }
    void lock()
    {
        pthread_mutex_lock(&m_mutex);
    }
    void unlock(){
        pthread_mutex_unlock(&m_mutex);
    }

private:
    pthread_mutex_t m_mutex;
};

class NullMutex{
public:
    typedef ScopedLockImpl<NullMutex> Lock;
    NullMutex(){}
    ~NullMutex(){}
    void lock(){}
    void unlock(){}
};

/*
读写锁
    读模式下加锁状态，写模式下加锁状态，不加锁状态
    一次只有一个线程可以占有写模式的读写锁，但是多个线程可以同时占有读模式的读写锁
    当读写锁是写加锁状态时，在这个锁被解锁之前，所有视图对这个锁加锁的线程都会被阻塞
    当读写锁在读加锁状态时，所有试图以读模式对它进行加锁的线程都可以得到访问权，但是任何希望以写模式对此锁进行加锁的线程都会阻塞，
        int pthread_rwlock_init(pthread_rwlock_t *rwlock, const pthread_rwlockattr_t *rwlockattr);//初始化读写锁
        int pthread_rwlock_destroy(pthread_rwlock_t *rwlock);//销毁读写锁
        int pthread_rwlock_rdlock(pthread_rwlock_t *rwlock);//读模式锁定读写锁
        int pthread_rwlock_wrlock(pthread_rwlock_t *rwlock);//写模式锁定读写锁
        int pthread_rwlock_unlock(pthread_rwlock_t *rwlock);//解锁读写锁
        eg.pthread_rwlock_t q_lock;
        pthread_rwlock_init(&q_lock, NULL);
        pthread_rwlock_rdlock(&q_lock);
        ...
        pthread_rwlock_unlock(&q_lock);
        pthread_rwlock_detroy(&q_lock);
*/
class RWMutex{
public:
    typedef ReadScopedLockImpl<RWMutex> ReadLock;
    typedef WriteScopedLockImpl<RWMutex> WriteLock;

    RWMutex(){
        pthread_rwlock_init(&m_lock,nullptr);
    }
    ~RWMutex(){
        pthread_rwlock_destroy(&m_lock);
    }

    void rdlock(){
        pthread_rwlock_rdlock(&m_lock);
    }

    void wrlock(){
        pthread_rwlock_wrlock(&m_lock);
    }

    void unlock(){
        pthread_rwlock_unlock(&m_lock);
    }
private:
    pthread_rwlock_t m_lock;
};

class NullRWMutex{
public:
    typedef ScopedLockImpl<NullMutex> Lock;
    NullRWMutex(){}
    ~NullRWMutex(){}
    void rdlock(){}
    void wrlock(){}
    void unlock(){}
};

class Spinlock {
public:
    typedef ScopedLockImpl<Spinlock> Lock;
    Spinlock() {
        pthread_spin_init(&m_mutex, 0);
    }

    ~Spinlock() {
        pthread_spin_destroy(&m_mutex);
    }

    void lock() {
        pthread_spin_lock(&m_mutex);
    }

    void unlock() {
        pthread_spin_unlock(&m_mutex);
    }
private:
    pthread_spinlock_t m_mutex;
};

class CASLock {
public:
    typedef ScopedLockImpl<CASLock> Lock;
    CASLock() {
        m_mutex.clear();
    }
    ~CASLock() {
    }

    void lock() {
        while(std::atomic_flag_test_and_set_explicit(&m_mutex, std::memory_order_acquire));
    }

    void unlock() {
        std::atomic_flag_clear_explicit(&m_mutex, std::memory_order_release);
    }
private:
    volatile std::atomic_flag m_mutex;
};


class Thread{
public:
    typedef std::shared_ptr<Thread> ptr;
    Thread(std::function<void()> cb,const std::string& name);
    ~Thread();

    pid_t getId() const { return m_id; }
    const std::string& getName() const { return m_name; }

    void join();

    // 在某个函数中获取自己的线程 然后对线程执行某些操作  join等
    // 给当前线程使用
    static Thread* GetThis();
    // 用于日志  获取线程名称
    static const std::string& GetName();
    // 主线程不是我们创建的提供一个写函数用于主线程使用
    static void SetName(const std::string& name);
    static void* run(void *arg);

private:
    Thread(const Thread&) = delete;
    Thread(const Thread&&) = delete;
    Thread& operator=(const Thread&) = delete;

private:
    pid_t m_id = -1;
    //typedef unsigned long pthread_t 
    pthread_t m_thread = 0;
    std::function<void()> m_cb;
    std::string m_name;
};

}

/*
    linux线程同步常用方式
    互斥锁 死锁:
        一个线程需要访问两个或多个不同的共享资源 而每个资源又由不同的互斥量管理 当超过一个线程加锁
        同一组互斥量时,就可能发生死锁
        死锁:多个线程/进程因竞争资源而造成的一种僵局(相互等待)  若无外力作用 这些进程都无法向前推进
    死锁的处理策略:
        1.预防死锁 破坏死锁产生的四个条件：互斥条件、不剥夺条件、请求和保持条件以及循环等待条件。
        2.避免死锁 在每次进行资源分配前，应该计算此次分配资源的安全性，如果此次资源分配不会导致系统进入不安全状态，那么将资源分配给进程，否则等待。算法：银行家算法。
        3.检测死锁 检测到死锁后通过资源剥夺、撤销进程、进程回退等方法解除死锁

    条件变量
        互斥量用于上锁，条件变量则用于等待，并且条件变量总是需要与互斥量一起使用
        条件变量本身是由互斥量保护的，线程在改变条件变量之前必须首先锁住互斥量。
            int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr);//初始化条件变量
            int pthread_cond_destroy(pthread_cond_t *cond);//销毁条件变量
            int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);//无条件等待条件变量变为真
            int pthread_cond_timewait(pthread_cond_t *cond, pthread_mutex_t *mutex, const struct timespec *tsptr);//在给定时间内，等待条件变量变为真
            
            eg.pthread_mutex_t mutex;
            pthread_cond_t cond;
            ...
            pthread_mutex_lock(&mutex);
            pthread_cond_wait(&cond, &mutex);
            ...
            pthread_mutex_unlock(&mutex);
            ...

        pthread_cond_wait 执行的流程首先将这个mutex解锁, 然后等待条件变量被唤醒, 如果没有被唤醒, 该线程将一直休眠,
        也就是说, 该线程将一直阻塞在这个pthread_cond_wait调用中, 而当此线程被唤醒时, 将自动将这个mutex加锁，
        然后再进行条件变量判断（原因是“惊群效应”，如果是多个线程都在等待这个条件，而同时只能有一个线程进行处理，
        此时就必须要再次条件判断，以使只有一个线程进入临界区处理。），如果满足，则线程继续执行
*/

#endif