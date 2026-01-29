#include<iostream>
#include<string>
#include<ostream>
#include<vector>
#include<queue>
#include<memory>
#include"fstream"


using std::string;

enum class severity {
    info,
    error,
    warning
};

class logmassage{
private:
    severity s;
    string app_name;
    string context;
    string time;
    string text;

public:
    logmassage(severity s , string app_name , string context , string time , string text);
    friend std::ostream& operator<<(std::ostream& os , const logmassage& log);

};

logmassage::logmassage(severity s_t , string app_name_t , string context_t , string time_t , string text_t)
                    : s{s_t} , app_name{app_name_t}, context{context_t}, time{time_t}, text{text_t}{}


string get_serevity(severity s){
    switch(s){
        case severity::info:
            return "INFO";
        case severity::error:
            return "ERROR";
        case severity::warning:
            return "WARNING";  
        default: return "UNKNOWN";           
    }
}


std::ostream& operator<<(std::ostream& os ,const logmassage& log){
    os <<"[" << log.time<<"]"
        <<"[" <<get_serevity(log.s) << "]"
        <<"["<<log.app_name<<"]"
        <<"["<<log.context<<"]"
        <<log.text;
        
        return os;        
}




class Ilog{
    public:
        virtual void write(const logmassage& msg) = 0;

        virtual ~Ilog() = default;
};


class console : public Ilog{
    public:
        void write(const logmassage& message);
};

void console::write(const logmassage& message){
        std::cout<<message<<std::endl;
}

class Filesink : public Ilog{
    private:
        std::ofstream file;

    public:
        void write(const logmassage& message);
        Filesink(const string& filename); 
        virtual ~Filesink()=default;    
};


Filesink ::Filesink(const string& filename):file(std::ofstream(filename, std::ios::app)){

}

void Filesink :: write(const logmassage& message){
    file << message<< '\n';
}


class logmanger{
    private:
        std::vector<std::unique_ptr<Ilog>> sinks;
        std::vector<logmassage> messages;
    public:
        void add_sink(std::unique_ptr<Ilog> sink);
        void log(const logmassage& message);
        logmanger& operator<<(const logmassage& message);
        logmanger() = default;
        ~logmanger() = default;
};


void logmanger::add_sink(std::unique_ptr<Ilog> sink){
    sinks.push_back(std::move(sink));
}

void logmanger::log(const logmassage& message){
    for(auto &sink : sinks){
        sink->write(message);
    }
}

logmanger& logmanger::operator<<(const logmassage& message){
    std::cout<< "entered operator <<\n";
    this->log(message);
    return *this;
}

int main(){

    logmanger manager;

    manager.add_sink(std::make_unique<console>());
    manager.add_sink(std::make_unique<Filesink>("log.txt"));

    logmassage log1(severity::error, "App1", "Init", "12:00", "Something went wrong");
    logmassage log2(severity::info, "App1", "Run", "12:01", "Application started");

    manager.log(log1);
    manager << log2;

    return 0;
}