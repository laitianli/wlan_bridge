subdirs += src_lib wlan_br_server wlan_br_client

build:
	for dir in $(subdirs) ;\
	do \
		[ ! -d $$dir ] || make -C $$dir || exit $$?; \
	done

clean:
	for dir in $(subdirs) ;\
	do \
		[ ! -d $$dir ] || make -C $$dir clean || exit $$?; \
	done


all: build

.PHONY: all build clean
