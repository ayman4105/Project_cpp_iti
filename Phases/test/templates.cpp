#include <initializer_list>
#include <iostream>
#include <memory>
#include <typeinfo>
#include <vector>



template<typename T , typename C , typename A>

T add(A a ,C b){
    return a+b;
}


class car {
    float i ;
    public:
     car (float val) : i{val}{
        std::cout<<"rrrr"<<std::endl;
     } 
    car operator+(car &other){

        return(i + other.i);
    }

};

   

 
int main(){

    // car x = add<car , int , float>(5,(int)10.5);

    //ex for lambda with templates
    auto lambda = [](int a, int b) { return a + b; };
    std::cout << lambda(5, 10) << std::endl;
    [](){}();


    return 0;
}