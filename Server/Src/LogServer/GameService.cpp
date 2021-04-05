﻿#include "stdafx.h"
#include "GameService.h"
#include "../Message/Msg_Game.pb.h"
#include "../Message/Msg_RetCode.pb.h"
#include "../Message/Msg_ID.pb.h"
#include "WatcherClient.h"

CGameService::CGameService(void)
{

}

CGameService::~CGameService(void)
{

}

CGameService* CGameService::GetInstancePtr()
{
	static CGameService _GameService;

	return &_GameService;
}

BOOL CGameService::Init()
{
	CommonFunc::SetCurrentWorkDir("");

	if(!CLog::GetInstancePtr()->Start("LogServer", "log"))
	{
		return FALSE;
	}
	CLog::GetInstancePtr()->LogInfo("---------服务器开始启动--------");
	if(!CConfigFile::GetInstancePtr()->Load("servercfg.ini"))
	{
		CLog::GetInstancePtr()->LogError("配制文件加载失败!");
		return FALSE;
	}

	if (CommonFunc::IsAlreadyRun("LogServer" + CConfigFile::GetInstancePtr()->GetStringValue("areaid")))
	{
		CLog::GetInstancePtr()->LogError("LogServer己经在运行!");
		return FALSE;
	}

	CLog::GetInstancePtr()->SetLogLevel(CConfigFile::GetInstancePtr()->GetIntValue("log_log_level"));

	UINT16 nPort = CConfigFile::GetInstancePtr()->GetRealNetPort("log_svr_port");
	if (nPort <= 0)
	{
		CLog::GetInstancePtr()->LogError("配制文件log_svr_port配制错误!");
		return FALSE;
	}

	INT32  nMaxConn = CConfigFile::GetInstancePtr()->GetIntValue("log_svr_max_con");
	if(!ServiceBase::GetInstancePtr()->StartNetwork(nPort, nMaxConn, this))
	{
		CLog::GetInstancePtr()->LogError("启动服务失败!");
		return FALSE;
	}

	m_LogMsgHandler.Init(0);
	CLog::GetInstancePtr()->LogError("---------服务器启动成功!--------");
	return TRUE;
}


BOOL CGameService::OnNewConnect(UINT32 nConnID)
{
	CWatcherClient::GetInstancePtr()->OnNewConnect(nConnID);

	return TRUE;
}

BOOL CGameService::OnCloseConnect(UINT32 nConnID)
{
	CWatcherClient::GetInstancePtr()->OnCloseConnect(nConnID);

	return TRUE;
}

BOOL CGameService::OnSecondTimer()
{
	CWatcherClient::GetInstancePtr()->OnSecondTimer();

	return TRUE;
}

BOOL CGameService::DispatchPacket(NetPacket* pNetPacket)
{
	if (CWatcherClient::GetInstancePtr()->DispatchPacket(pNetPacket))
	{
		return TRUE;
	}

	if (m_LogMsgHandler.DispatchPacket(pNetPacket))
	{
		return TRUE;
	}

	return FALSE;
}

BOOL CGameService::Uninit()
{
	CLog::GetInstancePtr()->LogError("==========服务器开始关闭=======================");

	ServiceBase::GetInstancePtr()->StopNetwork();

	google::protobuf::ShutdownProtobufLibrary();

	CLog::GetInstancePtr()->LogError("==========服务器关闭完成=======================");

	return TRUE;
}

BOOL CGameService::Run()
{
	while (CWatcherClient::GetInstancePtr()->IsRun())
	{
		ServiceBase::GetInstancePtr()->Update();

		m_LogMsgHandler.OnUpdate(CommonFunc::GetTickCount());

		CommonFunc::Sleep(1);
	}

	return TRUE;
}

