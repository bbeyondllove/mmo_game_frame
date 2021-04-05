﻿#include "stdafx.h"
#include "SealManager.h"
#include "GameService.h"
#include "StaticData.h"
#include "../Message/Msg_ID.pb.h"
#include "../Message/Msg_RetCode.pb.h"
#include "PacketHeader.h"
#include "IBufferHandler.h"
#include "SealData.h"
#include "DataPool.h"

CSealManager::CSealManager()
{
}

CSealManager::~CSealManager()
{
	for (auto itor = m_mapSealData.begin(); itor != m_mapSealData.end(); ++itor)
	{
		SealDataObject* pObject = itor->second;
		pObject->Release();
	}

	m_mapSealData.clear();
}

CSealManager* CSealManager::GetInstancePtr()
{
	static CSealManager _StaticMgr;

	return &_StaticMgr;
}

BOOL CSealManager::LoadData(CppMySQL3DB& tDBConnection)
{
	CppMySQLQuery QueryResult = tDBConnection.querySQL("SELECT * FROM seal_role");
	while (!QueryResult.eof())
	{
		SealDataObject* pSealDataObject = DataPool::CreateObject<SealDataObject>(ESD_SEAL_ROLE, FALSE);
		pSealDataObject->m_uRoleID = QueryResult.getInt64Field("roleid");
		pSealDataObject->m_uSealTime = QueryResult.getIntField("sealendtime");
		pSealDataObject->m_nSealAction = QueryResult.getIntField("sealaction");
		m_mapSealData.insert(std::make_pair(pSealDataObject->m_uRoleID, pSealDataObject));
		QueryResult.nextRow();
	}

	return TRUE;
}

BOOL CSealManager::SealRole(UINT64 uRoleID, UINT64 uSealTime, UINT32 nSealAction)
{
	SealDataObject* pSealData = GetSealData(uRoleID);
	if (pSealData == NULL)
	{
		pSealData = DataPool::CreateObject<SealDataObject>(ESD_SEAL_ROLE, TRUE);
		pSealData->Lock();
		pSealData->m_uRoleID = uRoleID;
		pSealData->m_uSealTime = CommonFunc::GetCurrTime() + uSealTime;
		pSealData->m_nSealAction = nSealAction;
		pSealData->Unlock();
		m_mapSealData.insert(std::make_pair(pSealData->m_uRoleID, pSealData));
	}
	else
	{
		pSealData->Lock();
		pSealData->m_uRoleID = uRoleID;
		pSealData->m_uSealTime = CommonFunc::GetCurrTime() + uSealTime;
		pSealData->m_nSealAction = nSealAction;
		pSealData->Unlock();
	}

	return TRUE;
}

BOOL CSealManager::UnSealRole(UINT64 uRoleID)
{
	auto itor = m_mapSealData.find(uRoleID);
	if (itor != m_mapSealData.end())
	{
		SealDataObject* pSealData = itor->second;
		ERROR_RETURN_FALSE(pSealData != NULL);

		pSealData->Destroy();

		m_mapSealData.erase(itor);
	}

	return TRUE;
}

BOOL CSealManager::IsSealRole(UINT64 uRoleID)
{
	SealDataObject* pSealObject = GetSealData(uRoleID);
	if (pSealObject == NULL)
	{
		return FALSE;
	}

	if (CommonFunc::GetCurrTime() > pSealObject->m_uSealTime)
	{
		return FALSE;
	}

	return TRUE;
}

SealDataObject* CSealManager::GetSealData(UINT64 uRoleID)
{
	auto itor = m_mapSealData.find(uRoleID);
	if (itor != m_mapSealData.end())
	{
		return itor->second;
	}

	return NULL;
}

