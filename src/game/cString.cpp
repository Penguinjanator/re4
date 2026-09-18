#include "types.h"
#include "cString.h"

extern "C" {
unsigned int strlen(const char* s);
char* strcpy(char* dst, const char* src);
char* strcat(char* dst, const char* src);
}

cString::cString()
{
    m_str = "";
}

cString::cString(const char* s)
{
    m_str = "";
    if (s != NULL) {
        if (strlen(s) != 0) {
            copy(s);
        }
    }
}

cString::~cString()
{
    clear();
}

cString& cString::operator=(const cString& o)
{
    if (this != &o) {
        clear();
        if (((cString&) o).size() != 0) {
            copy(((cString&) o).c_str());
        }
    }
    return *this;
}

cString& cString::operator+=(const cString& o)
{
    u32 len = size() + ((cString&) o).size();
    char* buf = new char[len + 1];

    strcpy(buf, c_str());
    strcat(buf, ((cString&) o).c_str());
    clear();
    copy(buf);
    delete[] buf;
    return *this;
}

cString& cString::operator+=(const char* s)
{
    cString tmp(s);

    return *this += tmp;
}

char* cString::c_str()
{
    return m_str;
}

u32 cString::size()
{
    return strlen(m_str);
}

void cString::clear()
{
    if (m_str != "") {
        delete[] m_str;
        m_str = "";
    }
}

void cString::copy(const char* s)
{
    char* p = new char[strlen(s) + 1];

    strcpy(p, s);
    m_str = p;
}
