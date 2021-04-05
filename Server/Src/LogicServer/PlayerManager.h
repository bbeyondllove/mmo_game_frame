﻿#ifndef __WS_PLAYER_MANAGER_OBJECT_H__
#define __WS_PLAYER_MANAGER_OBJECT_H__
#include "AVLTree.h"
#include "Position.h"
#include "PlayerObject.h"

class CPlayerManager : public AVLTree<UINT64, CPlayerObject>
{
	CPlayerManager();
	~CPlayerManager();
public:
	static CPlayerManager* GetInstancePtr();

public:
	CPlayerObject*      CreatePlayer(UINT64 u64RoleID);

	CPlayerObject*      GetPlayer(UINT64 u64RoleID);

	CPlayerObject*      CreatePlayerByID(UINT64 u64RoleID);

	INT32               GetOnlineCount();

	BOOL                ReleasePlayer(UINT64 u64RoleID);

	BOOL                BroadMessageToAll(UINT32 dwMsgID, const google::protobuf::Message& pdata);

	BOOL                ZeroTimer(UINT32 nParam);

	BOOL                OnUpdate(UINT64 uTick);

	BOOL                ReleaseAll();
public:

	UINT32              m_dwMaxCacheNum;
};

#endif //__WS_PLAYER_OBJECT_H__