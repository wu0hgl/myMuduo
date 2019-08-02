#include <iostream>
#include <boost/function.hpp>
#include <boost/bind.hpp>
using namespace std;

class Foo {
public:
    void memberFunc(double d, int i , int j) {
        cout << d << " ";
        cout << i << " ";
        cout << j << " ";
        cout << endl;
    }
};

int main() {
    Foo foo;
    /* 
     * 将四个参数的函数适配成一个参数的函数 
     * 将一个函数接口转换成另一个函数的接口
     * */
    boost::function<void (int)> fp_1 = boost::bind(&Foo::memberFunc, &foo, 0.5, _1, 10);    // 绑定的成员函数, 第一个参数是this指针
    fp_1(100);

    /* 两个参数 */
    boost::function<void (int, int)> fp_2 = boost::bind(&Foo::memberFunc, &foo, 0.5, _1, _2);
    fp_2(100, 13);      // 相当于: (&foo)->menberFunc(0.5, 100, 13)

    /* 使用boost库把this转换成引用 */
    boost::function<void (int, int)> fp_3 = boost::bind(&Foo::memberFunc, boost::ref(foo), 0.5, _1, _2);
    fp_3(100, 13);      // 相当于: foo.memberFunc(0.5, 100, 13)

    return 0;
}
