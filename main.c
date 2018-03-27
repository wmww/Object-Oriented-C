#include <stdio.h>
#include <stdlib.h>
#include "define_EVIL/define_EVIL.h"

#define COMMA() ,

#define func_INTERNAL(type, name, arg_types_and_types, arg_names) \
	type name##_WRAPPED(EXPAND arg_types_and_types, void *); \
	type name arg_types_and_types \
	{ \
		type ret = name##_WRAPPED(EXPAND arg_names, NULL); \
		return ret; \
	} \
	type name##_WRAPPED(EXPAND arg_types_and_types, void * SCOPE) \

#define BOTH(a, b) a b
#define SECOND(a, b) b

#define GET_ARG_TYPES_AND_NAMES(item, i) () BOTH item
#define GET_ARG_NAMES(item, i) () SECOND item

#define func(type, name, ...) \
	func_INTERNAL( \
		type, \
		name, \
		(EXPAND MAP((GET_ARG_TYPES_AND_NAMES, COMMA), __VA_ARGS__)), \
		(EXPAND MAP((GET_ARG_NAMES, COMMA), __VA_ARGS__)))

#define make(type, name) \
	struct type * name##_OBJ = malloc(sizeof(struct type))

#define set(obj, prop, val) \
	obj##_OBJ->prop = val

#define get(obj, prop) \
	obj##_OBJ->prop

struct TwoVals
{
	int a;
	int b;
};

func(int, add_nums, (int, foo), (int, bar))
{
	make(TwoVals, vals);
	set(vals, a, foo);
	set(vals, b, bar);
	return get(vals, a) + get(vals, b);
}

int main()
{
	printf("hello world\n");
	printf("%d\n", add_nums(4, 7));
	return 0;
}