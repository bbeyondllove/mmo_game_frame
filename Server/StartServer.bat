:start
@echo off
color 0A
echo. �X�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�[
echo. �U  ��ѡ��Ҫ���еĲ�����Ȼ�󰴻س�  �U
echo. �U�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�U
echo. �U                                  �U
echo. �U     1.����ȫ��������(DEBUG)      �U
echo. �U                                  �U
echo. �U     2.����ȫ��������(RELEASE)    �U
echo. �U                                  �U
echo. �U     3.�ر�ȫ��������             �U
echo. �U                                  �U
echo. �U     4.����ѹ�����Թ���           �U
echo. �U                                  �U
echo. �U     5.�����Ļ                   �U
echo. �U                                  �U
echo. �U     6.�˳������               �U
echo. �^�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�a
echo.             
set DebugDir=%cd%\Src\Bin\Debug64\
set ReleaseDir=%cd%\Src\Bin\Release64\
set PressDir=%cd%\Client\\PressureTest\Debug\
:cho
set choice=
set /p choice=          ��ѡ��:
IF NOT "%choice%"=="" SET choice=%choice:~0,1%
if /i "%choice%"=="1" start /D %DebugDir%  /MIN  StartServer.bat
if /i "%choice%"=="2" start /D %ReleaseDir% /MIN StartServer.bat
if /i "%choice%"=="3" taskkill /im LoginServer.exe & taskkill /im ProxyServer.exe & taskkill /im DBServer.exe & taskkill /im GameServer.exe & taskkill /im LogServer.exe & taskkill /im LogicServer.exe & taskkill /im PressureTest.exe & taskkill /im AccountServer.exe /im LoginServer_d.exe & taskkill /im ProxyServer_d.exe & taskkill /im DBServer_d.exe & taskkill /im GameServer_d.exe & taskkill /im LogServer_d.exe & taskkill /im LogicServer_d.exe & taskkill /im PressureTest.exe & taskkill /im AccountServer_d.exe
if /i "%choice%"=="4" start /D %ClientDir% %ClientDir%PressureTest.exe
if /i "%choice%"=="5" cls & goto start
if /i "%choice%"=="6" exit
if /i "%choice%"=="7" type %DebugDir%ServerCfg.ini
echo.
goto cho


