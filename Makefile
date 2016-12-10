all: build_all verify_all

build_all:
	$(MAKE) -C encode $@
	$(MAKE) -C decode $@

verify_all:
	$(MAKE) -C encode $@
	$(MAKE) -C decode $@
