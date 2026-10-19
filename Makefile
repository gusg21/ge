src := main.c $\
	buffer.c cmds.c error_line.c hilite.c surface.c 
objects = $(patsubst %.c,%.o,$(src))

ge: $(objects)
	$(CC) $^ -o ge -lncurses -lm

# Syntax - targets ...: target-pattern: prereq-patterns ...
# In the case of the first target, foo.o, the target-pattern matches foo.o and sets the "stem" to be "foo".
# It then replaces the '%' in prereq-patterns with that stem
$(objects): %.o: %.c
	$(CC) -c $^ -o $@ -Werror -g -Ikat/include/

clean:
	rm -f ge
	rm -f *.o