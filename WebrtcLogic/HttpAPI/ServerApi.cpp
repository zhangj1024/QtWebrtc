#include "stdafx.h"
#include "ServerApi.h"

#define SXBPARAM_IN receiver, data
#define SXBPARAM_IN_REV data, receiver

// void ServerApi::CreateToken(SXBPARAM, QString &szPhone, QString &szPassword, QString &szSmCode)
// {
// 	QVariantMap varmap;
// 	varmap.insert("userNo", szPhone);
// 	varmap.insert("pwd", szPassword);
// 	varmap.insert("smsCode", szSmCode);
// 
// 	ServerHelper::request(varmap, SVC_ACCOUNT, CMD_ACC_Register, SXBPARAM_IN);
// }
