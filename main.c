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

#define _OBJ(name) _##name##_OBJ
#define _CLASS(name) _##name##_CLASS
#define _DROP_FUNC(name) _##name##_DROP
#define _WRAPPED(name) EXPAND_CAT(_, EXPAND_CAT(name, _WRAPPED))

#define _func_A(type, name, a, b, c) \
	type _WRAPPED(name)(EXPAND b struct _Scope **); \
	type name c \
	{ \
		struct _Scope* scope = NULL; \
		IF(NE(void, type))(type ret =) _WRAPPED(name)(EXPAND a &scope); \
		_Scope_drop(scope); \
		return IF(NE(void, type))(ret) ; \
	} \
	type _WRAPPED(name)(EXPAND b struct _Scope ** _scope) \

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
	struct _CLASS(type) * _OBJ(name) = _make_obj(sizeof(struct _CLASS(type)), &_DROP_FUNC(type), _scope)

#define set(obj, prop, val) \
	_##obj##_OBJ->prop = val

#define get(obj, prop) \
	_##obj##_OBJ->prop

#define class(name, members) \
	struct _CLASS(name) \
	{ \
		struct _Scope * _scope; \
		EXPAND members \
	}; \

#define drop(name) \
	void _WRAPPED(_DROP_FUNC(name))(struct _CLASS(name) * data); \
	void _DROP_FUNC(name)(void * data) \
	{ \
		debug(printf("dropping " #name "\n");) \
		_WRAPPED(_DROP_FUNC(name))(data); \
		_Scope_drop(((struct _CLASS(name) *)data)->_scope); \
		free(data); \
	} \
	void _WRAPPED(_DROP_FUNC(name))(struct _CLASS(name) * data)

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