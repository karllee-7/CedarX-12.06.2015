
CROSS_COMPILE=arm-linux-gnueabihf-


all:
	$(CROSS_COMPILE)g++ VideoTest.cpp \
			-I../../src/base/include \
			-I../../src/vdecoder/include \
			-L../../src/_install/lib/full-package-name \
			-lcedar_vdecoder -lcedar_common -lcedar_base\

clean:
	rm a.out

