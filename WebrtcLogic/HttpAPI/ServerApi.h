#ifndef SXBSERVERAPI_H
#define SXBSERVERAPI_H
#include "ServerHelper.h"

#define SXBPARAM void* data, SxbRecFun receiver

class ServerApi
{
public:
	ServerApi();
	virtual ~ServerApi();
};
#endif //SXBSERVERAPI_H