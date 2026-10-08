#---------------------------------------------------------------------------------
# Makefile for Dear ImGui PS3 Standalone Application
# Supports: PSL1GHT v2 / OpenPS3 (CFW + HFW / PS3HEN compatible)
#---------------------------------------------------------------------------------

ifeq ($(strip $(PS3DEV)),)
  export PS3DEV := /opt/ps3dev
endif

ifeq ($(strip $(PSL1GHT)),)
  export PSL1GHT := $(PS3DEV)
endif

export PATH := $(PS3DEV)/bin:$(PS3DEV)/ppu/bin:$(PATH)

TARGET    := imgui_ps3_demo
TITLE     := Dear ImGui PS3
APPID     := IMGU00001
CONTENTID := UP0001-$(APPID)_00-0000000000000000

IMGUI_DIR   := ../..
BACKEND_DIR := ../../backends

CXX       := ppu-g++
CC        := ppu-gcc

SRCS_CXX  := main.cpp \
             $(BACKEND_DIR)/imgui_impl_ps3rsx.cpp \
             $(BACKEND_DIR)/imgui_impl_ps3pad.cpp \
             $(IMGUI_DIR)/imgui.cpp \
             $(IMGUI_DIR)/imgui_draw.cpp \
             $(IMGUI_DIR)/imgui_tables.cpp \
             $(IMGUI_DIR)/imgui_widgets.cpp \
             $(IMGUI_DIR)/imgui_demo.cpp

OBJS      := $(SRCS_CXX:.cpp=.o)

CXXFLAGS  := -O2 -Wall -mcpu=cell -D__PS3__ -D__PSL1GHT__ \
             -I$(IMGUI_DIR) -I$(BACKEND_DIR) -I$(PSL1GHT)/ppu/include

CFLAGS    := -O2 -Wall -mcpu=cell -D__PS3__ -D__PSL1GHT__ \
             -I$(IMGUI_DIR) -I$(BACKEND_DIR) -I$(PSL1GHT)/ppu/include

LIBS      := -L$(PSL1GHT)/ppu/lib -lrsx -lgcm_sys -lio -lsysutil -lm

.PHONY: all clean

all: $(TARGET).elf $(TARGET).self $(TARGET).pkg

$(TARGET).elf: $(OBJS)
	@echo "Linking ELF: $@"
	$(CXX) $(OBJS) $(LIBS) -o $@

%.o: %.cpp
	@echo "Compiling: $<"
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TARGET).self: $(TARGET).elf
	@echo "Creating SELF: $@"
	make_fself $< $@
	cp $@ EBOOT.BIN

$(TARGET).pkg: $(TARGET).self
	@echo "Creating PKG: $@"
	mkdir -p build/pkg/USRDIR
	cp EBOOT.BIN build/pkg/USRDIR/
	cp $(PS3DEV)/bin/ICON0.PNG build/pkg/ICON0.PNG 2>/dev/null || true
	sfo --title "$(TITLE)" --appid "$(APPID)" -f $(PS3DEV)/bin/sfo.xml build/pkg/PARAM.SFO 2>/dev/null || true
	pkg --contentid $(CONTENTID) build/pkg/ $@ 2>/dev/null || true

clean:
	rm -f $(OBJS) $(TARGET).elf $(TARGET).self $(TARGET).pkg EBOOT.BIN
	rm -rf build
