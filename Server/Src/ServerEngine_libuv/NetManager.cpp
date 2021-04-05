﻿#include "stdafx.h"
#include "NetManager.h"
#include "Connection.h"
#include "CommandDef.h"
#include "../ServerEngine/CommonConvert.h"
#include "Log.h"
#include "PacketHeader.h"
#include "DataBuffer.h"


void _Run_Loop(void* arg)
{

	CNetManager* pNetManager = (CNetManager*)arg;

	pNetManager->RunLoop();

	return;
}

void On_Async_Event(uv_async_t* handle)
{
	CConnection* pConnection = (CConnection*)handle->data;

	pConnection->DoSend();

	uv_close((uv_handle_t*)&pConnection->m_AsyncReq, NULL);

	return;
}

void On_Connection(uv_connect_t* req, int status)
{
	CConnection* pConnection = (CConnection*)req->data;
	if (status == 0)
	{
		//成功
		CNetManager::GetInstancePtr()->HandleConnect(pConnection, status);
	}
	else
	{
		//失败
		pConnection->Close();
	}

	return;
}

void On_RecvConnection(uv_stream_t* pServer, int status)
{
	if (status < 0)
	{
		return;
	}

	CConnection* pConnection = CConnectionMgr::GetInstancePtr()->CreateConnection();
	ERROR_RETURN_NONE(pConnection != NULL);

	CNetManager* pNetManager = CNetManager::GetInstancePtr();

	uv_tcp_init(pNetManager->m_pMainLoop, pConnection->GetSocket());

	if (uv_accept(pServer, (uv_stream_t*)pConnection->GetSocket()) == 0)
	{
		pNetManager->HandleAccept(pConnection, 0);
	}
	else
	{
		pConnection->Close();
	}

	return;
}


CNetManager::CNetManager(void)
{
	m_pBufferHandler	= NULL;
}

CNetManager::~CNetManager(void)
{
}

BOOL CNetManager::Start(UINT16 nPortNum, UINT32 nMaxConn, IDataHandler* pBufferHandler, std::string& strListenIp)
{
	ERROR_RETURN_FALSE(pBufferHandler != NULL);

	m_pBufferHandler = pBufferHandler;

	CConnectionMgr::GetInstancePtr()->InitConnectionList(nMaxConn);

	if (strListenIp.empty() || strListenIp.length() < 4)
	{
		strListenIp = "0.0.0.0";
	}

	m_pMainLoop = uv_default_loop();

	uv_tcp_init(m_pMainLoop, &m_ListenSocket);

	sockaddr_in addr;

	uv_ip4_addr(strListenIp.c_str(), nPortNum, &addr);

	uv_tcp_bind(&m_ListenSocket, (const struct sockaddr*)&addr, 0);

	int nRetCode = uv_listen((uv_stream_t*)&m_ListenSocket, 20, On_RecvConnection);
	if (nRetCode)
	{
		return FALSE;
	}

	uv_thread_create(&m_LoopThreadID, _Run_Loop, this);

	return TRUE;
}

BOOL CNetManager::Stop()
{
	uv_close((uv_handle_t*)&m_ListenSocket, NULL);

	uv_stop(m_pMainLoop);
	uv_loop_close(m_pMainLoop);
	uv_thread_join(&m_LoopThreadID);

	CConnectionMgr::GetInstancePtr()->CloseAllConnection();

	CConnectionMgr::GetInstancePtr()->DestroyAllConnection();

	return TRUE;
}

CConnection* CNetManager::ConnectTo_Sync(std::string strIpAddr, UINT16 sPort)
{
	return NULL;
}

CConnection* CNetManager::ConnectTo_Async( std::string strIpAddr, UINT16 sPort )
{
	struct sockaddr_in bind_addr;
	int iret = uv_ip4_addr(strIpAddr.c_str(), sPort, &bind_addr);
	if (iret)
	{
		return NULL;
	}

	CConnection* pConnection = CConnectionMgr::GetInstancePtr()->CreateConnection();
	if (pConnection == NULL)
	{
		return NULL;
	}

	iret = uv_tcp_init(m_pMainLoop, pConnection->GetSocket());
	pConnection->GetSocket()->data = pConnection;
	pConnection->m_ConnectReq.data = pConnection;
	pConnection->SetDataHandler(m_pBufferHandler);
	iret = uv_tcp_connect(&pConnection->m_ConnectReq, pConnection->GetSocket(), (const sockaddr*)&bind_addr, On_Connection);
	if (iret)
	{
		pConnection->Close();
	}

	return pConnection;
}


void CNetManager::HandleConnect(CConnection* pConnection, INT32 dwStatus)
{
	m_pBufferHandler->OnNewConnect(pConnection->GetConnectionID());

	pConnection->DoReceive();

	return ;
}

void CNetManager::HandleAccept(CConnection* pConnection, INT32 dwStatus)
{
	if (dwStatus == 0)
	{
		struct sockaddr_in ClientAddr;

		socklen_t namelen = sizeof(ClientAddr);

		uv_tcp_getpeername(pConnection->GetSocket(), (sockaddr*)&ClientAddr, &namelen);

		pConnection->m_dwIpAddr = ClientAddr.sin_addr.s_addr;

		m_pBufferHandler->OnNewConnect(pConnection->GetConnectionID());

		pConnection->SetDataHandler(m_pBufferHandler);

		pConnection->DoReceive();
	}
	else
	{
		pConnection->Close();
		//处理错误
	}

	return;
}


void CNetManager::RunLoop()
{
	uv_run(m_pMainLoop, UV_RUN_DEFAULT);

	return;
}

BOOL	CNetManager::SendMessageBuff(UINT32 dwConnID, IDataBuffer* pBuffer)
{
	ERROR_RETURN_FALSE(dwConnID != 0);
	ERROR_RETURN_FALSE(pBuffer != 0);
	CConnection* pConn = CConnectionMgr::GetInstancePtr()->GetConnectionByID(dwConnID);
	if (pConn == NULL)
	{
		//表示连接己经失败断开了，这个连接ID不可用了。
		return FALSE;
	}
	if (!pConn->IsConnectionOK())
	{
		CLog::GetInstancePtr()->LogError("CNetManager::SendMsgBufByConnID FAILED, 连接己断开");
		return FALSE;
	}

	pBuffer->AddRef();
	if (pConn->SendBuffer(pBuffer))
	{
		PostSendOperation(pConn);
		return TRUE;
	}

	return FALSE;
}


BOOL CNetManager::SendMessageData(UINT32 dwConnID, UINT32 dwMsgID, UINT64 u64TargetID, UINT32 dwUserData, const char* pData, UINT32 dwLen)
{
	if (dwConnID <= 0)
	{
		return FALSE;
	}

	CConnection* pConn = CConnectionMgr::GetInstancePtr()->GetConnectionByID(dwConnID);
	if (pConn == NULL)
	{
		//表示连接己经失败断开了，这个连接ID不可用了。
		return FALSE;
	}

	if (!pConn->IsConnectionOK())
	{
		CLog::GetInstancePtr()->LogError("CNetManager::SendMessageByConnID FAILED, 连接己断开");
		return FALSE;
	}

	IDataBuffer* pDataBuffer = CBufferAllocator::GetInstancePtr()->AllocDataBuff(dwLen + sizeof(PacketHeader));
	ERROR_RETURN_FALSE(pDataBuffer != NULL);

	PacketHeader* pHeader = (PacketHeader*)pDataBuffer->GetBuffer();
	pHeader->CheckCode = CODE_VALUE;
	pHeader->dwUserData = dwUserData;
	pHeader->u64TargetID = u64TargetID;
	pHeader->dwSize = dwLen + sizeof(PacketHeader);
	pHeader->dwMsgID = dwMsgID;
	pHeader->dwPacketNo = 1;

	memcpy(pDataBuffer->GetBuffer() + sizeof(PacketHeader), pData, dwLen);

	pDataBuffer->SetTotalLenth(pHeader->dwSize);

	if (pConn->SendBuffer(pDataBuffer))
	{
		PostSendOperation(pConn);
		return TRUE;
	}

	return FALSE;
}


BOOL CNetManager::PostSendOperation(CConnection* pConnection)
{
	ERROR_RETURN_FALSE(pConnection != NULL);

	if (!pConnection->m_IsSending)
	{
		uv_async_init(m_pMainLoop, &pConnection->m_AsyncReq, On_Async_Event);

		uv_handle_set_data((uv_handle_t*)&pConnection->m_AsyncReq, (void*)pConnection);

		uv_async_send(&pConnection->m_AsyncReq);
	}

	return TRUE;
}