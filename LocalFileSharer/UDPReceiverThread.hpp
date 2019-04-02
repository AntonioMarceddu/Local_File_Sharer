#pragma once
//Standard C++.
#include <set>

//Qt.
#include <QThread>
#include <QDebug>
#include <QMutex>
#include <QPixmap>
#include <QString>

//Other imports.
#include "UserData.hpp"
#include "UserDataComparator.hpp"
#include "CommonNetworkImports.hpp"


class QUdpSocket;

class UDPReceiverThread : public QThread
{
	Q_OBJECT

	public:
		UDPReceiverThread(std::shared_ptr<QMutex>& sharedDownloadFolderMutex, std::shared_ptr<std::set<UserData, UserDataComparator>>& sharedUsersSet, QObject *parent = 0);
		~UDPReceiverThread();
	
	protected:
		void run() override;

	private:
		/*VARIABILI.*/
		/*Socket di ascolto dei messaggio UDP.*/
		QUdpSocket *udpSocket;
		/*Mutex condiviso relativo alla variabile sharedUsersSet.*/
		std::shared_ptr<QMutex> sharedDownloadFolderMutex;
		/*Set contenente gli utenti presenti nella rete locale.*/
		std::shared_ptr<std::set<UserData, UserDataComparator>> sharedUsersSet;
};