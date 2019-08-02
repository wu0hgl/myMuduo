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

	Y* r = p->f2();				// ������ָ��
	assert(p == q);
	assert(p.get() == r);

	std::cout<<p.use_count()<<std::endl;
	boost::shared_ptr<Y> s(r);	// ����ʹ����ָ�뿽������һ���µ�Y����, ������ʹ��22�е�Y����
	std::cout<<s.use_count()<<std::endl;
	assert(p.use_count() == s.use_count());

	return 0;
}

