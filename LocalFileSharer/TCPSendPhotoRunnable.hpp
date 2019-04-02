#pragma once
//Qt.
#include <QDebug>
#include <QRunnable>
#include <QFile>
#include <QString>

//Other imports.
#include "CommonNetworkImports.hpp"

class TCPSendPhotoRunnable : public QObject, public QRunnable
{
	Q_OBJECT

	public:
		TCPSendPhotoRunnable(QString serverIP, QString path);

	protected:
		void run() override;

	private:
		/*VARIABILI*/
	/*Variabile contenente l'ip dell'host ricevente.*/
		QString hostIP;
		/*Variabile contenente il path del file.*/
		QString path;
};