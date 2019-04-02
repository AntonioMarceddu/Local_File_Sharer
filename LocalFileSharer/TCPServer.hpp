#pragma once
//Qt.
#include <QTcpServer>
#include <QThread>
#include <QDebug>
#include <QMutex>

//Other imports.
#include "TCPReceiverThread.hpp"
#include "UserData.hpp"
#include "UserDataComparator.hpp"

class TCPServer : public QTcpServer
{
	Q_OBJECT

public:
	TCPServer(std::shared_ptr<QMutex>& sharedDownloadFolderMutex, std::shared_ptr<QString>& sharedDownloadFolder, QObject *parent = 0);
	~TCPServer();

signals:
	/*Segnale di richiesta di ricezione di un nuovo file.*/
	void FileReceptionRequest(QString ip, QString userName, QString file, qint64 fileSize);
	/*Segnale di emissione della scelta effettuata dall'utente riguardo la volontà di scaricare o meno il file.*/
	void FileTransferChoice(QString ip, quint16 choice);
	/*Segnale di aggiornamento della barra di download del file e di stima del tempo rimanente.*/
	void UpdateBar(QString ip, qint64 value, qint64 estimatedRemainingTime, quint16 sender);
	/*Segnale di cancellazione del download in corso.*/
	void CancelCurrentDownload(QString ip);
	/*Segnale di avvenuta cancellazione del download del file.*/
	void DownloadCanceled(QString message);
	/*Segnale emesso all'avvenuta ricezione di una nuova foto profilo.*/
	void ProfilePhotoReceived(QString ip);
	/*Segnale di reset dello stato a inattivo (0).*/
	void ResetState(QString ip, int value);
	/*Segnale emesso in caso di errore durante il download del file.*/
	void ReceiveFileException(QString exception);

public slots:
	/*Slot di richiesta di ricezione di un nuovo file.*/
	void FileReceptionRequestSlot(QString ip, QString userName, QString file, qint64 fileSize);
	/*Slot di emissione della scelta effettuata dall'utente riguardo la volontà di scaricare o meno il file.*/
	void FileTransferChoiceSlot(QString ip, quint16 choice);
	/*Slot di aggiornamento della barra di download del file e di stima del tempo rimanente.*/
	void UpdateBarSlot(QString ip, qint64 value, qint64 estimatedRemainingTime, quint16 sender);
	/*Slot di avvenuta cancellazione del download del file.*/
	void CancelCurrentDownloadSlot(QString ip);
	/*Slot di avvenuta ricezione di una nuova foto profilo.*/
	void ProfilePhotoReceivedSlot(QString ip);
	/*Slot di reset dello stato a inattivo (0).*/
	void ResetStateSlot(QString ip, int value);
	/*Segnale emesso in caso di errore durante il download del file.*/
	void ReceiveFileExceptionSlot(QString exception);

protected:
	void incomingConnection(qintptr socketDescriptor) override;

private:
	/*Mutex condiviso relativo alla variabile SharedDownloadFolder.*/
	std::shared_ptr<QMutex> sharedDownloadFolderMutex;
	/*Variabile recante il percorso in cui salvare i file ricevuti.*/
	std::shared_ptr<QString> sharedDownloadFolder;
};
