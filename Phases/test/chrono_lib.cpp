#include<iostream>
#include<chrono>
#include<ostream>
#include<thread>


int main(){
    std::chrono::system_clock::time_point t1 = std::chrono::system_clock::now();

    std::this_thread::sleep_for(std::chrono::seconds(5));

    std::chrono::system_clock::time_point t2 = std::chrono::system_clock::now();
    std::chrono::duration<double> time_span = t2 - t1;
    std::cout << "time_span.count() : " << time_span.count() << std::endl;

     return 0;
}