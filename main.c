#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "define_EVIL/define_EVIL.h"

// enables debug messages and checks for the framework itself
// #define ENABLE_DEBUG

// for performence, makes using nil objects undefined behaviour
// #define DISABLE_NIL_CHECKS

#define _OBJ(name) EXPAND_CAT(EXPAND_CAT(_, name), _OBJECT)
#define _CLASS(name) _##name##_CLASS
#define _TETHER(name) EXPAND_CAT(EXPAND_CAT(_, name), _TETHER)
#define _DROP_FUNC(name) _##name##_DROP
#define _MAKE_FUNC(name) _##name##_MAKE
#define _WRAPPED(name) EXPAND_CAT(_, EXPAND_CAT(name, _WRAPPED))

#define ENABLE_EQ_nil_nil

#ifdef DISABLE_NIL_CHECKS
#define _CHECK_NIL FALSE
#else
#define _CHECK_NIL TRUE
#endif

#ifdef ENABLE_DEBUG
#define _DEBUG TRUE
#else
#define _DEBUG FALSE
#endif

typedef void _CLASS(void);

#ifdef ENABLE_DEBUG
const char * _current_func = NULL;
int _stack_frame_count = 0;
#endif

#define panic(...) _panic(__FILE__, __func__, __LINE__, __VA_ARGS__)

int _panic(const char * file, const char * func_name, int line, const char * format, ...)
{
	fprintf(stderr, "%s panicked on line %d: ", file, line);
	va_list argptr;
    va_start(argptr, format);
    vfprintf(stderr, format, argptr);
    va_end(argptr);
	fprintf(stderr, "\n");
	exit(1);
	return 0;
}

#define debug_msg(...) _debug_msg(__FILE__, __func__, __LINE__, __VA_ARGS__)

int _debug_msg(const char * file, const char * func_name, int line, const char * format, ...)
{
	const char * current_func = func_name;
	#ifdef ENABLE_DEBUG
	if (_current_func)
		current_func = _current_func;
	#endif
	fprintf(stdout, "debug [%s:%d] ", file, line);
	#ifdef ENABLE_DEBUG
	for (int i = 1; i < _stack_frame_count; i++)
		fprintf(stdout, "  ");
	#endif
	fprintf(stdout, "%s(): ", current_func);
	va_list argptr;
    va_start(argptr, format);
    vfprintf(stdout, format, argptr);
    va_end(argptr);
	fprintf(stdout, "\n");
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
	_Scope_extract(source->scope);
	*dest = *source;
	memset(source, 0, sizeof(struct _GenericTether));
	return 0;
}

// must return int for stupid reasons
int _share_tether(struct _GenericTether * dest, struct _GenericTether * source)
{
	*dest = *source;
	dest->scope = _alloc_zeroed_mem(sizeof(struct _Scope));
	dest->scope->data = source->scope->data;
	dest->scope->drop = source->scope->drop;
	*((int*)dest->data) += 1; // add to the ref count
	IF(_DEBUG)(debug_msg("incremented object[%p] ref count to %d", dest->data, *((int*)dest->data) + 1);)
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
#define _PUT_OBJ_PARAM_IN_SCOPE_FUNCTOR(item, i) IF(HAS_PEREN(item))(_Scope_insert(&scope, _OBJ(EXPAND_FALSE EXPAND item).scope);)

#define _FUNCTION_HEADER(type, name, is_wrapper, ...) \
	_GET_RETURN_TYPE(type) \
	IF_ELSE(is_wrapper) \
		(_WRAPPED(name)) \
		(name) \
	( \
		MAP( \
			IF_ELSE(is_wrapper)\
				(_GET_PARAMS_FUNCTOR) \
				(_GET_PARAMS_FUNCTOR_NO_END_COMMA), \
			__VA_ARGS__) \
		IF(is_wrapper)(struct _Scope * _scope) \
	) \

#define func(type, name, ...) \
	_FUNCTION_HEADER(type, name, TRUE, __VA_ARGS__); \
	_FUNCTION_HEADER(type, name, FALSE, __VA_ARGS__) IF(HAS_PEREN(type))(__attribute__((warn_unused_result))); \
	_FUNCTION_HEADER(type, name, FALSE, __VA_ARGS__) \
	{ \
		struct _Scope scope; \
		memset(&scope, 0, sizeof(struct _Scope)); \
		MAP(_PUT_OBJ_PARAM_IN_SCOPE_FUNCTOR, __VA_ARGS__) \
		IF(_DEBUG)( \
			const char * calling_func = _current_func; \
			if (_current_func) \
				debug_msg("calling %s()", #name); \
			_current_func = #name; \
			_stack_frame_count++; \
		) \
		IF(NE(void, _GET_RETURN_TYPE(type)))(_GET_RETURN_TYPE(type) ret =) _WRAPPED(name)(MAP(_GET_PARAM_NAMES_FUNCTOR, __VA_ARGS__) &scope); \
		IF(_DEBUG)(debug_msg("dropping scope");) \
		_Scope_list_drop(scope.next); \
		IF(_DEBUG)( \
			debug_msg("retuning"); \
			_stack_frame_count--; \
			_current_func = calling_func; \
		) \
		return IF(NE(void, _GET_RETURN_TYPE(type)))(ret) ; \
	} \
	_FUNCTION_HEADER(type, name, TRUE, __VA_ARGS__) \

#define var(type, obj, val) \
	struct _TETHER(type) _OBJ(tmp_##obj); \
	(void) _OBJ(tmp_##obj); \
	struct _TETHER(type) _OBJ(obj) = IF_ELSE(EQ(nil, val)) \
										((struct _TETHER(type)){.data = NULL, .scope = NULL}) \
										(val); \
	IF(_DEBUG)(debug_msg("gave '%s' initial value %s[%p]", #obj, #type, _OBJ(obj).data);) \
	_Scope_insert(_scope, _OBJ(obj).scope)

#define make(type) \
	_MAKE_FUNC(type)()

#define _MOVE_SHARE_GET_TETHER(obj) \
	((struct _GenericTether *)&_OBJ(tmp_##obj), (struct _GenericTether *)&_OBJ(obj)) \
		? _OBJ(tmp_##obj) : _OBJ(tmp_##obj)
		// ternary is so the expansion can start with a concatable id, so nil checks will work

#define move(obj) \
	IF(_DEBUG)(debug_msg("moving object[%p] out of '%s'", _OBJ(obj).data, #obj) +) \
	_move_tether _MOVE_SHARE_GET_TETHER(obj)

#define share(obj) \
	IF(_DEBUG)(debug_msg("sharing object[%p] from '%s'", _OBJ(obj).data, #obj) +) \
	_share_tether _MOVE_SHARE_GET_TETHER(obj)

struct _Scope * _set_tmp_target;
struct _Scope ** _set_tmp_scope;

#define set(obj, val) \
	_OBJ(tmp_##obj) = val; \
	_Scope_drop(_OBJ(obj).scope); \
	IF_ELSE(EQ(nil, val)) ( \
		_OBJ(obj).data = NULL; \
		_OBJ(obj).scope = NULL; \
	)	(_OBJ(obj) = _OBJ(tmp_##obj);) \
	IF(_DEBUG)(debug_msg("'%s' set to object[%p]", #obj, _OBJ(obj).data);) \
	_Scope_insert(_scope, _OBJ(obj).scope)

#define prop(obj, prop) \
	( \
		IF_ELSE(_CHECK_NIL) \
			(_OBJ(obj).data ? _OBJ(obj).data : _OBJ(obj).data + panic("accessed '%s' from nil object '%s'", #prop, #obj)) \
			(_OBJ(obj).data) \
	) \
	->prop

#define nil(type) (struct _TETHER(type)){.data = NULL, .scope = NULL}

#define class(name, members) \
	struct _CLASS(name) \
	{ \
		/* currently ref count is assumed to be the first arg */ \
		/* always 1 less then it should be (zero when there is one owner */ \
		int _ref_count; \
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
		struct _CLASS(name) * data = _alloc_zeroed_mem(sizeof(struct _CLASS(name))); \
		IF(_DEBUG)(debug_msg("making %s[%p]", #name, data);) \
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
		IF(_DEBUG)(if (!data) panic("dropped nil %s", #name);) \
		if (((struct _CLASS(name) *)data)->_ref_count > 0) \
		{ \
			((struct _CLASS(name) *)data)->_ref_count--; \
			IF(_DEBUG)(debug_msg("decremented %s[%p] ref count to %d", #name, data, ((struct _CLASS(name) *)data)->_ref_count + 1);) \
		} \
		else \
		{ \
			IF(_DEBUG)(debug_msg("dropping %s[%p]", #name, data);) \
			_WRAPPED(_DROP_FUNC(name))(data); \
			_Scope_list_drop(((struct _CLASS(name) *)data)->_scope.next);\
			IF(_DEBUG)(memset(data, 0, sizeof(struct _CLASS(name)));) \
			free(data); \
		} \
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
	var(MyStruct, bbb, make(MyStruct));
	printf("about to return\n");
	return move(bbb);
}

func(int, add_nums, (int) foo, (int) bar)
{
	var(MyStruct, result, do_nothing(nil(TwoVals)));
	set(result, move(result));
	return 7;
}

int main()
{
	printf("hello world\n");
	printf("%d\n", add_nums(4, 7));
	return 0;
}