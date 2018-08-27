SRCS = $(wildcard *.cpp)
OBJS = $(SRCS:.cpp=.o)
SPARC_DLL = sparc.dll

DEBUG_FLAG = -g
CXX_PIC_FLAGS = -fPIC
CXXFLAGS = $(DEBUG_FLAG) $(CXX_PIC_FLAGS) -w -I$(HCC1_SRCDIR)
RM = rm -r -f

all:$(SPARC_DLL)


$(SPARC_DLL) : $(OBJS)
	$(CXX) $(CXX_DEBUG_FLAGS) -shared -o $@ $(OBJS)

clean:
	$(RM) *.o *~ *.dll *.so .vs x64 Debug Release DebugCXX ReleaseCXX
