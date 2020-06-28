OBJS := main.o token.o

default: mysh

mysh: $(OBJS)
	$(CC) -o mysh $(OBJS)

clean:
	rm -rf mysh *.o *.test $(OBJS)

.PHONY: default clean
