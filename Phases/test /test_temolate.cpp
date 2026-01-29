#include<iostream>


class car{
    public:
    car(){
        std::cout << "car constructor" << std::endl;
    }
};

template<typename T>

T add(T a, T b){
    return a + b;
}

template<typename T>

T sub(T a, T b){
    return a - b;
}

template <>
car sub(car a, car b){
    std::cout << "car sub" << std::endl;
    return a;
}

//template int sub(int a, int b);

template <int size>
int func(int (&ref)[size]){
    for(int i = 0; i < size; i++){
        std::cout << ref[i] << std::endl;
    }
    return 0;
}



int arr[5] {1,2,3,4,5};

int (&ref)[5] = arr;

int &ref2 = arr[0];

//lamda expresion 
auto lambda = [](int a, int b)->int { return a + b; };


int main(){
    int w = 6;
    int r = 7;
    int h = [](int x, int y)->int { if(x>y) return x;
                                    else return y; }(w,r);

    auto u = lambda(w,r);
    // auto u = h(w,r);   
    std::cout << h << std::endl;
    std::cout << u << std::endl;

    // std::cout <<ref2+7 << std::endl;
    // std::cout <<ref<<std::endl;
    // std::cout << func<10>() << std::endl;
    // std::cout << add(1,2) << std::endl;
    // std::cout << sub(1,2) << std::endl;
    return 0;
}
