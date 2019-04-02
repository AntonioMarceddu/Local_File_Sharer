#include "TCPSendPhotoRunnable.hpp"

TCPSendPhotoRunnable::TCPSendPhotoRunnable(QString serverIP, QString path)
{
	this->hostIP = serverIP;
	this->path = path;
}

/*Runnable di invio della foto profilo.*/
void TCPSendPhotoRunnable::run()
{
	//Creazione e connessione del QTcpSocket socket con il segnale disconnected.
	QTcpSocket socket;
	connect(&socket, &QTcpSocket::disconnected, &socket, &QObject::deleteLater);

	//Connessione all'IP hostIP tramite la porta TCPPort.
	socket.connectToHost(hostIP, TCPPort);
	if (!socket.waitForConnected())
	{
		return;
	}
	else
	{
		//Se la connessione non è stata stabilita il runnable verrà chiuso.
		if (socket.state() != QAbstractSocket::ConnectedState)
		{
			return;
		}
		//Creazione di un ByteArray e di una struttura di serializzazione dei dati.
		QByteArray block;
		QDataStream out(&block, QIODevice::WriteOnly);
		out.setVersion(QDataStream::Qt_5_11);
		out.startTransaction();

		//Verrà inviato all'altro host:
		//1) un qint16 avete come valore 2 per indicare l'invio della foto del profilo;
		//2) un qint32 contenente la dimensione dell'immagine (implicita in rawFile) e successivamente l'immagine stessa.
		QFile image(path);
		if (image.open(QIODevice::ReadOnly))
		{
			QByteArray rawFile = image.readAll();
			out << (quint16)2;
			out << rawFile;
			out.device()->seek(0);
			socket.write(block);
			socket.waitForBytesWritten(-1);
		}
		socket.disconnectFromHost();
	}
}