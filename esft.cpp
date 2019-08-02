#include <iostream>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include <cassert>

class Y: public boost::enable_shared_from_this<Y>
{
public:
	boost::shared_ptr<Y> f()
	{
		return shared_from_this();
	}

	Y* f2()
	{
		return this;
	}
};

int main()
{
	boost::shared_ptr<Y> p(new Y);
	boost::shared_ptr<Y> q = p->f();

	Y* r = p->f2();				// 返回裸指针
	assert(p == q);
	assert(p.get() == r);

	std::cout<<p.use_count()<<std::endl;
	boost::shared_ptr<Y> s(r);	// 这里使用裸指针拷贝构造一个新的Y对象, 而不是使用22行的Y对象
	std::cout<<s.use_count()<<std::endl;
	assert(p.use_count() == s.use_count());

	return 0;
}

