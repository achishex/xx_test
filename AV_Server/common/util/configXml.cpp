#include "configXml.h"

ConfigXml::ConfigXml() :m_sConfigFileName(std::string("../../conf/config.xml"))
{
}

ConfigXml::~ConfigXml()
{
    //...
}

int ConfigXml::init(string fileName)
{
    m_sConfigFileName = fileName;
    try {
        rapidxml::file<> file(m_sConfigFileName.c_str());
        m_data = file.data();

        rapidxml::xml_document<> doc;
        doc.parse<0>(file.data());
        //获取根节点
        rapidxml::xml_node<char>* rootNode = doc.first_node();

        //遍历所有节点
        string keyBelong = "";
        string key = "";
        string name = "";
        string value = "";
        for(rapidxml::xml_node<char>* node=rootNode->first_node(); node!=NULL; node=node->next_sibling())
        {
            for(rapidxml::xml_node<char>* subNode=node->first_node(); subNode!=NULL; subNode=subNode->next_sibling())
            {
                keyBelong = node->name();
                key = subNode->name();
                name = keyBelong + "." + key;
                value = subNode->value();
                ConfigXml::m_map_data.insert(myConfigPair(name, value));
            }
        }
    }
    catch( const std::exception& ex )
    {
        std::cerr << "parse file: " << m_sConfigFileName << ", exception, err: %s" << ex.what() << std::endl;
        return -1;
    }
    catch( ... )
    {
        std::cerr << "parse file: " << m_sConfigFileName << ", exception " << std::endl;
        return -2;
    }
    return 0;
}

string ConfigXml::getValue(string keyBelong, string key)
{
    string ret = "";
    if ( keyBelong.empty() || key.empty() )
    {
        return ret;
    }

    string name = keyBelong + "." + key;
    myConfigIter it = m_map_data.find(name);
    if ( it == m_map_data.end() )
    {
        return ret;
    }

    ret = it->second;
    return ret;
}


void ConfigXml::print() const
{
    cout << m_data << endl;
}

void ConfigXml::display()
{
    for ( myConfigIter it=m_map_data.begin(); it!=m_map_data.end(); ++it )
    {
        cout << it->first << ": " << it->second << endl;
    }
}


bool ConfigXml::appendValue(string keyBelong, string key, string value)
{
    if (keyBelong.empty() || key.empty() || value.empty() )
    {
        return false;
    }

    rapidxml::file<> file(m_sConfigFileName.c_str());
    rapidxml::xml_document<> doc;
    doc.parse<0>(file.data());

    //获取根节点
    rapidxml::xml_node<char>* rootNode = doc.first_node();

    for(rapidxml::xml_node<char>* node=rootNode->first_node(); node!=NULL; node=node->next_sibling())
    {
        string belong(node->name());
        if (belong == keyBelong )
        {
            for(rapidxml::xml_node<char>* subNode=node->first_node(); subNode!=NULL; subNode=subNode->next_sibling())
            {
                string subKey(subNode->name());
                if ( subKey == key )
                {
                    cout << "the config has the same node <" << keyBelong << "> -> <" << subKey << ">" << endl;
                    return false;
                }
            }

            node->append_node(doc.allocate_node(node_element, key.c_str(), value.c_str()));
            cout << "add config node <" << keyBelong << "> -> <" << key << ">: " << value << endl;

            string name = keyBelong + "." + key;
            ConfigXml::m_map_data.insert(myConfigPair(name, value));

            m_data = "";
            rapidxml::print(std::back_inserter(m_data), doc, 0);
     		std::ofstream out(m_sConfigFileName.c_str());
			out << doc;
            return true;
        }
    }

    rootNode->append_node(doc.allocate_node(node_element, keyBelong.c_str(), NULL));
    rootNode->last_node()->append_node(doc.allocate_node(node_element, key.c_str(), value.c_str()));
    cout << "add config node <" << keyBelong << "> -> <" << key << ">: " << value << endl;

    string name = keyBelong + "." + key;
    ConfigXml::m_map_data.insert(myConfigPair(name, value));

    m_data = "";
    rapidxml::print(std::back_inserter(m_data), doc, 0);
    std::ofstream out(m_sConfigFileName.c_str());
	out << doc;
    return true;
}

bool ConfigXml::modifyValue(string keyBelong, string key, string value)
{
    if (keyBelong.empty() || key.empty() || value.empty())
    {
        return false;
    }

    rapidxml::file<> file(m_sConfigFileName.c_str());
    rapidxml::xml_document<> doc;
    doc.parse<0>(file.data());

    //获取根节点
    rapidxml::xml_node<char>* rootNode = doc.first_node();

    for(rapidxml::xml_node<char>* node=rootNode->first_node(); node!=NULL; node=node->next_sibling())
    {
        string belong(node->name());
        if (belong == keyBelong )
        {
            for(rapidxml::xml_node<char>* subNode=node->first_node(); subNode!=NULL; subNode=subNode->next_sibling())
            {
                string subKey(subNode->name());
                if ( subKey == key )
                {
                    //rapidxml::xml_node<char>* tempNode = subNode->previous_sibling();
                    node->remove_node(subNode);
                    if ( rootNode->last_node() )
                    {
                        node->append_node(doc.allocate_node(node_element, key.c_str(), value.c_str()));
                        cout << "modify node: " << keyBelong << " -> " << key << ", value via \"" << subNode->value() << "\" to \"" << value << "\"" << endl;

                        string name = keyBelong + "." + key;
                        ConfigXml::m_map_data.erase(name);
                        ConfigXml::m_map_data.insert(myConfigPair(name, value));
                    }

                    m_data = "";
                    rapidxml::print(std::back_inserter(m_data), doc, 0);
                    std::ofstream out(m_sConfigFileName.c_str());
                    out << doc;
                    return true;
                }
            }
        }
    }

    cout << "not find the node of " << keyBelong << " -> " << key << endl;

    rootNode->append_node(doc.allocate_node(node_element, keyBelong.c_str(), NULL));
    rootNode->last_node()->append_node(doc.allocate_node(node_element, key.c_str(), value.c_str()));
    cout << "add config node <" << keyBelong << "> -> <" << key << ">: " << value << endl;

    string name = keyBelong + "." + key;
    ConfigXml::m_map_data.insert(myConfigPair(name, value));

    m_data = "";
    rapidxml::print(std::back_inserter(m_data), doc, 0);
    std::ofstream out(m_sConfigFileName.c_str());
	out << doc;
    return true;
}
