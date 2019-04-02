#pragma once
//Qt.
#include <QThread>
#include <QDebug>
#include <QMutex>
#include <QFile>
#include <QString>
#include <QDateTime>

//Other imports.
#include "CommonNetworkImports.hpp"

class TCPReceiverThread : public QThread
{
	Q_OBJECT

public:
	TCPReceiverThread(int socketDescriptor, std::shared_ptr<QMutex>& sharedDownloadFolderMutex, std::shared_ptr<QString>& sharedDownloadFolder, QObject *parent = 0);
	~TCPReceiverThread();

signals:
	/*Segnale di richiesta di ricezione di un nuovo file.*/
	void FileReceptionRequestTCPServer(QString ip, QString user, QString file, qint64 fileSize);
	/*Segnale di aggiornamento della barra di download del file e di stima del tempo rimanente.*/
	void UpdateBarTCPServer(QString ip, qint64 value, qint64 estimatedRemainingTime, quint16 sender);
	/*Segnale emesso all'avvenuta ricezione di una nuova foto profilo.*/
	void ProfilePhotoReceivedTCPServer(QString ip);
	/*Segnale di reset dello stato a inattivo (0).*/
	void ResetStateTCPServer(QString ip, int value);
	/*Segnale emesso in caso di errore durante il download del file.*/
	void ReceiveFileExceptionTCPServer(QString exception);
	/*Segnali di interruzione degli event loop.*/
	void QuitEventLoopQ1();
	void QuitEventLoopQ2();

public slots:
	/*Slot richiamato ad avvenuta disconnessione con l'altro host.*/
	void Disconnected();
	/*Slot richiamato quando son disponibili nuovi dati sul buffer.*/
	void ReadyRead();
	/*Slot di ricezione della scelta effettuata dall'utente.*/
	void FileTransferChoice(QString ip, quint16 choice);
	/*Slot di cancellazione del download in corso.*/
	void CancelDownload(QString ip);

protected:
	void run() override;

private:
	/*MUTEX.*/
	/*Mutex condiviso relativo alla variabile SharedDownloadFolder.*/
	std::shared_ptr<QMutex> sharedDownloadFolderMutex;

	/*VARIABILI SHARED.*/
	/*Variabile recante il percorso in cui salvare i file ricevuti.*/
	std::shared_ptr<QString> sharedDownloadFolder;

	/*VARIABILI.*/
	/*Variabili legate alla connessione.*/
	QTcpSocket *tcpSocket;
	int socketDescriptor;

	/*Variabile contenente l'ip dell'host mittente.*/
	QString hostIPString;
	/*Variabile contenente la scelta effettuata dall'utente in caso di richiesta di ricezione di un file: 0 = richiesta accettata, 1 = richiesta rifiutata.*/
	quint16 userChoice;
	/*Variabile di movimento tra le diverse fasi della ricezione.*/
	int transition;
	/*Variabile contenente l'opzione attuale.*/
	quint16 option;
	/*Variabile contenente il nome dell'utente.*/
	QString userName;
	/*Variabile contenente il nome del file da ricevere.*/
	QString fileName;
	/*Variabile contenente la dimensione del blocco.*/
	quint32 blockSize;
	/*Variabile contenente la dimensione del file.*/
	qint64 fileSize;
	/*Variabile contenente la percentuale di dati ricevuti sinora.*/
	qint64 downloadPercentage;
	/*Variabile contenente il numero di bytes ricevuti.*/
	qint64 received;
	/*Variabile di calcolo delle differenze, utile per ridurre il numero di segnali emessi relativi all'aggiornamento della barra di download.*/
	qint64 difference;
	/*Variabile di calcolo della stima del tempo rimanente al completamento dell'upload.*/
	qint64 remainingTime;
	/*Variabile di prelievo del tempo attuale.*/
	QDateTime dateTime;
	/*Variabile di salvataggio del tempo trascorso.*/
	qint64 previousTimeSinceEpoch;
	/*Variabile recante lo stato del download: true=cancellato, false=attivo.*/
	bool canceled;
};
