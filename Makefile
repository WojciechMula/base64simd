all: build_intel verify_intel

build_intel:
	$(MAKE) -C encode $@
	$(MAKE) -C decode $@

verify_intel:
	$(MAKE) -C encode $@
	$(MAKE) -C decode $@
