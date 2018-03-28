#include <stdio.h>
#include <stdlib.h>
#include "define_EVIL/define_EVIL.h"

struct _Object_class {
	void * method_table;
};

void * _create_object(size_t size, void *methods) {
  struct _Object_class * obj = malloc(size);
  obj->method_table = methods;
  return (void *) obj;
}

void _destroy_object(void *obj) {
  free(obj);
}

#define class(name, superclass, members, methods) \
	typedef struct _##name##_class \
	{ \
		struct _##superclass##_class; /* This is unfortunately an extension that needs enabling with -fme-extensions, but it should work on gcc, clang, and MSVC (not tested on MSVC). */ \
		EXPAND members \
	} _##name##_class; \
	struct _##name##_method_table \
	{ \
		EXPAND methods \
	}; \
	struct _##name##_method_table _##name##_methods;

#define new(type, name) \
  struct _##type##_class * name = _create_object(sizeof(struct _##type##_class), &_##type##_methods)

#define delete(obj) \
  _destroy_object(obj); \
  obj = NULL

#define callmethod(obj, type, method, ...) /* Can't figure out a way to detect class of object. */ \
	((struct _##type##_method_table *) obj->method_table)->method(IF_ELSE(IS_THING(__VA_ARGS__))(obj, __VA_ARGS__)(obj))

#define setmethod(type, name, func) /* Has to be inside a function somewhere. Require it to be at top of main() somehow? */ \
	_##type##_methods.name = func

// Example code below

class(Example, Object, (
  int a;
  int b;
), (
  int (*add)(struct _Example_class * self);
))

int add(struct _Example_class * self)
{
  return self->a + self->b;
}

int main(int argc, char const *argv[]) {
  setmethod(Example, add, &add);
  new(Example, exmp);
  exmp->a = 9;
  exmp->b = 4;
  printf("%i\n", callmethod(exmp, Example, add));
  delete(exmp);
  return 0;
}
