build/test: test.c ce-command.c ce-instance.c 
	clang -o build/test *.c -lvulkan -O2 