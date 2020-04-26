TARGET = easyFLAC_example.exe
DEPEND_DLL = easyFLAC.dll
LDFLAGS = -Wl,--enable-stdcall-fixup,--kill-at


all: $(DEPEND_DLL)
	$(CXX) easyFLAC_example.c -c -I easyFLAC
	$(CXX) easyFLAC_example.o $(DEPEND_DLL) $(LDFLAGS) -o $(TARGET)
	
.PHONY : $(DEPEND_DLL)
$(DEPEND_DLL):
	make -C easyFLAC OSAL=$(OSAL)
	cp easyFLAC/$(DEPEND_DLL) .

clean: 
	rm -f *.o
	rm -f $(DEPEND_DLL)
	make -C easyFLAC clean