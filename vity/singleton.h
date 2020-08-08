#ifndef __VITY_SINGLETON_H__
#define __VITY_SINGLETON_H__

#include <memory>

template<typename T>
class Singleton{
public:
    static T* GetInstance(){
        static T t;
        return &t;
    }
};

template<typename T>
class SingletonPtr{
public:
    static std::shared_ptr<T> GetInstance(){
        static std::shared_ptr<T> t(new T);
        return t;
    }
};


#endif