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

class TCPSendFileThread : public QThread
{
	Q_OBJECT

public:
	TCPSendFileThread(QString senderUserName, QString receiverUserName, QString hostIP, QString path);
	~TCPSendFileThread();

signals:
	/*Segnale di ricezione della scelta effettuata dall'altro host.*/
	void Choice();
	/*Segnale di aggiornamento della barra di download del file e di stima del tempo rimanente.*/
	void UpdateBar(QString ip, qint64 value, qint64 estimatedRemainingTime, quint16 sender);
	/*Segnale di reset dello stato a inattivo (0).*/
	void ResetState(QString ip, int value);
	/*Segnale di gestione delle eccezioni in sede di invio.*/
	void SendFileRequestExceptionSignal(QString exception);

public slots:
	/*Slot richiamato ad avvenuta disconnessione con l'altro host.*/
	void Disconnected();
	/*Slot richiamato quando son disponibili nuovi dati sul buffer: verrà chiamato solamente quando l'utente comunicherà la scelta riguardante la richiesta di invio del file.*/
	void ReadyRead();
	/*Slot di cancellazione dell'upload in corso.*/
	void CancelCurrentUploadSlot(QString ip);

protected:
	void run() override;

private:
	/*VARIABILI*/
	/*Socket TCP.*/
	QTcpSocket *tcpSocket;
	/*Variabile contenente la risposta dell'altro host.*/
	quint16 response;
	/*Variabile contenente l'username dell'utente che sta inviando il file.*/
	QString senderUserName;
	/*Variabile contenente l'username dell'utente che sta ricevendo il file.*/
	QString receiverUserName;
	/*Variabile contenente il path del file*/
	QString path;
	/*Variabile contenente l'ip dell'host ricevente.*/
	QString hostIP;
	/*Variabile contenente il nome del file da inviare.*/
	QString fileName;
	/*Variabile contenente la dimensione del file da inviare.*/
	qint64 fileSize;
	/*Variabile contenente il numero di bytes letti.*/
	qint64 readSize;
	/*Variabile contenente la percentuale di dati ricevuti sinora.*/
	qint64 downloadPercentage;
	/*Variabile di calcolo delle differenze, utile per ridurre il numero di segnali di aggiornamento della barra di download emessi.*/
	qint64 difference;
	/*Variabile di calcolo della stima del tempo rimanente al completamento dell'upload.*/
	qint64 remainingTime;
	/*Variabile di salvataggio del tempo trascorso.*/
	qint64 previousTimeSinceEpoch;
	/*Variabile recante lo stato del download: true=cancellato, false=attivo.*/
	bool canceled;
};