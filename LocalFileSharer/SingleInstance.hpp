#pragma once

#include <QByteArray>
#include <QDataStream>
#include <QDebug>
#include <QLocalServer>
#include <QLocalSocket>

class SingleInstance : public QObject
{
	Q_OBJECT
	public:
		explicit SingleInstance(QString parameter, QObject *parent = 0);
		~SingleInstance();
			
		/*Metodo di ascolto per connessioni locali in ingresso.*/
		bool Listen();
		/*Metodo di verifica della presenza di istanze aperte in precedenza.*/
		bool HasPrevious();

	signals:
		/*Segnale di corretta ricezione del parametro.*/
		void ParameterReceived(QString parameter);

	public slots:
		/*Slot richiamato all'apertura di una nuova istanza.*/
		void NewConnection();
		/*Slot di segnalazione di nuovi dati presenti sul socket.*/
		void ReadyRead();

	private:
		/*Socket locale.*/
		QLocalSocket *localSocket;
		/*Server locale.*/
		QLocalServer localServer;
		/*Parametri ricevuti.*/
		QString parameter;
};