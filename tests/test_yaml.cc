
#include <yaml-cpp/yaml.h>
#include "../vity/config.h"
#include "../vity/log.h"

#if 0
vity::ConfigVar<int>::ptr g_int_value_config =
vity::Config::Lookup("system.port", (int)8080, "system port");

vity::ConfigVar<float>::ptr g_int_valuex_config =
vity::Config::Lookup("system.port", (float)8080, "system port");

vity::ConfigVar<float>::ptr g_float_value_config =
    vity::Config::Lookup("system.value", (float)10.2f, "system value");

vity::ConfigVar<std::vector<int> >::ptr g_int_vector_value_config = 
    vity::Config::Lookup("system.int_vec",std::vector<int>{1,2},"system int vec");

vity::ConfigVar<std::list<int> >::ptr g_int_list_value_config = 
    vity::Config::Lookup("system.int_list",std::list<int>{1,2},"system int list");

vity::ConfigVar<std::set<int> >::ptr g_int_set_value_config = 
vity::Config::Lookup("system.int_set",std::set<int>{1,2},"system int set");

vity::ConfigVar<std::unordered_set<int> >::ptr g_int_uset_value_config = 
    vity::Config::Lookup("system.int_uset",std::unordered_set<int>{1,2},"system int uset");

vity::ConfigVar<std::map<std::string,int> >::ptr g_str_int_map_value_config = 
vity::Config::Lookup("system.str_int_map",std::map<std::string,int>{{"k1",2},},"system int map");

vity::ConfigVar<std::map<std::string,int> >::ptr g_str_int_umap_value_config = 
vity::Config::Lookup("system.str_int_umap",std::map<std::string,int>{{"k1",2},{"k2",3},{"k3",4}},"system int umap");

/*
  bool IsDefined() const;
  bool IsNull() const { return Type() == NodeType::Null; }
  bool IsScalar() const { return Type() == NodeType::Scalar; }
  bool IsSequence() const { return Type() == NodeType::Sequence; }
  bool IsMap() const { return Type() == NodeType::Map; }


    对象：键值对的集合，又称为映射（mapping）/ 哈希（hashes） / 字典（dictionary）
    数组：一组按次序排列的值，又称为序列（sequence） / 列表（list）
    纯量（scalars）：单个的、不可再分的值

    enum value { Undefined, Null, Scalar, Sequence, Map };
    Sequence：是无法获取结果的 Sequence----> Map -----> Scalar
    Map ---> key value
*/
void print_yaml(const YAML::Node& node,int level){
    if(node.IsScalar()){
        VITY_LOG_INFO(VITY_LOG_ROOT())<<std::string(level*4,' ')
            << node.Scalar() << " - "<< node.Type() <<" - "<< level;
    }else if(node.IsNull()){
        VITY_LOG_INFO(VITY_LOG_ROOT())<<std::string(level*4,' ')
            << "NULL - " << node.Type() <<" - "<< level;
    }else if(node.IsMap()){
        for(auto it = node.begin();it != node.end();++it){
            VITY_LOG_INFO(VITY_LOG_ROOT())<<std::string(level*4,' ')
                << it->first << " - " << it->second.Type() <<" - "<< level;
            print_yaml(it->second,level + 1);
        }
    }else if(node.IsSequence()){
        for(size_t i = 0; i < node.size();++i){
            VITY_LOG_INFO(VITY_LOG_ROOT())<<std::string(level*4,' ')
                << i << " - " << node[i].Type() <<" - "<< level;
            print_yaml(node[i],level + 1);
        }
    }
}

void test_config(){

    VITY_LOG_INFO(VITY_LOG_ROOT()) << "before: " << g_int_value_config->getValue();
    VITY_LOG_INFO(VITY_LOG_ROOT()) << "before: " << g_float_value_config->toString();

#define XX(g_var,name,prefix) \
    do{ \
       auto& v = g_var->getValue(); \
        for(auto& i : v) {\
            VITY_LOG_INFO(VITY_LOG_ROOT()) << #prefix " " #name ": " << i; \
        } \
        VITY_LOG_INFO(VITY_LOG_ROOT()) << #prefix " " #name " yaml: " << g_var->toString(); \
    } while (0);

#define XX_M(g_var, name, prefix) \
    do{ \
        auto& v = g_var->getValue(); \
        for(auto& i : v) { \
            VITY_LOG_INFO(VITY_LOG_ROOT()) << #prefix " " #name ": {" \
                    << i.first << " - " << i.second << "}"; \
        } \
        VITY_LOG_INFO(VITY_LOG_ROOT()) << #prefix " " #name " yaml: " << g_var->toString(); \
    } while (0);

    XX(g_int_vector_value_config,int_vec,before)
    XX(g_int_list_value_config,int_list,before)
    XX(g_int_set_value_config,int_set,before)
    XX(g_int_uset_value_config,int_uset,before)
    XX_M(g_str_int_map_value_config, int_map, before)
    XX_M(g_str_int_umap_value_config, int_umap, before)

    YAML::Node root = YAML::LoadFile("/home/fredzhan/vity/bin/conf/test.yaml");
    //print_yaml(root,0);

    vity::Config::LoadFromYaml(root);

    VITY_LOG_INFO(VITY_LOG_ROOT()) << "after: " << g_int_value_config->getValue();
    VITY_LOG_INFO(VITY_LOG_ROOT()) << "after: " << g_float_value_config->toString();

    XX(g_int_vector_value_config,int_vec,after)
    XX(g_int_list_value_config,int_list,after)
    XX(g_int_set_value_config,int_set,after)
    XX(g_int_uset_value_config,int_uset,after)
    XX_M(g_str_int_map_value_config, int_map, after)
    XX_M(g_str_int_umap_value_config, int_umap, before)
}

#endif

class Person{
public:
    Person(){}

    std::string m_name;
    int m_age = 0;
    bool m_sex = 0;

    std::string toString() const{
        std::stringstream ss;
        ss << "[Person name=" << m_name
           << " age=" << m_age
           << " sex=" << m_sex
           << "]";
        return ss.str();
    }
    bool operator==(const Person& oth) const {
        return m_name == oth.m_name
            && m_age == oth.m_age
            && m_sex == oth.m_sex;
    }  
};
namespace vity{
template<>
class LexicalCast<std::string,Person >{
public:
    Person operator()(const std::string& v){
        YAML::Node node = YAML::Load(v);
        Person p;
        p.m_name = node["name"].as<std::string>();
        p.m_age = node["age"].as<int>();
        p.m_sex = node["sex"].as<bool>();
        return p;
    }
};

template<>
class LexicalCast<Person,std::string>{
public:
    std::string operator()(const Person& p){
        std::stringstream ss;
        YAML::Node node;
        node["name"] = p.m_name;
        node["age"] = p.m_age;
        node["sex"] = p.m_sex;
        ss << node;
        return ss.str();
    }
};
}

vity::ConfigVar<Person>::ptr g_person =
    vity::Config::Lookup("class.person", Person(), "system person");

vity::ConfigVar<std::map<std::string, Person> >::ptr g_person_map =
    vity::Config::Lookup("class.map", std::map<std::string, Person>(), "system person");

vity::ConfigVar<std::map<std::string, std::vector<Person> > >::ptr g_person_vec_map =
    vity::Config::Lookup("class.vec_map", std::map<std::string, std::vector<Person> >(), "system person");

void test_class()
{
    VITY_LOG_INFO(VITY_LOG_ROOT())<< "before: "<< g_person->getValue().toString() << "-"<< g_person->toString();

#define XX_PM(g_var, prefix) \
    { \
        auto m = g_var->getValue(); \
        for(auto& i : m) { \
            VITY_LOG_INFO(VITY_LOG_ROOT()) <<  prefix << ": " << i.first << " - " << i.second.toString(); \
        } \
        VITY_LOG_INFO(VITY_LOG_ROOT()) <<  prefix << ": size=" << m.size(); \
    }


    XX_PM(g_person_map, "class.map before");
    YAML::Node root = YAML::LoadFile("/home/fredzhan/vity/bin/conf/test.yaml");
    vity::Config::LoadFromYaml(root);
    VITY_LOG_INFO(VITY_LOG_ROOT())<< "after: "<< g_person->getValue().toString() << "-"<< g_person->toString();
    XX_PM(g_person_map, "class.map after");
}

void test_log()
{
    static vity::Logger::ptr system_log = VITY_LOG_NAME("system");
    VITY_LOG_INFO(system_log) << "hello system" << std::endl;
    std::cout << vity::LoggerMgr::GetInstance()->toYamlString() << std::endl;
    YAML::Node root = YAML::LoadFile("/home/fredzhan/vity/bin/conf/log.yaml");
    vity::Config::LoadFromYaml(root);
    std::cout << "=============" << std::endl;
    std::cout << vity::LoggerMgr::GetInstance()->toYamlString() << std::endl;
    std::cout << "=============" << std::endl;
    std::cout << root << std::endl;
    VITY_LOG_INFO(system_log) << "hello system" << std::endl;

    system_log->setFormatter("%d - %m%n");
    VITY_LOG_INFO(system_log) << "hello system" << std::endl;
}

int main()
{
    test_log();
    return 0;
}