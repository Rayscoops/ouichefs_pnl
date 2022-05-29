CC= gcc

all: restor_version relese_version change_version

restor: restor_version
relese: relese_version
change: change_version

restor_version: restor_version.c
	$(CC) -o $@  $<

relese_version: relese_version.c
	$(CC) -o $@  $<

change_version: change_version.c
	$(CC) -o $@  $<

clean:
	-rm -f restor_version relese_version change_version

.PHONY: all clean
