#include<iostream>



class car
{
private:
    float i;
public:
    car(float i){};
    car operator+(car &a){
        return i+a.i;
    }
    ~car();
};



template<typename T,typename A , typename B>
T add(A x , B y){
    return x + y;
}


int main(){

    car c1(1);
    car c2(2);
   car c3 =  add<car , car , car>(c1,c2);

    return 0;
}