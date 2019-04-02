#pragma once
//Qt.
#include <QDebug>
#include <QThread>

//Other imports.
#include "CommonNetworkImports.hpp"

class ListUpdateSignalEmitter : public QThread
{
	Q_OBJECT

	signals:
		/*Segnale di richiesta di un nuovo aggiornamento della lista.*/
		void UpdateList();

	protected:
		void run() override;
};
