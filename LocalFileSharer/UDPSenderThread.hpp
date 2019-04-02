#pragma once
//Standard C++.
#include <string>

//Qt.
#include <QThread>
#include <QDebug>
#include <QMutex>

//Other imports.
#include "CommonNetworkImports.hpp"
#include "UserData.hpp"

class UDPSenderThread : public QThread
{
	Q_OBJECT

	public:
		UDPSenderThread(std::shared_ptr<QMutex>& sharedUserDataMutex, std::shared_ptr<UserData> sharedUserData, std::shared_ptr<QMutex>& isOnlineMutex, std::shared_ptr<bool> isOnline, QObject *parent = 0);
		~UDPSenderThread();

	protected:
		void run() override;

	private:
		/*MUTEX.*/
		/*Mutex condiviso relativo alla variabile sharedUserData.*/
		std::shared_ptr<QMutex> sharedUserDataMutex;
		/*Mutex condiviso relativo alla variabile isOnline.*/
		std::shared_ptr<QMutex> isOnlineMutex;

		/*VARIABILI SHARED.*/
		/*Variabile di tipo UserData, contenente i dati dell'utente.*/
		std::shared_ptr<UserData> sharedUserData;
		/*Variabile riportante lo stato dell'utente: true=online, false=offline.*/
		std::shared_ptr<bool> isOnline;

		/*VARIABILI.*/
		/*Socket di invio dei messaggi UDP.*/
		QUdpSocket *udpSocket = nullptr;
		/*Variabile contenente il nome dell'utente.*/
		std::string sharedUserName;
};