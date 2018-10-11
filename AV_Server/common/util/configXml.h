#ifndef _RAPID_XML_H_
#define _RAPID_XML_H_

#include <iostream>
#include <map>
#include <string>
#include "../../thirdpart/include/rapidxml/rapidxml.hpp"
#include "../../thirdpart/include/rapidxml/rapidxml_utils.hpp"
#include "../../thirdpart/include/rapidxml/rapidxml_print.hpp"
#include "LibSingleton.h"

using namespace rapidxml;
using std::map;
using std::string;
using std::cout;
using std::endl;

typedef map<string, string> myConfigType;
typedef map<string, string>::iterator myConfigIter;
typedef std::pair<string, string> myConfigPair;

class ConfigXml: public CSingleton<ConfigXml>
{
public:
    ConfigXml();
    ~ConfigXml();

    int init(string fileName = "../conf/config.xml");
    string getValue(string keyBelong, string key);
    bool appendValue(string keyBelong, string key, string value);
    bool modifyValue(string keyBelong, string key, string value);
    void print() const;
    void display();

private:
    myConfigType m_map_data;
    string m_data;
    string m_sConfigFileName;
};


#endif
