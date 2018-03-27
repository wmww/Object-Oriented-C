#include <stdio.h>
#include <stdlib.h>
#include "define_EVIL/define_EVIL.h"

struct _Scope
{
	void * data;
	void (*drop)(void*);
	struct _Scope* next;
};

void * make_obj(void * data, void(*drop)(void*), struct _Scope ** scope)
{
	struct _Scope * tmp = *scope;
	*scope = malloc(sizeof(struct _Scope));
	(*scope)->data = data;
	(*scope)->drop = drop;
	(*scope)->next = tmp;
	return data;
}

#define COMMA() (),

#define CHECK_VOID_void
#define SPEW_A_THING(...) __VA_ARGS__
#define SPEW_NOTHING(...)

#define func_INTERNAL(type, name, a, b, c) \
	type name##_WRAPPED(EXPAND b struct _Scope **); \
	type name c \
	{ \
		struct _Scope* scope = NULL; \
		EXPAND_CAT(SPEW_, CHECK_IF_THING(CHECK_VOID_##type)) (type ret =) name##_WRAPPED(EXPAND a &scope); \
		while (scope) \
		{ \
			scope->drop(scope->data); \
			scope = scope->next; \
		} \
		EXPAND_CAT(SPEW_, CHECK_IF_THING(CHECK_VOID_##type)) (return ret;) \
	} \
	type name##_WRAPPED(EXPAND b struct _Scope ** _scope) \

#define BOTH(a, b) a b
#define SECOND(a, b) b

#define ARGS_A(item, i) SECOND item,
#define ARGS_B(item, i) BOTH item,
#define ARGS_C(item, i) () BOTH item EXPAND

#define func(type, name, ...) \
	func_INTERNAL( \
		type, \
		name, \
		(MAP(ARGS_A, __VA_ARGS__)), \
		(MAP(ARGS_B, __VA_ARGS__)), \
		(EXPAND MAP((ARGS_C, COMMA), __VA_ARGS__)()))

#define make(type, name) \
	printf(#type " created\n"); \
	struct type * name##_OBJ = make_obj(malloc(sizeof(struct type)), &type##_DROP, _scope)

#define set(obj, prop, val) \
	obj##_OBJ->prop = val

#define get(obj, prop) \
	obj##_OBJ->prop

struct TwoVals
{
	int a;
	int b;
};

void TwoVals_DROP(void * data)
{
	printf("TwoVals_DROP()\n");
	free(data);
}

struct MyStruct
{
	int x, y, z;
};

void MyStruct_DROP(void * data)
{
	printf("MyStruct_DROP()\n");
	free(data);
}

func(void, do_nothing)
{
	make(MyStruct, aaa);
}

func(int, add_nums, (int, foo), (int, bar))
{
	make(TwoVals, vals);
	set(vals, a, foo);
	do_nothing();
	set(vals, b, bar);
	make(TwoVals, xyz);
	return get(vals, a) + get(vals, b);
}

int main()
{
	printf("hello world\n");
	printf("%d\n", add_nums(4, 7));
	return 0;
}