#include "SingleInstance.hpp"
SingleInstance::SingleInstance(QString parameter, QObject *parent) : QObject (parent)
{
	this->parameter = parameter;
	connect(&localServer, SIGNAL(newConnection()), this, SLOT(NewConnection()));
}

SingleInstance::~SingleInstance()
{
	localServer.close();
}

/*Metodo di ascolto per connessioni locali in ingresso.*/
bool SingleInstance::Listen()
{
	localServer.removeServer("it.polito.localfilesharer");
	if (!localServer.listen("it.polito.localfilesharer")) 
	{
		return false;
	}
	return true;
}

/*Metodo di verifica della presenza di istanze aperte in precedenza.*/
bool SingleInstance::HasPrevious()
{
	//Creazione di un socket per il testing della presenza di un eventuale QLocalServer in esecuzione.
	QLocalSocket socket;
	socket.connectToServer("it.polito.localfilesharer", QLocalSocket::ReadWrite);

	if (socket.waitForConnected())
	{
		//Protezione da errori di cattiva impostazione del registro di sistema.
		if (parameter.isEmpty())
		{
			parameter = "::emptyFile";
		}
		
		//Invio dei parametri verso l'istanza del programma in esecuzione.
		QByteArray block;
		QDataStream out(&block, QIODevice::WriteOnly);
		out.setVersion(QDataStream::Qt_5_11);
		out.startTransaction();
		out << parameter;
		out.device()->seek(0);
		socket.write(block);
		socket.waitForBytesWritten();
		return true;
	}

	return false;
}

/*Slot richiamato all'apertura di una nuova istanza.*/
void SingleInstance::NewConnection()
{
	localSocket = localServer.nextPendingConnection();
	connect(localSocket, SIGNAL(readyRead()), this, SLOT(ReadyRead()));
}

/*Slot di segnalazione di nuovi dati presenti sul socket.*/
void SingleInstance::ReadyRead()
{
	//Creazione di un QDatastream in grado di prelevare i parametri inoltrati dall'altra istanza del programma.
	QDataStream in(localSocket);
	in.setVersion(QDataStream::Qt_5_11);
	in >> parameter;
	localSocket->close();
	localSocket->deleteLater();
	emit ParameterReceived(parameter);
}