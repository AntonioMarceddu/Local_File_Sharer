#include "UDPSenderThread.hpp"

UDPSenderThread::UDPSenderThread(std::shared_ptr<QMutex>& sharedUserDataMutex, std::shared_ptr<UserData> sharedUserData, std::shared_ptr<QMutex>& isOnlineMutex, std::shared_ptr<bool> isOnline, QObject *parent) : QThread(parent)
{
	this->sharedUserDataMutex = sharedUserDataMutex;
	this->sharedUserData = sharedUserData;
	this->isOnlineMutex = isOnlineMutex;
	this->isOnline = isOnline;

	//Inizializzazione del socket.
	udpSocket = new QUdpSocket();
	udpSocket->bind(broadcastSendPort, QUdpSocket::ShareAddress);
}

UDPSenderThread::~UDPSenderThread()
{
	//Chiusura del socket e schedulazione della sua eliminazione nell'event loop.
	udpSocket->close();
	udpSocket->deleteLater();
	//Azzeramento dei puntatori, utile al decremento del reference count.
	sharedUserDataMutex = nullptr;
	sharedUserData = nullptr;
	isOnlineMutex = nullptr;
	isOnline = nullptr;
	//Terminazione del thread corrente.
	quit();
	wait();
}

void UDPSenderThread::run()
{
	QByteArray name;

	while (true)
	{
		//Prelievo dello stato del collegamento.
		/*SEZIONE CRITICA.*/
		isOnlineMutex->lock();
		bool state = *isOnline;
		/*FINE SEZIONE CRITICA.*/
		isOnlineMutex->unlock();

		//Se l'utente è online il suo username verrà inoltrato agli altri utenti della rete.
		if (state == true)
		{
			/*SEZIONE CRITICA.*/
			sharedUserDataMutex->lock();
			name = sharedUserData->getName().toStdString().c_str();
			/*FINE SEZIONE CRITICA.*/
			sharedUserDataMutex->unlock();

			udpSocket->writeDatagram(name, QHostAddress::Broadcast, broadcastReceivePort);
		}

		//Sleep di attesa del successivo invio.
		msleep(broadcastSendSleepTime);
	}
}