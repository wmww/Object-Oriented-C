example: main.c define_EVIL/define_EVIL.h define_EVIL/generated.h
	gcc -std=c11 -o main_bin main.c

dump:
	gcc -std=c11 -E main.c
