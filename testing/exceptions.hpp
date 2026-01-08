#pragma once
#include <stdexcept>
#ifndef STATIC_PARSER_NO_HEAP
#include <string>
#endif

class raw_string_exception : std::exception {
    private :
    const char* msg;
    public :
    raw_string_exception(const char* err_msg) : std::exception(), msg(err_msg) {}
    const char* what() const noexcept { return msg; }
};

class string_exception : std::exception {
    private :
    std::string msg;
    public :
    string_exception(const std::string& err_msg) : msg(err_msg), std::exception() {}
};

class comtime_except : raw_string_exception {
    public :
    comtime_except(const char* err_msg) : raw_string_exception(err_msg) {};
};

#ifdef STATIC_PARSER_NO_HEAP
class ParseError : raw_string_exception {
    public :
    ParseError(const char* err_msg) : raw_string_exception(err_msg) {};
};

class SetupError : raw_string_exception {
    public :
    SetupError(const char* err_msg) : raw_string_exception(err_msg) {}
};

#else

class ParseError : string_exception {
    public :
    ParseError(const std::string& err_msg) : string_exception(err_msg) {};
    
};

class SetupError : string_exception {
    public :
    SetupError(const std::string& err_msg) : string_exception(err_msg) {};
};

#endif
