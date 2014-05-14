include ../../global_config

OPENCV_DIR := $(shell expr ../../thirdparty/OpenCV* )

OBJS := \
	$(OBJDIR)/findobject.o \
	$(OBJDIR)/findobjectwindow.o \
	$(OBJDIR)/surfscan.o \

LFLAGS += $(OPENCV_DIR)/libopencv.a

CFLAGS += -I$(OPENCV_DIR)/modules/core/include/ \
	-I$(OPENCV_DIR)/modules/features2d/include/ \
	-I$(OPENCV_DIR)/modules/calib3d/include/ \
	-I$(OPENCV_DIR)/modules/flann/include/ \
	-I$(OPENCV_DIR)/modules/imgproc/include/ \
	-I$(OPENCV_DIR)/modules/legacy/include/ \
	-I$(OPENCV_DIR)/modules/highgui/include/ \
	-I$(OPENCV_DIR)/modules/objdetect/include/ \
	-I$(OPENCV_DIR)/modules/video/include/

OUTPUT = $(PLUGIN_DIR)/findobject.plugin

include ../../plugin_config

$(OBJDIR)/findobject.o: findobject.C findobject.h findobjectwindow.h
$(OBJDIR)/findobjectwindow.o: findobjectwindow.C findobject.h findobjectwindow.h
$(OBJDIR)/surfscan.o: surfscan.C




