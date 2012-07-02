DIRS    := src     

.PHONY:install

all: install $(DIRS)
	for dir in $(DIRS); do \
		make -C $$dir; \
	done

install:
	mkdir -p lib

clean:
	for dir in $(DIRS); do \
		make -C $$dir clean; \
	done

