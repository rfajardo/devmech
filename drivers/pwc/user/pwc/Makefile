#USER INPUT
SRCS =  pwc.c buffers.c images.c pwc-dec23.c pwc-kiara.c
SRCS += frame_constructor_if.c acquire_data_if.c img_capture_if.c 
SRCS += dev_class.c
#SRCS += dev_func.c
TARGET = pwc
TARGETLIB = 
MODEL = static  	#dynamic|static
VERSION = 0.1
MODE = debug		#release|debug

INCLUDEDIRS = ../../../../ /usr/include/libusb-1.0
#libsystemc or systemc (system ignores lib at the beginning)
LIBNAMES = pwcmech devif usbif usb-1.0
LIBDIRS = ../pwcmech ../../../../devif/Build ../../../../usbif /usr/lib
DEPENDDIR = ./depend


#CONFIGURATION
DEBUGPARAM = 
RELEASEPARAM = 

DEBUGFLAGS = -g -O0
RELEASEFLAGS = -O2 -fomit-frame-pointer

CFLAGS = -Wall
CC = gcc



#MECHANICS
INCLUDESPATH = $(addprefix -I, $(INCLUDEDIRS))
LIBSPATH = $(addprefix -L, $(LIBDIRS))
LIBSLINKAGE = $(addprefix -l, $(subst lib, , $(LIBNAMES)) )
COMMA = ,
RPATH = $(addprefix -Wl$(COMMA)-R, $(LIBDIRS))

OBJS = $(addsuffix .o, $(basename $(SRCS)))
DEPS = $(addprefix $(DEPENDDIR)/, $(addsuffix .d, $(basename $(SRCS) ) ) )

STATICLIB = $(addprefix lib, $(addsuffix .a, $(TARGETLIB) ) )
DYNAMICLIB = $(addprefix lib, $(addsuffix .so, $(TARGETLIB) ) )
SONAME = $(addsuffix .$(VERSION), $(DYNAMICLIB))

ifeq (debug,$(findstring debug, $(MODE)))
CFLAGS += $(DEBUGFLAGS) $(addprefix -D, $(DEBUGPARAM))
else
CFLAGS += $(RELEASEFLAGS) $(addprefix -D, $(RELEASEPARAM))
endif

ifdef TARGETLIB
ifeq (dynamic,$(findstring dynamic, $(MODEL)))
TARGET = $(DYNAMICLIB)
CFLAGS += -fPIC
else
TARGET = $(STATICLIB)
endif
endif


#MAKEFILE RULES
all: $(TARGET)

depend: $(DEPS)

docs: Doxyfile
	doxygen

clean:
	rm -f *.o *~ $(TARGET) $(STATICLIB) $(DYNAMICLIB) $(SONAME)

distclean:
	make clean
	rm -rf $(DEPENDDIR) Doxygen


-include $(DEPS)


ifndef TARGETLIB
$(TARGET): $(OBJS)
	$(CC) $(LIBSPATH) $(RPATH) -o $@ $^ $(LIBSLINKAGE)
endif


$(STATICLIB): $(OBJS)
	ar rcs $@ $^

$(DYNAMICLIB): $(OBJS)
	$(CC) -shared -Wl,-soname,$(SONAME) -o $@ $^
	ln -fs $@ $(SONAME)


%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDESPATH) -c $< -o $@


$(DEPENDDIR)/%.d: %.c
	mkdir -p $(DEPENDDIR)
	$(CC) $(INCLUDESPATH) -MM -MF $@ $<


# DO NOT DELETE
