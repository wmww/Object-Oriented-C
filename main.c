#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "define_EVIL/define_EVIL.h"

#define debug EXPAND_TRUE
#define CHECK EXPAND_TRUE
//#define debug EXPAND_FALSE;

#define _OBJ(name) _##name##_OBJ
#define _CLASS(name) _##name##_CLASS
#define _TETHER(name) _##name##_TETHER
#define _DROP_FUNC(name) _##name##_DROP
#define _WRAPPED(name) EXPAND_CAT(_, EXPAND_CAT(name, _WRAPPED))

typedef void _CLASS(void);

#define _TETHER_FOR_TYPE(name) \
	struct _TETHER(name) \
	{ \
		struct _CLASS(name) * data; \
		void (*drop)(void*); \
		struct _TETHER(void) * next; \
		struct _TETHER(void) * prev; \
	}; \

_TETHER_FOR_TYPE(void);
typedef struct _TETHER(void) _Tether;

void * _make_zeroed_mem(size_t size)
{
	void * data = malloc(size);
	memset(data, 0, size);
	return data;
}

void _DROP_FUNC(_Tether)(_Tether * tether)
{
	CHECK(if (tether->prev){
			fprintf(stderr, "WARNING: " TO_STRING(_DROP_FUNC(_Tether)) " called with non-starting tethe\n");
		})
	
	while (tether)
	{
		if (tether->drop)
		{
			tether->drop(tether->data);
		}
		_Tether * next = tether->next;
		memset(tether, 0, sizeof(_Tether));
		tether = next;
	}
}

#define _func_A(type, name, a, b, c) \
	type _WRAPPED(name)(EXPAND b _Tether * tether); \
	type name c \
	{ \
		_Tether tether; \
		memset(&tether, 0, sizeof(_Tether)); \
		IF(NE(void, type))(type ret =) _WRAPPED(name)(EXPAND a &tether); \
		_DROP_FUNC(_Tether)(&tether); \
		return IF(NE(void, type))(ret) ; \
	} \
	type _WRAPPED(name)(EXPAND b _Tether * _tether) \

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

#define declare(type, name) \
	struct _TETHER(type) _OBJ(name);

#define make(type) \
	(struct _TETHER(type)) \
	{ \
		.data = _make_zeroed_mem(sizeof(struct _CLASS(type))), \
		.drop = &_DROP_FUNC(type), \
		.next = NULL, \
		.prev = NULL, \
	} \

#define assign(var, obj) \
	_OBJ(var) = obj; \
	_OBJ(var).next = _tether->next; \
	_OBJ(var).prev = _tether; \
	_tether->next = (_Tether *)&_OBJ(var); \
	if (_OBJ(var).next) \
		_OBJ(var).next->prev = (_Tether *)&_OBJ(var); \

#define set(obj, prop, val) \
	CHECK(if (!_OBJ(obj).data) { fprintf(stderr, "WARNING: null pointer\n"); }) \
	_OBJ(obj).data->prop = val

#define get(obj, prop) \
	CHECK(_OBJ(obj).data ?) _OBJ(obj).data->prop CHECK(: fprintf(stderr, "WARNING: null pointer\n"))

#define class(name, members) \
	struct _CLASS(name) \
	{ \
		_Tether _tether; \
		EXPAND members \
	}; \
	_TETHER_FOR_TYPE(name);

#define drop(name) \
	void _WRAPPED(_DROP_FUNC(name))(struct _CLASS(name) * data); \
	void _DROP_FUNC(name)(void * data) \
	{ \
		debug(printf("dropping " #name "\n");) \
		_WRAPPED(_DROP_FUNC(name))(data); \
		_DROP_FUNC(_Tether)(&((struct _CLASS(name) *)data)->_tether); \
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
	declare(MyStruct, aaa);
	assign(aaa, make(MyStruct))
}

func(int, add_nums, (int, foo), (int, bar))
{
	declare(TwoVals, vals);
	assign(vals, make(TwoVals));
	set(vals, a, foo);
	do_nothing();
	set(vals, b, bar);
	declare(TwoVals, xyz);
	assign(xyz, make(TwoVals));
	return get(vals, a) + get(vals, b);
}

int main()
{
	printf("hello world\n");
	printf("%d\n", add_nums(4, 7));
	return 0;
}