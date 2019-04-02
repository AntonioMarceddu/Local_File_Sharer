#include "UDPReceiverThread.hpp"

UDPReceiverThread::UDPReceiverThread(std::shared_ptr<QMutex>& sharedDownloadFolderMutex, std::shared_ptr<std::set<UserData, UserDataComparator>>& sharedUsersSet, QObject *parent) : QThread(parent)
{
	this->sharedDownloadFolderMutex = sharedDownloadFolderMutex;
	this->sharedUsersSet = sharedUsersSet;
}

UDPReceiverThread::~UDPReceiverThread()
{
	//Chiusura del socket e schedulazione della sua eliminazione nell'event loop.
	udpSocket->close();
	udpSocket->deleteLater();
	//Azzeramento dei puntatori, utile al decremento del reference count.
	sharedDownloadFolderMutex = nullptr;
	sharedUsersSet = nullptr;
	//Terminazione del thread corrente.
	quit();
	wait();
}

void UDPReceiverThread::run()
{
	int flag = 0;
	QString name;
	std::set<QString> hostIpList;

	//Prelievo della lista degli ip legati all'host corrente.
	QList<QHostAddress> list = QNetworkInterface::allAddresses();

	//Ciclo di salvataggio degl indirizzi ipv4 dell'host corrente in hostIpList.
	for (int nIter = 0; nIter < list.count(); nIter++)
	{
		if (list[nIter].protocol() == QAbstractSocket::IPv4Protocol)
		{
			name = list[nIter].toString();
			hostIpList.insert(name);
		}
	}

	//Inizializzazione del socket.
	udpSocket = new QUdpSocket(nullptr);
	udpSocket->bind(broadcastReceivePort, QUdpSocket::ShareAddress);

	//Ciclo di attesa per nuovi datagrammi.
	while (udpSocket->waitForReadyRead(-1))
	{
		QByteArray datagram;
		QHostAddress senderAddress = QHostAddress();
		quint16  port = 0;
		//Ciclo di lettura dei datagrammi pendenti.
		while (udpSocket->hasPendingDatagrams())
		{
			//Prelievo del dato contenuto nel datagramma.
			datagram.resize(int(udpSocket->pendingDatagramSize()));
			udpSocket->readDatagram(datagram.data(), datagram.size(), &senderAddress, &port);
			name = datagram.constData();
			//L'ip viene restituito come un indirizzo ipv4 incapsulato in uno ipv6 (::ffff:x.y.w.z). Per ottenere l'indirizzo ipv4 è sufficiente rimuovere i primi 7 caratteri.
			QString senderAddressString = senderAddress.toString().remove(0, 7);
			//Ciclo di verifica della provenienza del messaggio.
			std::set<QString>::iterator it = hostIpList.begin();
			while ((it != hostIpList.end())&&(flag==0))
			{
				if (*it == senderAddressString)
				{
					flag = 1;
				}
				++it;
			}
			//Il messaggio verrà considerato solamente se non proviene dall'host corrente.
			if (flag == 0)
			{
				//Aggiunta dell'utente nel set condiviso.
				UserData user(name, senderAddressString);
				/*SEZIONE CRITICA.*/
				sharedDownloadFolderMutex->lock();
				sharedUsersSet->insert(UserData(name, senderAddressString, QPixmap(":/images/DefaultUser"), false));
				/*FINE SEZIONE CRITICA.*/
				sharedDownloadFolderMutex->unlock();
			}
			//Reset del flag.
			flag = 0;
		}
	}
	return;
}