#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "define_EVIL/define_EVIL.h"

#define debug EXPAND_TRUE
//#define debug EXPAND_FALSE;

#define CHECK EXPAND_TRUE

#define _OBJ(name) EXPAND_CAT(EXPAND_CAT(_, name), _OBJECT)
#define _CLASS(name) _##name##_CLASS
#define _TETHER(name) EXPAND_CAT(EXPAND_CAT(_, name), _TETHER)
#define _DROP_FUNC(name) _##name##_DROP
#define _MAKE_FUNC(name) _##name##_MAKE
#define _WRAPPED(name) EXPAND_CAT(_, EXPAND_CAT(name, _WRAPPED))

#define ENABLE_EQ_nil_nil

typedef void _CLASS(void);

#define panic(message) _panic(__FILE__, __LINE__, message)

int _panic(const char * file, int line, const char * message)
{
	fprintf(stderr, "'%s' panicked on line %d: %s\n", file, line, message);
	exit(1);
	return 0;
}

void * _alloc_zeroed_mem(size_t size)
{
	void * data = malloc(size);
	memset(data, 0, size);
	return data;
}

struct _Scope
{
	void * data;
	void (*drop)(void*);
	struct _Scope * next;
	struct _Scope * prev;
};

// can be sent a null scope, scope it is sent an subsequent scopes must be freeable
void _Scope_list_drop(struct _Scope * start)
{
	if (start && start->prev)
		start->prev->next = NULL;

	while (start)
	{
		if (start->drop)
			start->drop(start->data);

		struct _Scope * next = start->next;
		free(start);
		start = next;
	}
}

void _Scope_extract(struct _Scope * scope)
{
	if (!scope) return;
	if (scope->prev) scope->prev->next = scope->next;
	if (scope->next) scope->next->prev = scope->prev;
	scope->prev = NULL;
	scope->next = NULL;
}

void _Scope_insert(struct _Scope * target, struct _Scope * scope)
{
	if (!scope) return;
	scope->prev = target;
	scope->next = target->next;
	if (target->next) target->next->prev = scope;
	target->next = scope;
}

void _Scope_drop(struct _Scope * scope)
{
	if (!scope) return;
	_Scope_extract(scope);
	if (scope->drop)
		scope->drop(scope->data);
	free(scope);
}

struct _GenericTether
{
	void * data;
	struct _Scope * scope;
} _tmp_generic_tether;

// must return int for stupid reasons
int _move_tether(struct _GenericTether * dest, struct _GenericTether * source)
{
	*dest = *source;
	memset(source, 0, sizeof(struct _GenericTether));
	return 0;
}

#define _GET_TYPE_A(item) item EXPAND_FALSE (
#define _GET_TYPE(item) _GET_TYPE_A item )

#define _GET_OBJ_PERAM(item, with_type, with_tmps)\
	EXPAND_##with_type(struct _TETHER(_GET_TYPE(item))) \
	_OBJ(EXPAND_FALSE item) \
	IF(with_tmps) \
		(, IF_ELSE(with_type) \
			(struct _TETHER(_GET_TYPE(item)) _OBJ(EXPAND_CAT(tmp_, EXPAND_FALSE item))) \
			((struct _TETHER(_GET_TYPE(item))) {}))
#define _GET_PERAM(item, with_type, with_tmps) IF_ELSE(HAS_PEREN(item)) \
										(_GET_OBJ_PERAM(EXPAND item, with_type, with_tmps)) \
										(EXPAND_##with_type item)
#define _GET_PARAMS_FUNCTOR_NO_END_COMMA(item, i) IF(NE(0, i))(,) _GET_PERAM(item, TRUE, FALSE)
#define _GET_PARAMS_FUNCTOR(item, i) _GET_PERAM(item, TRUE, TRUE),
#define _GET_PARAM_NAMES_FUNCTOR(item, i) _GET_PERAM(item, FALSE, TRUE),
#define _GET_RETURN_TYPE(type) IF_ELSE(HAS_PEREN(type))(struct _TETHER(EXPAND type))(type)

#define func(type, name, ...) \
	_GET_RETURN_TYPE(type) _WRAPPED(name)(MAP(_GET_PARAMS_FUNCTOR, __VA_ARGS__) struct _Scope * _scope); \
	_GET_RETURN_TYPE(type) name (MAP(_GET_PARAMS_FUNCTOR_NO_END_COMMA, __VA_ARGS__)) \
	{ \
		struct _Scope scope; \
		memset(&scope, 0, sizeof(struct _Scope)); \
		IF(NE(void, _GET_RETURN_TYPE(type)))(_GET_RETURN_TYPE(type) ret =) _WRAPPED(name)(MAP(_GET_PARAM_NAMES_FUNCTOR, __VA_ARGS__) &scope); \
		_Scope_list_drop(scope.next); \
		return IF(NE(void, _GET_RETURN_TYPE(type)))(ret) ; \
	} \
	_GET_RETURN_TYPE(type) _WRAPPED(name)(MAP(_GET_PARAMS_FUNCTOR, __VA_ARGS__) struct _Scope * _scope)

#define var(type, obj, val) \
	struct _TETHER(type) _OBJ(tmp_##obj); \
	(void) _OBJ(tmp_##obj); \
	struct _TETHER(type) _OBJ(obj) = IF_ELSE(EQ(nil, val)) \
										((struct _TETHER(type)){.data = NULL, .scope = NULL}) \
										(val); \
	_Scope_insert(_scope, _OBJ(obj).scope)

#define make(type) \
	_MAKE_FUNC(type)()

#define move(obj) \
	_move_tether((struct _GenericTether *)&_OBJ(tmp_##obj), (struct _GenericTether *)&_OBJ(obj)) \
		? _OBJ(tmp_##obj) : _OBJ(tmp_##obj)
		// ternary is so the expansion can start with a concatable id, so nil checks will work

struct _Scope * _set_tmp_target;
struct _Scope ** _set_tmp_scope;

#define set(obj, val) \
	_Scope_drop(_OBJ(obj).scope); \
	IF_ELSE(EQ(nil, val)) ( \
		_OBJ(obj).data = NULL; \
		_OBJ(obj).scope = NULL; \
	)	(_OBJ(obj) = val;) \
	_Scope_insert(_scope, _OBJ(obj).scope)

#define prop(obj, prop) \
	(CHECK(_OBJ(obj).data ?) _OBJ(obj).data CHECK(: _OBJ(obj).data + panic("accessed '" #prop "' from nil object '" #obj "'")))->prop

#define nil(type) (struct _TETHER(type)){.data = NULL, .scope = NULL}

#define class(name, members) \
	struct _CLASS(name) \
	{ \
		struct _Scope _scope; \
		EXPAND members \
	}; \
	struct _TETHER(name) \
	{ \
		struct _CLASS(name) * data; \
		struct _Scope * scope; \
	}; \
	void _DROP_FUNC(name)(void * data);

#define func_make(name) \
	void _WRAPPED(_MAKE_FUNC(name))(struct _CLASS(name) * data); \
	struct _TETHER(name) _MAKE_FUNC(name)() \
	{ \
		debug(printf("making " #name "\n");) \
		struct _CLASS(name) * data = _alloc_zeroed_mem(sizeof(struct _CLASS(name))); \
		struct _Scope * scope = _alloc_zeroed_mem(sizeof(struct _Scope)); \
		scope->data = data; \
		scope->drop = _DROP_FUNC(name); \
		return (struct _TETHER(name)) \
		{ \
			.data = data, \
			.scope = scope, \
		}; \
	} \
	void _WRAPPED(_MAKE_FUNC(name))(struct _CLASS(name) * data)

#define func_drop(name) \
	void _WRAPPED(_DROP_FUNC(name))(struct _CLASS(name) * data); \
	void _DROP_FUNC(name)(void * data) \
	{ \
		debug(printf("dropping " #name "\n");) \
		_WRAPPED(_DROP_FUNC(name))(data); \
		_Scope_list_drop(((struct _CLASS(name) *)data)->_scope.next);\
		free(data); \
	} \
	void _WRAPPED(_DROP_FUNC(name))(struct _CLASS(name) * data)

// ABOVE THIS IS BOILERPLATE ==========================================

class(TwoVals,
(
	int a;
	int b;
));

func_make(TwoVals) {}
func_drop(TwoVals) {}

class(MyStruct,
(
	int x, y, z;
));

func_make(MyStruct) {}
func_drop(MyStruct) {}

func((MyStruct), do_nothing, ((TwoVals) vals))
{
	var(TwoVals, xyz, move(vals));
	var(MyStruct, aaa, make(MyStruct));
	return move(aaa);
}

func(int, add_nums, (int) foo, (int) bar)
{
	var(TwoVals, vals, make(TwoVals));
	set(vals, nil);
	set(vals, make(TwoVals));
	prop(vals, a) = foo;
	do_nothing(move(vals));
	prop(vals, b) = bar;
	var(TwoVals, xyz, nil);
	return prop(vals, a) + prop(vals, b);
}

int main()
{
	printf("hello world\n");
	printf("%d\n", add_nums(4, 7));
	return 0;
}