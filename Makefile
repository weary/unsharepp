BUILDDIR=build

all: build

build: $(BUILDDIR)/Makefile
	$(MAKE) -C "$(BUILDDIR)"

clean:
	rm -rf "$(BUILDDIR)"

$(BUILDDIR)/Makefile:
	mkdir -p "$(BUILDDIR)"
	(cd "$(BUILDDIR)" && cmake -DCMAKE_BUILD_TYPE=Debug ..)

