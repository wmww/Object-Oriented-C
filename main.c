#include <stdio.h>

struct TwoVals
{
	int a;
	int b;
};

FUNC(int add_nums, (int foo, int bar))
{
	MAKE(TwoVals, vals);
	SET(vals, a, foo);
	SET(vals, b, bar);
	return GET(vals, a) + GET(vals, b)
}

int main()
{
	printf("hello world\n");
	printf("%d\n", add_nums(4, 7));
	return 0;
}