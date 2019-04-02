#include "TCPSendFileThread.hpp"

TCPSendFileThread::TCPSendFileThread(QString senderUserName, QString receiverUserName, QString hostIP, QString path)
{
	this->hostIP = hostIP;
	this->senderUserName = senderUserName;
	this->receiverUserName = receiverUserName;
	this->path = path;

	readSize = 0;
	fileSize = 0;
	difference = 0;
	remainingTime = 0;
	previousTimeSinceEpoch = 0;
	canceled = false;
}

TCPSendFileThread::~TCPSendFileThread()
{
	//Terminazione del thread corrente.
	quit();
	wait();
}

void TCPSendFileThread::run()
{
	//Istanziazione e connessione del tcpSocket con i segnali di readyRead e disconnected.
	tcpSocket = new QTcpSocket();
	connect(tcpSocket, SIGNAL(disconnected()), this, SLOT(Disconnected()));
	connect(tcpSocket, SIGNAL(readyRead()), this, SLOT(ReadyRead()));

	//Connessione all'IP hostIP tramite la porta TCPPort.
	tcpSocket->connectToHost(hostIP, TCPPort);
	if (!tcpSocket->waitForConnected(5000))
	{
		emit SendFileRequestExceptionSignal("TCPSendFileThread - There was a problem while connecting to host " + hostIP);
		return;
	}
	else
	{
		//Se la connessione non è stata stabilita il thread verrà chiuso.
		if (tcpSocket->state() != QAbstractSocket::ConnectedState)
		{
			emit SendFileRequestExceptionSignal("TCPSendFileThread - There was a problem while connecting to host " + hostIP);
			return;
		}
		//Creazione di un ByteArray e di una struttura di serializzazione dei dati.
		QByteArray block;
		QDataStream out(&block, QIODevice::WriteOnly);
		out.setVersion(QDataStream::Qt_5_11);
		out.startTransaction();

		QFile file(path);
		QFileInfo fileInfo(file.fileName());
		fileName = fileInfo.fileName();
		if (file.open(QIODevice::ReadOnly))
		{
			//In questa prima fase, verrà inviato all'altro host:
			//a) il nome dell'utente;
			//b) il nome del file;
			//c) la dimensione del file.
			fileSize = file.size();
			out << (quint16)1;
			out << senderUserName;
			out << fileName;
			out << fileSize;
			out.device()->seek(0);
			tcpSocket->write(block);
			tcpSocket->waitForBytesWritten(-1);
			//Creazione di un event loop che continui l'esecuzione solo dopo la ricezione del segnale di Choice.
			QEventLoop q1;
			connect(this, SIGNAL(Choice()), &q1, SLOT(quit()));
			q1.exec();
			//Ricezione della scelta effettuata dall'utente.
			QDataStream in(tcpSocket);
			in.setVersion(QDataStream::Qt_5_11);
			in >> response;
			//Caso in cui l'altro utente abbia accettato la richiesta di invio del file.
			QByteArray rawFile;			
			if (response == 0)
			{
				QDateTime dateTime;
				//Emissione del segnale di azzeramento della barra di scaricamento del file.
				emit UpdateBar(hostIP, (qint64)0, (qint64)0, (quint16)1);
				previousTimeSinceEpoch = dateTime.currentMSecsSinceEpoch();
				//Loop di lettura ed invio graduale del file.
				while ((readSize < fileSize) && (canceled == false) && (tcpSocket->state() == QAbstractSocket::ConnectedState))
				{
					rawFile = file.read((qint64)1024);
					if (rawFile.size() == 0)
					{
						break;
					}
					readSize = readSize + rawFile.size();
					//Calcolo della percentuale di scaricamento.
					downloadPercentage = (float)readSize / (float)fileSize * 100;
					//Se l'incremento della percentuale di invio del file è superiore all'1% verrà emesso un segnale di aggiornamento della barra di invio e di stima del tempo rimanente.
					if ((downloadPercentage - difference) >= 1)
					{
						remainingTime = (double)((dateTime.currentMSecsSinceEpoch() - previousTimeSinceEpoch) * (100 - downloadPercentage)) / (double)1000;
						previousTimeSinceEpoch = dateTime.currentMSecsSinceEpoch();
						emit UpdateBar(hostIP, downloadPercentage, remainingTime, (quint16)1);
						difference++;
					}
					tcpSocket->write(rawFile);
					tcpSocket->waitForBytesWritten(-1);
					rawFile.clear();
				}
				emit UpdateBar(hostIP, (qint64)100, (qint64)0, (quint16)1);
			}
			//Caso in cui l'altro utente abbia rifiutato la richiesta di invio del file: notifico il rifiuto all'utente.
			else
			{
				emit SendFileRequestExceptionSignal("Your request to send the file " + file.fileName() + " has been denied by the user " + receiverUserName + ".");
			}
		}
		//Caso di errore durante l'apertura del file da inviare.
		else
		{
			emit SendFileRequestExceptionSignal("There was a problem while opening the file to send. Error: " + file.error());
		}
		//Disconnessione e schedulazione della rimozione del socket.
		tcpSocket->disconnectFromHost();
		tcpSocket->deleteLater();
		//Emissione del segnale di reset dello stato dell'utente avente come indirizzo ip hostIPString.
		emit ResetState(hostIP, 0);
	}
}

/*Slot richiamato ad avvenuta disconnessione con l'altro host.*/
void TCPSendFileThread::Disconnected()
{
	if ((response == 0) && (readSize < fileSize) && (canceled == false))
	{
		emit SendFileRequestExceptionSignal("Your request to send the file " + fileName + " has been canceled by the user " + receiverUserName + ".");
	}
}

/*Slot richiamato quando son disponibili nuovi dati sul buffer: verrà chiamato solamente quando l'utente comunicherà la scelta riguardante la richiesta di invio del file.*/
void TCPSendFileThread::ReadyRead()
{
	emit Choice();
}

/*Slot di cancellazione dell'upload in corso.*/
void TCPSendFileThread::CancelCurrentUploadSlot(QString ip)
{
	if (hostIP == ip)
	{
		canceled = true;
	}
}