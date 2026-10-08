#---------------------------------------------------------------------------------
# Makefile for Dear ImGui PS3 Standalone Application
# Supports: PSL1GHT v2 / OpenPS3 (CFW + HFW / PS3HEN compatible)
#---------------------------------------------------------------------------------

.SUFFIXES:
ifeq ($(strip $(PSL1GHT)),)
$(error "Please set PSL1GHT in your environment. export PSL1GHT=<path>")
endif

include $(PSL1GHT)/host/rules/ps3.rules

TARGET          := imgui_ps3_demo
CONTENT_ID      := UP0001-IMGU00001_00-0000000000000000
TITLE           := Dear ImGui PS3

IMGUI_DIR       := ../..
BACKEND_DIR     := ../../backends

SRCS_C          := 
SRCS_CXX        := main.cpp \
                   $(BACKEND_DIR)/imgui_impl_ps3rsx.cpp \
                   $(BACKEND_DIR)/imgui_impl_ps3pad.cpp \
                   $(IMGUI_DIR)/imgui.cpp \
                   $(IMGUI_DIR)/imgui_draw.cpp \
                   $(IMGUI_DIR)/imgui_tables.cpp \
                   $(IMGUI_DIR)/imgui_widgets.cpp \
                   $(IMGUI_DIR)/imgui_demo.cpp

OBJS            := $(SRCS_C:.c=.o) $(SRCS_CXX:.cpp=.o)

CXXFLAGS        := -O2 -Wall -mcpu=cell -D__PS3__ -D__PSL1GHT__ \
                   -I$(IMGUI_DIR) -I$(BACKEND_DIR) -I$(PSL1GHT)/include

CFLAGS          := -O2 -Wall -mcpu=cell -D__PS3__ -D__PSL1GHT__ \
                   -I$(IMGUI_DIR) -I$(BACKEND_DIR) -I$(PSL1GHT)/include

LDFLAGS         := -L$(PSL1GHT)/lib -lrsx -lgcm_sys -lpad -lio -lsysutil -lm

.PHONY: all clean pkg

all: $(TARGET).elf $(TARGET).self

$(TARGET).elf: $(OBJS)
	@echo "Linking ELF: $@"
	$(CXX) $(OBJS) $(LDFLAGS) -o $@

$(TARGET).self: $(TARGET).elf
	@echo "Creating FSELF for CFW & HEN: $@"
	make_fself $< EBOOT.BIN
	cp EBOOT.BIN $@

pkg: $(TARGET).self
	@echo "Building PKG file..."
	mkdir -p pkg/USRDIR
	cp EBOOT.BIN pkg/USRDIR/
	echo "Content-ID = $(CONTENT_ID)" > package.conf
	echo "k_licensee = 0x00000000000000000000000000000000" >> package.conf
	echo "DRM_Type = Free" >> package.conf
	echo "Content_Type = Game_Exec" >> package.conf
	echo "Package_Version = 01.00" >> package.conf
	pkg.py --contentid $(CONTENT_ID) pkg/ $(TARGET).pkg

clean:
	rm -f $(OBJS) $(TARGET).elf $(TARGET).self EBOOT.BIN $(TARGET).pkg package.conf
	rm -rf pkg
