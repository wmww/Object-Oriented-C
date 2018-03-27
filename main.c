#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "define_EVIL/define_EVIL.h"

#define SPEW_A_THING(...) __VA_ARGS__
#define SPEW_NOTHING(...)

#define debug SPEW_A_THING
//#define debug SPEW_NOTHING;

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

#define COMMA() (),

#define CHECK_VOID_void

#define func_INTERNAL(type, name, a, b, c) \
	type name##_WRAPPED(EXPAND b struct _Scope **); \
	type name c \
	{ \
		struct _Scope* scope = NULL; \
		EXPAND_CAT(SPEW_, CHECK_IF_THING(CHECK_VOID_##type)) (type ret =) name##_WRAPPED(EXPAND a &scope); \
		_Scope_drop(scope); \
		return EXPAND_CAT(SPEW_, CHECK_IF_THING(CHECK_VOID_##type)) (ret) ; \
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
	debug(printf(#type " created\n");) \
	struct _##type##_class * name##_OBJ = _make_obj(sizeof(struct _##type##_class), &_##type##_drop, _scope); \
	name##_OBJ->method_table = &_##type##_methods

#define set(obj, prop, val) \
	obj##_OBJ->prop = val

#define get(obj, prop) \
	obj##_OBJ->prop

#define class(name, superclass, members, methods) \
	struct _##name##_class \
	{ \
		struct superclass; \
		EXPAND members \
	}; \
	struct \
	{ \
		EXPAND methods \
	} _##name##_methods; \

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

#define callmethod(obj, type, method, ...) /* Can't figure out a way to detect class of object. */ \
	((_##type##_methods *) get(obj, method_table))->##method##(obj, __VA_ARGS__)

#define setmethod(type, name, func) \
	_##type##_methods.##name## = func

// TODO: method and defmethod macros doing argument wrapping like func and adding self pointer so we can have nicer syntax for declaring and defining methods.

struct _Object_class {
	struct _Scope * _scope;
	void * method_table;
};

// ABOVE THIS IS BOILERPLATE ==========================================

class(TwoVals, Object,
(
	int a;
	int b;
),
(
	int (*add)(struct _TwoVals_class * self_OBJ); // This syntax is... not the best.
));

func(int, add, (struct _TwoVals_class *, self_OBJ)) // This is also not the best.
{
	return get(self, a) + get(self, b);
}
setmethod(TwoVals, add, &add);

drop(TwoVals) {}

class(MyStruct, Object,
(
	int x, y, z;
),
());

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
	callmethod(vals, TwoVals, add);
	//return get(vals, a) + get(vals, b);
}

int main()
{
	printf("hello world\n");
	printf("%d\n", add_nums(4, 7));
	return 0;
}
