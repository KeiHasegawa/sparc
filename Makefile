SRCS = $(wildcard *.cpp)
OBJS = $(SRCS:.cpp=.o)
SPARC_DLL = sparc.dll

XX_OBJS = $(SRCS:.cpp=.obj)
SPARC_XX_DLL = sparcxx.dll

DEBUG_FLAG = -g
CXX_PIC_FLAGS = -fPIC
CXXFLAGS = $(DEBUG_FLAG) $(CXX_PIC_FLAGS) -w -I$(HCC1_SRCDIR)
CXXFLAGS_FOR_XX = $(DEBUG_FLAG) $(PIC_FLAG) -w -I$(HCXX1_SRCDIR) -DCXX_GENERATOR

RM = rm -r -f

all:$(SPARC_DLL) $(SPARC_XX_DLL)


$(SPARC_DLL) : $(OBJS)
	$(CXX) $(DEBUG_FLAG) -shared -o $@ $(OBJS)

$(SPARC_XX_DLL) : $(XX_OBJS)
	$(CXX) $(DEBUG_FLAG) -shared -o $@ $(XX_OBJS)

%.obj : %.cpp
	$(CXX) $(CXXFLAGS_FOR_XX) -c $< -o $@

clean:
	$(RM) *.o *.obj *~ *.dll *.so .vs x64 Debug Release DebugCXX ReleaseCXX
