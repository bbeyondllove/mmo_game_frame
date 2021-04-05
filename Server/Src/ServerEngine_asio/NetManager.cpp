﻿#include "stdafx.h"
#include "NetManager.h"
#include "Connection.h"
#include "CommandDef.h"
#include "boost/asio/placeholders.hpp"
#include <boost/asio/impl/connect.hpp>
#include "../ServerEngine/CommonConvert.h"
#include "Log.h"
#include "PacketHeader.h"
#include "DataBuffer.h"


CNetManager::CNetManager(void)
{
	m_pBufferHandler	= NULL;
}

CNetManager::~CNetManager(void)
{
	m_pBufferHandler = NULL;
}

BOOL CNetManager::Start(UINT16 nPortNum, UINT32 nMaxConn, IDataHandler* pBufferHandler, std::string& strListenIp)
{
	ERROR_RETURN_FALSE(pBufferHandler != NULL);

	m_pBufferHandler = pBufferHandler;

	CConnectionMgr::GetInstancePtr()->InitConnectionList(nMaxConn, m_IoService);

	if (strListenIp.empty() || strListenIp.length() < 4)
	{
		strListenIp = "0.0.0.0";
	}

	boost::asio::ip::tcp::endpoint ep(boost::asio::ip::address_v4::from_string(strListenIp), nPortNum);

	m_pAcceptor = new boost::asio::ip::tcp::acceptor(m_IoService, ep, true);

	//m_pAcceptor->set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));

	WaitForConnect();

	m_pWorkThread = new boost::thread(boost::bind(&boost::asio::io_service::run, &m_IoService));

	return TRUE;
}

BOOL CNetManager::Stop()
{
	m_IoService.stop();

	m_pAcceptor->close();

	delete m_pAcceptor;

	m_pAcceptor = NULL;

	if (m_pWorkThread != NULL)
	{
		m_pWorkThread->join();

		delete m_pWorkThread;

		m_pWorkThread = NULL;
	}

	CConnectionMgr::GetInstancePtr()->CloseAllConnection();

	CConnectionMgr::GetInstancePtr()->DestroyAllConnection();

	return TRUE;
}


BOOL CNetManager::WaitForConnect()
{
	CConnection* pConnection = CConnectionMgr::GetInstancePtr()->CreateConnection();
	if (pConnection == NULL)
	{
		return FALSE;
	}
	pConnection->SetDataHandler(m_pBufferHandler);
	m_pAcceptor->async_accept(pConnection->GetSocket(), boost::bind(&CNetManager::HandleAccept, this,  pConnection, boost::asio::placeholders::error));

	return TRUE;
}

CConnection* CNetManager::ConnectTo_Sync(std::string strIpAddr, UINT16 sPort)
{
	return NULL;
}

CConnection* CNetManager::ConnectTo_Async( std::string strIpAddr, UINT16 sPort )
{
	//boost::asio::ip::tcp::resolver resolver(m_IoService);
	//boost::asio::ip::tcp::resolver::query query(serverip.c_str(), Helper::IntToString(portnumber));
	//boost::asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
	//boost::asio::ip::tcp::resolver::iterator end;
	//boost::system::error_code error = boost::asio::error::host_not_found;

	CConnection* pConnection = CConnectionMgr::GetInstancePtr()->CreateConnection();
	ERROR_RETURN_NULL(pConnection != NULL);

	//boost::asio::ip::tcp::endpoint ep(boost::asio::ip::address::from_string(strIpAddr), sPort);

	boost::asio::ip::tcp::resolver resolver(m_IoService);
	boost::asio::ip::tcp::resolver::query query(strIpAddr, CommonConvert::IntToString(sPort));
	boost::asio::ip::tcp::resolver::iterator enditor = resolver.resolve(query);
	pConnection->SetDataHandler(m_pBufferHandler);
	boost::asio::async_connect(pConnection->GetSocket(), enditor, boost::bind(&CNetManager::HandleConnect, this, pConnection, boost::asio::placeholders::error));

	return pConnection;
}


void CNetManager::HandleConnect(CConnection* pConnection, const boost::system::error_code& e)
{
	if (!e)
	{
		m_pBufferHandler->OnNewConnect(pConnection->GetConnectionID());

		pConnection->DoReceive();
	}
	else
	{
		pConnection->Close();
	}

	return ;
}

void CNetManager::HandleAccept(CConnection* pConnection, const boost::system::error_code& e)
{
	if (!e)
	{
		m_pBufferHandler->OnNewConnect(pConnection->GetConnectionID());

		pConnection->DoReceive();
	}
	else
	{
		pConnection->Close();
		//这里是监听出错，需要处理．
	}

	WaitForConnect();

	return ;
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
		m_IoService.post(boost::bind(&CConnection::DoSend, pConnection));
	}

	return TRUE;
}