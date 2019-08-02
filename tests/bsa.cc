#include <boost/static_assert.hpp>

class Timestamp
{
private:
	int64_t microSecondsSinceEpoch_;
};

/* 编译时确保类Timestamp和int64_t大小相等  */
BOOST_STATIC_ASSERT(sizeof(Timestamp) == sizeof(int64_t));

/* int大小和short不同, 编译时不通过 */
//BOOST_STATIC_ASSERT(sizeof(int) == sizeof(short));    // 编译时会计算大小

int main(void)
{
	return 0;
}
