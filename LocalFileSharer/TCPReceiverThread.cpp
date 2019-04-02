#include "TCPReceiverThread.hpp"

TCPReceiverThread::TCPReceiverThread(int socketDescriptor, std::shared_ptr<QMutex>& sharedDownloadFolderMutex, std::shared_ptr<QString>& sharedDownloadFolder, QObject *parent) : QThread(parent)
{
	this->socketDescriptor = socketDescriptor;
	this->sharedDownloadFolderMutex = sharedDownloadFolderMutex;
	this->sharedDownloadFolder = sharedDownloadFolder;

	transition = 0;
	option = 0;
	fileSize = 0;
	blockSize = 0;
	received = 0;
	difference = 0;
	downloadPercentage = 0;
	remainingTime = 0;
	previousTimeSinceEpoch = 0;
	userChoice = 1;
	canceled = false;
}

TCPReceiverThread::~TCPReceiverThread()
{
	//Azzeramento dei puntatori, utile al decremento del reference count.
	sharedDownloadFolderMutex = nullptr;
	sharedDownloadFolder = nullptr;
	//Terminazione del thread corrente.
	quit();
	wait();
}

/*Metodo chiamato all'avvio del thread.*/
void TCPReceiverThread::run()
{
	//Istanziazione e connessione del tcpSocket con i segnali di readyRead e disconnected.
	tcpSocket = new QTcpSocket(nullptr);
	connect(tcpSocket, SIGNAL(disconnected()), this, SLOT(Disconnected()));
	connect(tcpSocket, SIGNAL(readyRead()), this, SLOT(ReadyRead()));

	//Verifica di validità del descrittore ricevuto.
	if (!tcpSocket->setSocketDescriptor(socketDescriptor))
	{
		ReceiveFileExceptionTCPServer("Data reception has failed with the following error: " + tcpSocket->error());
		return;
	}

	//L'ip viene restituito come un indirizzo ipv4 incapsulato in uno ipv6 (::ffff:x.y.w.z). Per ottenere l'indirizzo ipv4 è sufficiente rimuovere i primi 7 caratteri.
	hostIPString = tcpSocket->peerAddress().toString().remove(0, 7);

	//Creazione di un event loop che continui l'esecuzione solo dopo la ricezione del segnale di QuitEventLoopQ1.
	QEventLoop q1;
	connect(this, SIGNAL(QuitEventLoopQ1()), &q1, SLOT(quit()));
	q1.exec();

	//Se si è ricevuta una richiesta di invio di un file, si invierà all'altro host la scelta effettuata dall'utente.
	if (option == 1)
	{
		if (tcpSocket->state() == QAbstractSocket::ConnectedState)
		{
			QByteArray block;
			QDataStream out(&block, QIODevice::WriteOnly);
			out.setVersion(QDataStream::Qt_5_11);
			out << (quint16)userChoice; //0 = file accettato, 1 = file rifiutato.
			out.device()->seek(0);
			tcpSocket->write(block);
			tcpSocket->waitForBytesWritten(-1);

			//Emissione del segnale di azzeramento della barra di scaricamento del file.
			emit UpdateBarTCPServer(hostIPString, (qint64)0, (qint64)0, (quint16)2);
			previousTimeSinceEpoch = dateTime.currentMSecsSinceEpoch();

			//Creazione di un event loop che continui l'esecuzione solo dopo la ricezione del segnale di QuitEventLoopQ2.
			QEventLoop q2;
			connect(this, SIGNAL(QuitEventLoopQ2()), &q2, SLOT(quit()));
			q2.exec();
		}
		else
		{
			emit ReceiveFileExceptionTCPServer("The user " + userName + " has disconnected.");
		}
		//Emissione del segnale di reset dello stato dell'utente avente come indirizzo ip hostIPString.
		emit ResetStateTCPServer(hostIPString, 0);
	}
	//Disconnessione e schedulazione della rimozione del socket.
	tcpSocket->disconnectFromHost();
	tcpSocket->deleteLater();
}

/*Slot richiamato ad avvenuta disconnessione con l'altro host.*/
void TCPReceiverThread::Disconnected()
{

	//Nel caso di richiesta di ricezione di un file, se si è ricevuto una quantità di dati inferiore rispetto a quanto atteso verrà emesso un segnale di avvenuta cancellazione da parte dell'altro host.
	if (option == 1)
	{
		if ((received < fileSize) && (canceled == false))
		{
			if (userChoice == 0)
			{
				emit ReceiveFileExceptionTCPServer("The sending of the file " + fileName + " has been canceled by the user " + userName + ".");
			}
			emit QuitEventLoopQ2();
		}
	}
}

/*Slot richiamato quando son presenti nuovi valori nel buffer.*/
void TCPReceiverThread::ReadyRead()
{
	QDataStream in(tcpSocket);
	in.setVersion(QDataStream::Qt_5_11);

	//Al primo ingresso nella funzione ReadyRead la variabile option sarà pari a 0: occorre quindi leggere il valore ricevuto così da sapere l'operazione da effettuare.
	if (option == 0)
	{
		if (tcpSocket->bytesAvailable() < sizeof(quint16))
			return;
		in >> option;
	}

	//Opzione 1: richiesta di ricezione di un file.
	if (option == 1)
	{
		//Ricezione del nome dell'utente.
		if (transition == 0)
		{
			if (tcpSocket->bytesAvailable() < sizeof(QString))
			{
				return;
			}
			in >> userName;
			transition++;
		}
		//Ricezione del nome del file.
		if (transition == 1)
		{
			if (tcpSocket->bytesAvailable() < sizeof(QString))
			{
				return;
			}
			in >> fileName;
			transition++;
		}
		//Ricezione della dimensione del file.
		if (transition == 2)
		{
			if (tcpSocket->bytesAvailable() < sizeof(qint64))
			{
				return;
			}
			in >> fileSize;
			transition++;
			//Emissione del segnale di richiesta della scelta dell'utente riguardo alla richiesta di invio del file.
			emit FileReceptionRequestTCPServer(hostIPString, userName, fileName, fileSize);
			return;
		}
		//Loop di ricezione file.
		if ((transition == 3) && (canceled == false))
		{
			//Salvataggio del numero di bytes disponibili sul buffer.
			received = tcpSocket->bytesAvailable();
			//Calcolo della percentuale di scaricamento.
			downloadPercentage = (float)received / (float)fileSize * 100;
			//Se l'incremento della percentuale di scaricamento del file è superiore all'1% verrà emesso un segnale di aggiornamento della barra di scaricamento e di stima del tempo rimanente.
			if ((downloadPercentage - difference) >= 1)
			{
				remainingTime = (double)((dateTime.currentMSecsSinceEpoch() - previousTimeSinceEpoch) * (100 - downloadPercentage)) / (double)1000;
				previousTimeSinceEpoch = dateTime.currentMSecsSinceEpoch();
				emit UpdateBarTCPServer(hostIPString, downloadPercentage, remainingTime, (quint16)2);
				difference++;
			}
			//Caso in cui si sia ricevuto il numero di bytes atteso.
			if (received >= fileSize)
			{
				int count = 1;
				//Verifica dell'esistenza del percorso di scaricamento del file.
				QString downloadPath;
				/*SEZIONE CRITICA.*/
				sharedDownloadFolderMutex->lock();
				QFile downloadFolder(*sharedDownloadFolder);
				//Se esiste verrà assegnato alla variabile downloadPath.
				if (downloadFolder.exists())
				{
					downloadPath = *sharedDownloadFolder;
				}
				//Se non esiste verrà invece assegnato alla variabile downloadPath il path di scaricamento di default del programma.
				else
				{
					downloadPath = "C:/LocalFileSharerDownloadFolder";
				}
				/*FINE SEZIONE CRITICA.*/
				sharedDownloadFolderMutex->unlock();

				//Separazione dell'estensione del file dal resto del nome.
				QString name="",extension;
				QStringList fileSplit = fileName.split('.');
				for (int i = 0; i < fileSplit.size() - 1; i++)
				{
					if (i < fileSplit.size() - 2)
					{
						name.append(fileSplit.value(i)).append(".");
					}
					else
					{
						name.append(fileSplit.value(i));
					}
				}
				extension = fileSplit.value(fileSplit.size() - 1);

				//Calcolo di un nome univoco per il file.
				downloadPath.append("/");
				QString uniqueDownloadPath = downloadPath;
				uniqueDownloadPath = uniqueDownloadPath.append(name).append(".").append(extension);
				while (QFile::exists(uniqueDownloadPath))
				{
					QString number = QString::number(count);
					uniqueDownloadPath = downloadPath;
					uniqueDownloadPath = uniqueDownloadPath.append(name).append("(").append(number).append(")").append(".").append(extension);
					count++;
				}
				
				//Scrittura del file.
				QFile target(uniqueDownloadPath);
				if (!target.open(QIODevice::WriteOnly))
				{
					ReceiveFileExceptionTCPServer("It was not possible to save the file " + fileName + ": error opening the file.");
				}
				else
				{

					QByteArray buffer = tcpSocket->read(fileSize);
					target.write(buffer);
					target.close();
					//Emissione del segnale di completamento del download del file.
					emit UpdateBarTCPServer(hostIPString, (qint64)100, (qint64)100, (quint16)2);
				}
				transition++;
				emit QuitEventLoopQ2();
			}
			return;
		}
		//Cancellazione del download da parte dell'utente.
		else if ((transition == 3) && (canceled == true))
		{
			transition++;
			emit QuitEventLoopQ2();
		}
	}

	//Opzione 2: ricezione della foto profilo.
	if (option == 2)
	{
		//Ricezione della dimensione della foto profilo.
		if (transition == 0)
		{
			if (tcpSocket->bytesAvailable() < sizeof(quint32))
			{
				return;
			}
			in >> blockSize;
			transition++;
		}
		//Salvataggio nel profilePhotoPath della foto profilo ricevuta.
		if (transition == 1)
		{
			if (tcpSocket->bytesAvailable() >= blockSize)
			{
				QString downloadPath = profilePhotoPath;
				downloadPath.append("/").append(hostIPString);
				QFile photo(downloadPath);
				if (!photo.open(QIODevice::WriteOnly))
				{
					ReceiveFileExceptionTCPServer("It was not possible to save a user's profile photo: error opening the file.");
				}
				else
				{
					//Lettura da socket e scrittura su file della foto.
					QByteArray buffer = tcpSocket->read((qint64)blockSize);
					photo.write(buffer);
					photo.close();
					emit ProfilePhotoReceivedTCPServer(hostIPString);
				}
				transition++;
				emit QuitEventLoopQ1();
			}
		}
	}
}

/*Slot di ricezione della scelta effettuata dall'utente.*/
void TCPReceiverThread::FileTransferChoice(QString ip, quint16 choice)
{
	if (hostIPString == ip)
	{
		userChoice = choice;
		emit QuitEventLoopQ1();
	}
}

/*Slot di cancellazione del download in corso.*/
void TCPReceiverThread::CancelDownload(QString ip)
{
	if (hostIPString == ip)
	{
		canceled = true;
	}
}