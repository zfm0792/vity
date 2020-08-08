#ifndef __VITY_CONFIG_H__
#define __VITY_CONFIG_H__

#include <memory>
#include <string>
#include <sstream>
#include <boost/lexical_cast.hpp>
#include <yaml-cpp/yaml.h>
#include "../vity/log.h"
#include <vector>
#include <list>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <functional>

namespace vity{

/*

约定优于配置 ---- 在程序中约定哪些是可以通过配置文件进行修改
            ---- 只有程序中调用了Lookup 规定的配置参数  那些配置参数才可以起效果
vity::ConfigVar<int>::ptr g_int_value_config =
vity::Config::Lookup("system.port", (int)8080, "system port"); --- 约定 --- 创建 system.port名字命名的配置项

    ConfigVarBase : 配置项基类
        - ConfigVarBase         构造函数
        - ~ConfigVarBase        析构函数
        - getName               获取配置项目名字
        - getDescription        获取配置项描述信息
        - toString      virtual 配置项值序列化成字符   (由派生类实现)
        - fromString    virtual 将字符序列化成配置项值 (由派生类实现)
        --------------------------
        + name
        + description
    
    ConfigVar<T> : 具体的配置项类 继承自 ConfigVarBase
        - ConfigVar
        - toString
        - fromString
        - getValue
        - setValue
        -----------------------------------------
        T v 

    Config: 具体的配置类
        - Lookup(name,default_value,description) 找 如果没有找到 就创建
        - Lookup(name) 找
        - LoadFromYaml static ;
        - LookupBase static;
        --------------------------------------------------------------
        typedef std::map<std::string, ConfigVarBase::ptr> ConfigVarMap;
        static ConfigVarMap s_datas;
*/
/*
    配置文件: 名字 值 描述
    名字 描述: std::string
    值: 多种类型  模板
*/

class ConfigVarBase{
public:
    typedef std::shared_ptr<ConfigVarBase> ptr;
    ConfigVarBase(const std::string& name,const std::string& description = "")
    :m_name(name)
    ,m_description(description)
    {
        // 转换为小写
        std::transform(m_name.begin(),m_name.end(),m_name.begin(),::tolower);
    }

    virtual ~ConfigVarBase(){} // 防止无法调用子类的析构函数

    const std::string& getName() const { return m_name; }
    const std::string& getDescription() const { return m_description; }

    // 配置项值序列化成字符
    virtual std::string toString() = 0;
    // 将字符序列化成配置项值
    virtual bool fromString(const std::string& val) = 0;
    // 用于调试的 --- 提示信息
    virtual std::string getTypeName() const = 0;

protected:
    std::string m_name;
    std::string m_description;
};

//F: from_type T to_type
// 转换的模板
// 仿函数本质就是类重载了一个operator()，创建一个行为类似函数的对象

/*
struct MyPlus{
int operator()(const int &a , const int &b) const{
    return a + b;
    }
};
int main()
{
    MyPlus a;
    // MyPlus() 临时对象
    cout << MyPlus()(1,2) << endl;//1、通过产生临时对象调用重载运算符
    cout << a.operator()(1,2) << endl;//2、通过对象显示调用重载运算符
    cout << a(1,2) << endl;//3、通过对象类似函数调用 隐示地调用重载运算符
    return 0;
}
*/
// 仿函数
template<class F,class T>
class LexicalCast{
public:
    T operator()(const F& v){
        // boost中的类型转换  此时即可以支持基本类型的相互转换
        return boost::lexical_cast<T>(v);
    }
};
// 偏特化  对stl中容器类型进行偏特化处理
template<class T>
class LexicalCast<std::string,std::vector<T> >{
public:
    std::vector<T> operator()(const std::string& v){
        YAML::Node node = YAML::Load(v);
        // 模板在实例化之前 并不知道std::vector<T> 是什么类型
        // 使用typename 可以让定义确定下来
        typename std::vector<T> vec;
        std::stringstream ss;
        for(size_t i =  0; i < node.size();++i){
            ss.str("");
            ss << node[i];
            // vector<vector<T> >
            // T vector<T>
            vec.push_back(LexicalCast<std::string,T>()(ss.str()));
        }
        return vec;
    }
};

template<class T>
class LexicalCast<std::vector<T>,std::string >{
public:
    std::string operator()(const std::vector<T>& v){
        YAML::Node node;
        // 把vector中的内容全部取出  转成std::string 
        // YAML::Load(std::string)  将 T convert std::string -----> Load
        for(auto& i : v){
            node.push_back(YAML::Load(LexicalCast<T,std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

template<class T>
class LexicalCast<std::string,std::list<T> >{
public:
    std::list<T> operator()(const std::string& v){
        YAML::Node node = YAML::Load(v);
        typename std::list<T> vec;
        std::stringstream ss;
        for(size_t i =  0; i < node.size();++i){
            ss.str("");
            ss << node[i];
            vec.push_back(LexicalCast<std::string,T>()(ss.str()));
        }
        return vec;
    }
};

template<class T>
class LexicalCast<std::list<T>,std::string >{
public:
    std::string operator()(const std::list<T>& v){
        YAML::Node node;
        for(auto& i : v){
            node.push_back(YAML::Load(LexicalCast<T,std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

template<class T>
class LexicalCast<std::string,std::set<T> >{
public:
    std::set<T> operator()(const std::string& v){
        YAML::Node node = YAML::Load(v);
        typename std::set<T> vec;
        std::stringstream ss;
        for(size_t i =  0; i < node.size();++i){
            ss.str("");
            ss << node[i];
            vec.insert(LexicalCast<std::string,T>()(ss.str()));
        }
        return vec;
    }
};

template<class T>
class LexicalCast<std::set<T>,std::string >{
public:
    std::string operator()(const std::set<T>& v){
        YAML::Node node;
        for(auto& i : v){
            node.push_back(YAML::Load(LexicalCast<T,std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

template<class T>
class LexicalCast<std::string,std::unordered_set<T> >{
public:
    std::unordered_set<T> operator()(const std::string& v){
        YAML::Node node = YAML::Load(v);
        typename std::unordered_set<T> vec;
        std::stringstream ss;
        for(size_t i =  0; i < node.size();++i){
            ss.str("");
            ss << node[i];
            vec.insert(LexicalCast<std::string,T>()(ss.str()));
        }
        return vec;
    }
};

template<class T>
class LexicalCast<std::unordered_set<T>,std::string >{
public:
    std::string operator()(const std::unordered_set<T>& v){
        YAML::Node node;
        for(auto& i : v){
            node.push_back(YAML::Load(LexicalCast<T,std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

template<class T>
class LexicalCast<std::string,std::map<std::string,T> >{
public:
    std::map<std::string,T> operator()(const std::string& v){
        YAML::Node node = YAML::Load(v);
        typename std::map<std::string,T> vec;
        std::stringstream ss;
        for(auto it = node.begin();
                it != node.end(); ++it) {
            ss.str("");
            ss << it->second;
            vec.insert(std::make_pair(it->first.Scalar(),
                        LexicalCast<std::string, T>()(ss.str())));
        }
        return vec;
    }
};

template<class T>
class LexicalCast<std::map<std::string,T>,std::string>{
public:
    std::string operator()(const std::map<std::string,T>& v){
        YAML::Node node;
        for(auto& i : v) {
            node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

template<class T>
class LexicalCast<std::string,std::unordered_map<std::string,T> >{
public:
    std::unordered_map<std::string,T> operator()(const std::string& v){
        YAML::Node node = YAML::Load(v);
        typename std::unordered_map<std::string,T> vec;
        std::stringstream ss;
        for(auto it = node.begin();
                it != node.end(); ++it) {
            ss.str("");
            ss << it->second;
            vec.insert(std::make_pair(it->first.Scalar(),
                        LexicalCast<std::string, T>()(ss.str())));
        }
        return vec;
    }
};

template<class T>
class LexicalCast<std::unordered_map<std::string,T>,std::string>{
public:
    std::string operator()(const std::unordered_map<std::string,T>& v){
        
        YAML::Node node;
        for(auto& i : v) {
            node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

// FromStr T operator()(const std::string& )
// ToStr std::string operator()(const T&)
template<class T,class FromStr=LexicalCast<std::string,T>
            ,class ToStr=LexicalCast<T,std::string> >
class ConfigVar : public ConfigVarBase{
public:
    typedef std::shared_ptr<ConfigVar> ptr;
    typedef std::function<void (const T& old_value,const T& new_value)> on_change_cb;

    ConfigVar(const std::string& name,
            const T& default_val,
            const std::string& description = ""
            )
            :ConfigVarBase(name,description)
            ,m_val(default_val){
    }
    
    // 转成string
    std::string toString() override
    {
        try{
            return ToStr()(m_val);
        }catch(std::exception& e){
            //typeid用于返回指针或引用所指对象的实际类型
            VITY_LOG_ERROR(VITY_LOG_ROOT()) << "ConfigVar::toString exception"
                << e.what() << " convert: "<< typeid(m_val).name() << "to string";
        }
        return "";
    }
    //从string 转为T
    bool fromString(const std::string& val) override
    {
        try{
           setValue(FromStr()(val));
        }catch(std::exception& e){
            //typeid用于返回指针或引用所指对象的实际类型
            VITY_LOG_ERROR(VITY_LOG_ROOT())<< "ConfigVar::toString exception"
                << e.what() << " convert: to string"<< typeid(m_val).name();
        }
        return false;
    }

    std::string getTypeName() const override { return typeid(T).name(); }

    const T getValue() const { return m_val; }

    // setValue的使用到了比较运算
    void setValue(const T& v){ 
        if(v == m_val) 
            return;

        // 事件通知 --- 会通知到所有
        for(auto& i : m_cbs){
            i.second(m_val,v);
        }

        m_val = v;
    }

    void addListener(uint64_t key,on_change_cb cb){
        m_cbs[key] = cb;
    }

    void delListener(uint64_t key){
        m_cbs.erase(key);
    }

    on_change_cb getListener(uint64_t key){
        auto it = m_cbs.find(key);
        return it == m_cbs.end() ? nullptr : it->second;
    }

    void clearListener(){
        m_cbs.clear();
    }

private:
    T  m_val;
    // 变更回调函数组 uint64_t key 要求唯一
    std::map<uint64_t,on_change_cb> m_cbs;
};

// 约定优于配置
class Config{
public:
    typedef std::map<std::string, ConfigVarBase::ptr> ConfigVarMap;

    // typename：指出模板声明（或定义）中的非独立名称（dependent names）是类型名，而非变量名
    // 如果存在 就直接返回 如果不存在  就创建
    template<class T>
    static typename ConfigVar<T>::ptr Lookup(const std::string& name,
            const T& default_value,const std::string& description = "")
    {
        auto it = GetDatas().find(name);
        if(it != GetDatas().end()) // 找到了
        {
            auto tmp = std::dynamic_pointer_cast<ConfigVar<T> >(it->second);

            if(tmp){
                VITY_LOG_INFO(VITY_LOG_ROOT())<<"Lookup name="<<name<<"exists";
                return tmp;
            }else{
                VITY_LOG_ERROR(VITY_LOG_ROOT())<<"Lookup name= "<<name<<"exists but type not "
                << typeid(T).name() << "  real type is  " << it->second->getTypeName()
                << " " << it->second->toString();
                return nullptr;
            }
        }

        // 检查名字是否合法
        if(name.find_first_not_of("abcdefghijklmnopqrstuvwxyz._0123456789")
            != std::string::npos)
            {
                VITY_LOG_ERROR(VITY_LOG_ROOT())<<"Lookup name invalid"<<name;
                throw std::invalid_argument(name);
            }
        typename ConfigVar<T>::ptr v(new ConfigVar<T>(name,default_value,description));
        GetDatas()[name] = v;
        return v;
    }

    template<class T>
    static typename ConfigVar<T>::ptr Lookup(const std::string& name)
    {
        auto it = GetDatas().find(name);
        if(it != GetDatas().end()){
            return std::dynamic_pointer_cast<ConfigVar<T> >(it->second);
        }
        return nullptr;
    }

    static void LoadFromYaml(const YAML::Node& root);
    static ConfigVarBase::ptr LookupBase(const std::string& name);

private:
    // 类成员的静态初始化与静态成员函数的初始化可能冲突  相互依赖的问题
    //https://www.cnblogs.com/hangj/p/6567724.html
    static ConfigVarMap& GetDatas(){
        static ConfigVarMap s_datas;
        return s_datas;
    }
};

}

#endif