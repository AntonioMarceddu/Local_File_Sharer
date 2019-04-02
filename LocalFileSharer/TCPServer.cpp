#include <TCPServer.hpp>
#include <TCPReceiverThread.hpp>

TCPServer::TCPServer(std::shared_ptr<QMutex>& sharedDownloadFolderMutex, std::shared_ptr<QString>& sharedDownloadFolder, QObject *parent) : QTcpServer(parent)
{
	this->sharedDownloadFolderMutex = sharedDownloadFolderMutex;
	this->sharedDownloadFolder = sharedDownloadFolder;
}

TCPServer::~TCPServer()
{
	//Azzeramento dei puntatori, utile al decremento del reference count.
	sharedDownloadFolderMutex = nullptr;
	sharedDownloadFolder = nullptr;
}

/*Per ogni connessione in ingresso viene istanziato un thread che si occupa di gestire la richiesta.*/
void TCPServer::incomingConnection(qintptr socketDescriptor)
{
	TCPReceiverThread *tcpReceiverThread = new TCPReceiverThread(socketDescriptor, sharedDownloadFolderMutex, sharedDownloadFolder, this);
	connect(tcpReceiverThread, SIGNAL(finished()), tcpReceiverThread, SLOT(deleteLater()));
	connect(tcpReceiverThread, SIGNAL(FileReceptionRequestTCPServer(QString, QString, QString, qint64)), this, SLOT(FileReceptionRequestSlot(QString, QString, QString, qint64)));
	connect(this, SIGNAL(FileTransferChoice(QString, quint16)), tcpReceiverThread, SLOT(FileTransferChoice(QString, quint16)));
	connect(tcpReceiverThread, SIGNAL(UpdateBarTCPServer(QString, qint64, qint64, quint16)), this, SLOT(UpdateBarSlot(QString, qint64, qint64, quint16)));
	connect(this, SIGNAL(CancelCurrentDownload(QString)), tcpReceiverThread, SLOT(CancelDownload(QString)));
	connect(tcpReceiverThread, SIGNAL(ProfilePhotoReceivedTCPServer(QString)), this, SLOT(ProfilePhotoReceivedSlot(QString)));
	connect(tcpReceiverThread, SIGNAL(ResetStateTCPServer(QString, int)), this, SLOT(ResetStateSlot(QString, int)));
	connect(tcpReceiverThread, SIGNAL(ReceiveFileExceptionTCPServer(QString)), this, SLOT(ReceiveFileExceptionSlot(QString)));
	tcpReceiverThread->start();
}

void TCPServer::FileReceptionRequestSlot(QString ip, QString userName, QString file, qint64 fileSize)
{
	emit FileReceptionRequest(ip, userName, file, fileSize);
}

void TCPServer::FileTransferChoiceSlot(QString ip, quint16 choice)
{
	emit FileTransferChoice(ip, choice);
}

void TCPServer::UpdateBarSlot(QString ip, qint64 value, qint64 estimatedRemainingTime, quint16 sender)
{
	emit UpdateBar(ip, value, estimatedRemainingTime, sender);
}

void TCPServer::CancelCurrentDownloadSlot(QString ip)
{
	emit CancelCurrentDownload(ip);
}

void TCPServer::ProfilePhotoReceivedSlot(QString ip)
{
	emit ProfilePhotoReceived(ip);
}

void TCPServer::ResetStateSlot(QString ip, int value)
{
	emit ResetState(ip, value);
}

void TCPServer::ReceiveFileExceptionSlot(QString exception)
{
	emit ReceiveFileException(exception);
}