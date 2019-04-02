#pragma once
//Qt.
#include <QtNetwork> 

/*UDP Sender.*/
#define broadcastSendSleepTime 1000	

/*UDP broadcast Sender-Receiver.*/
#define broadcastSendPort 12345	
#define broadcastReceivePort 54321	
#define broadcastMsgBufSize 256

/*TCP.*/
#define TCPPort 23456
#define TCPMsgBufSize 256

/*Delimitatore di stringa.*/
#define delimiter "-.-"

/*Impostazioni foto profilo.*/
#define profilePhotoPath "c:/profilephoto"