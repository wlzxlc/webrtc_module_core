#ifndef _TEST_ISOURCE_H_
#define _TEST_ISOURCE_H_
#include "IInterface.h"

class ISource {
    public:
        enum {
            kSourceFile = 1,
            kSourceCamera
        };
		enum {
			kCallId_Width = 3001,
			kCallId_Height,
			kCallId_Color,
			kCallId_Start,
			kCallId_Stop,
			kCallId_Register,
			kCallId_Deregiser
		};

        virtual int width() = 0;
		virtual int height() = 0;
		virtual int color() = 0;
		virtual int start() = 0;
		virtual int stop () = 0;
		virtual int registerDeliver(IDeliver * stream) = 0;
		virtual int deregisterDeliver(IDeliver *stream) = 0;
		virtual int sourceType() = 0;
        virtual ~ISource(){}
        static ISource * Create(int type, int w, int h, int color);
};

#endif
