build/libCE.so: build/ce-command.o build/ce-instance.o build/ce-pipeline.o build/ce-error.o
	clang -shared -o build/libCE.so build/*.o  -lvulkan -O2

build/ce-command.o: ce-command.c
	clang -c -fPIC ce-command.c -o build/ce-command.o -O2

build/ce-instance.o: ce-instance.c
	clang -c -fPIC ce-instance.c -o build/ce-instance.o -O2

build/ce-pipeline.o: ce-pipeline.c
	clang -c -fPIC ce-pipeline.c -o build/ce-pipeline.o -O2

build/ce-error.o: ce-error.c
	clang -c -fPIC ce-error.c -o build/ce-error.o -O2

.PHONY: clean install

clean:
	rm build/*

install: build/libCE.so
	cp build/libCE.so /usr/lib/libCE.so
	cp ce-def.h /usr/include/CE/
	cp ce-command.h /usr/include/CE/
	cp ce-error.h /usr/include/CE/
	cp ce-pipeline.h /usr/include/CE/ 
	cp ce-instance.h /usr/include/CE/
	cp CE.h /usr/include/CE/
