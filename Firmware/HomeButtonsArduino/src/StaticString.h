#ifndef HOMEBUTTONS_STATICSTRING_H
#define HOMEBUTTONS_STATICSTRING_H

#include <cstdlib>


// A class to hold a string up to MAX_SIZE-1 characters (or MAX_SIZE characters including \0)
template <size_t MAX_SIZE>
class StaticString
{
public:
    StaticString() {}
    StaticString(const char* str) {
        snprintf(m_data, MAX_SIZE, "%s", str);
    }

    template<typename ... Args>
    StaticString(const char* format, Args ... args) {
        std::snprintf( m_data, MAX_SIZE, format, args ... );
    }

    const char* c_str() const { return m_data; }

    size_t length() const { return strlen(m_data); }
    StaticString substring(size_t i, size_t j) {
        StaticString output;
        if (i < MAX_SIZE) {
            snprintf(output.m_data, std::min(MAX_SIZE, j-i+1), m_data+i);
        }
        return output;
    }

    StaticString operator+(const StaticString& other)
    {
        StaticString output;
        snprintf(output.m_data, MAX_SIZE, "%s%s", m_data, other.m_data);
        return output;
    }

    StaticString operator+(const char* other)
    {
        StaticString output;
        snprintf(output.m_data, MAX_SIZE, "%s%s", m_data, other);
        return output;
    }
private:
    char m_data[MAX_SIZE] = {'\0'};
};

#endif // HOMEBUTTONS_STATICSTRING_H