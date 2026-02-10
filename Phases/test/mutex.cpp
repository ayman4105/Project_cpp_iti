#include<iostream>
#include<thread>
#include<mutex>
#include <future>


int counter = 0;
std::mutex mtx;
int incerment(int n){
    for(int i =0;i<n;i++){
        
        std::lock_guard<std::mutex> lock(mtx); // auto lock & unlock    
        
        counter++;
    }
    return counter;
}


int main(void){


    std::thread t1(incerment , 8000);
    std::thread t2(incerment , 80000);
    std::packaged_task<int(int)> t1(increment);
    
   std::cout<<"counter"<<std::endl;

    t1.join();
    t2.join();


    std::cout<<counter<<std::endl;

    return 0;
}