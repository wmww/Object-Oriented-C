#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "define_EVIL/define_EVIL.h"

#define debug EXPAND_TRUE
//#define debug EXPAND_FALSE;

struct _Scope
{
	void * data;
	void (*drop)(void*);
	struct _Scope* next;
};

void * _make_obj(size_t size, void(*drop)(void*), struct _Scope ** scope)
{
	void * data = malloc(size);
	memset(data, 0, size);
	struct _Scope * tmp = *scope;
	*scope = malloc(sizeof(struct _Scope));
	(*scope)->data = data;
	(*scope)->drop = drop;
	(*scope)->next = tmp;
	return data;
}

void _Scope_drop(struct _Scope * scope)
{
	while (scope)
	{
		scope->drop(scope->data);
		scope = scope->next;
	}
}

#define _func_A(type, name, a, b, c) \
	type name##_WRAPPED(EXPAND b struct _Scope **); \
	type name c \
	{ \
		struct _Scope* scope = NULL; \
		IF(NE(void, type))(type ret =) name##_WRAPPED(EXPAND a &scope); \
		_Scope_drop(scope); \
		return IF(NE(void, type))(ret) ; \
	} \
	type name##_WRAPPED(EXPAND b struct _Scope ** _scope) \

#define BOTH(a, b) a b
#define SECOND(a, b) b

#define ARGS_A(item, i) SECOND item,
#define ARGS_B(item, i) BOTH item,
#define ARGS_C(item, i) IF(NE(0, i))(,) BOTH item

#define func(type, name, ...) \
	_func_A( \
		type, \
		name, \
		(MAP(ARGS_A, __VA_ARGS__)), \
		(MAP(ARGS_B, __VA_ARGS__)), \
		(MAP(ARGS_C, __VA_ARGS__)))

#define make(type, name) \
	debug(printf(#type " created\n");) \
	struct _##type##_class * name##_OBJ = _make_obj(sizeof(struct _##type##_class), &_##type##_drop, _scope)

#define set(obj, prop, val) \
	obj##_OBJ->prop = val

#define get(obj, prop) \
	obj##_OBJ->prop

#define class(name, members) \
	struct _##name##_class \
	{ \
		struct _Scope * _scope; \
		EXPAND members \
	}; \

#define drop(name) \
	void _##name##_drop_wrapped(struct _##name##_class * data); \
	void _##name##_drop(void * data) \
	{ \
		debug(printf("dropping " #name "\n");) \
		_##name##_drop_wrapped(data); \
		_Scope_drop(((struct _##name##_class *)data)->_scope); \
		free(data); \
	} \
	void _##name##_drop_wrapped(struct _##name##_class * data) \

// ABOVE THIS IS BOILERPLATE ==========================================

class(TwoVals,
(
	int a;
	int b;
));

drop(TwoVals) {}

class(MyStruct,
(
	int x, y, z;
));

drop(MyStruct) {}

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