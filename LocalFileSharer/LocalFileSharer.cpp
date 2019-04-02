#include "LocalFileSharer.hpp"

LocalFileSharer::LocalFileSharer(QString parameter, QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
	//Rimozione preventiva della cartella contenente le foto profilo degli utenti in rete, utile in caso di arresto anomalo del programma.
	std::experimental::filesystem::remove_all(profilePhotoPath);

	//Prelievo dell'eventuale parametro ricevuto.
	this->parameter = parameter;

	//Verifica della presenza di un'eventuale istanza attiva del programma.
	instance = new SingleInstance(parameter);
	//Se è presente, le viene inoltrato il parametro ricevuto.
	if (instance->HasPrevious())
	{
		exit(0);
	}
	else
	{
		//Creazione di un listener per verificare la presenza di istanze del programma create in tempi successivi.
		if (!instance->Listen())
		{
			FatalExceptionManager("Error while creating the instance listener.");
			exit(0);
		}
		else
		{
			//Creazione della cartella contenente le foto profilo degli utenti in rete.
			std::experimental::filesystem::create_directory(profilePhotoPath);

			//Visualizzazione della finestra.
			ui->setupUi(this);
			//Variazione del colore dello slider in verde per indicare che l'utente è online.
			setStyleSheet("QSlider::handle:horizontal {background-color: green;}");

			//Chiamata alla funzione di istanziazione delle variabili condivise.
			InstantiateSharedVariables();

			//Inizializzazione dei thread di rete.
			tcpServer = new TCPServer(sharedDownloadFolderMutex, sharedDownloadFolder);
			udpReceiverThread = new UDPReceiverThread(sharedUserSetMutex, sharedUsersSet);
			udpSenderThread = new UDPSenderThread(sharedUserDataMutex, sharedUserData, isOnlineMutex, isOnline);

			//Inizializzazione del thread di aggiornamento della lista mostrata a schermo.
			listUpdateSignalEmitter = new ListUpdateSignalEmitter();

			//Creazione delle connessioni tra i segnali e gli slot.
			CreateConnections();

			//Prelievo delle impostazioni del programma.
			FetchFileSettings();

			//Istanziazione del listener TCP.
			if (!tcpServer->listen(QHostAddress::Any, TCPPort))
			{
				FatalExceptionManager("The start of the TCP message listening server failed with the following error: " + tcpServer->errorString());
				exit(0);
			}

			//Avvio dei thread di rete e del thread di aggiornamento periodico della lista.
			udpReceiverThread->start();
			udpSenderThread->start();
			listUpdateSignalEmitter->start();

			//Istanziazione della tray icon e delle sue funzionalità.
			TrayIconManager();
		}
	}
}

LocalFileSharer::~LocalFileSharer()
{
	//Chiusura del server di ascolto TCP.
	tcpServer->close();

	//Rimozione della cartella contenente le foto profilo degli utenti in rete.
	std::experimental::filesystem::remove_all(profilePhotoPath);

	//Rimozione dei puntatori nativi.
	delete ui;

	//Azzeramento dei puntatori, utile al decremento del reference count.
	/*MUTEX.*/
	sharedUserSetMutex = nullptr;
	sharedUserDataMutex = nullptr;
	isOnlineMutex = nullptr;
	sharedDownloadFolderMutex = nullptr;
	sharedAutosaveMutex = nullptr;
	/*VARIABILI.*/
	sharedUserData = nullptr;
	sharedUsersSet = nullptr;
	isOnline = nullptr;
	sharedDownloadFolder = nullptr;
	sharedAutosave = nullptr;
}

/*Slot di segnalazione della corretta ricezione dei parametri inviati dalle altre istanze del programma.*/
void LocalFileSharer::ParameterReceived(QString parameter)
{
	//Protezione da errori di cattivo settaggio del registro di sistema di Windows.
	if (parameter != "::emptyFile")
	{
		this->parameter = parameter;
		QFileInfo file(parameter);
		show();
		QMessageBox::information(this, tr("Information"), "Select from the list the people you want to send the file " + file.fileName() + " and then click one of the send buttons at the bottom left.");
	}
}

/*Metodo di istanziazione delle variabili condivise.*/
void LocalFileSharer::InstantiateSharedVariables()
{
	/*MUTEX.*/
	sharedUserSetMutex = std::make_shared<QMutex>();
	sharedUserDataMutex = std::make_shared<QMutex>();
	isOnlineMutex = std::make_shared<QMutex>();
	sharedDownloadFolderMutex = std::make_shared<QMutex>();
	sharedAutosaveMutex = std::make_shared<QMutex>();

	/*VARIABILI.*/
	sharedUserData = std::make_shared<UserData>("", "127.0.0.1");
	sharedUsersSet = std::make_shared<std::set<UserData, UserDataComparator>>();
	isOnline = std::make_shared<bool>(true);
	sharedDownloadFolder = std::make_shared<QString>("C:/LocalFileSharerDownloadFolder");
	sharedAutosave = std::make_shared<int>(1);

	//Creazione della cartella di default del programma contenente i file ricevuti.
	//N.B. Non occorre usare mutex in quanto la variabile non è stata ancora condivisa con altri thread.
	std::experimental::filesystem::create_directory(sharedDownloadFolder->toStdString());
}

/*Metodo di connessione dei segnali con i corrispettivi slot.*/
void LocalFileSharer::CreateConnections()
{
	//Main Window.
	connect(ui->SendFileButton, SIGNAL(clicked()), this, SLOT(SendFile()));
	connect(ui->CancelButton, SIGNAL(clicked()), this, SLOT(Cancel()));
	connect(ui->UserDataButton, SIGNAL(clicked()), this, SLOT(Settings()));
	connect(ui->OnlineStatusSlider, SIGNAL(valueChanged(int)), this, SLOT(ChangeOnlineStatusWithParameters(int)));

	//TCP Server.
	connect(tcpServer, SIGNAL(FileReceptionRequest(QString, QString, QString, qint64)), this, SLOT(FileReception(QString, QString, QString, qint64)));
	connect(this, SIGNAL(FileTransferChoiceTCPServer(QString, quint16)), tcpServer, SLOT(FileTransferChoiceSlot(QString, quint16)));
	connect(tcpServer, SIGNAL(UpdateBar(QString, qint64, qint64, quint16)), this, SLOT(UpdateDownloadBar(QString, qint64, qint64, quint16)));
	connect(this, SIGNAL(CancelCurrentDownload(QString)), tcpServer, SLOT(CancelCurrentDownloadSlot(QString)));
	connect(tcpServer, SIGNAL(ProfilePhotoReceived(QString)), this, SLOT(UpdateUserProfilePhoto(QString)));
	connect(tcpServer, SIGNAL(ResetState(QString, int)), this, SLOT(UpdateState(QString, int)));
	connect(tcpServer, SIGNAL(ReceiveFileException(QString)), this, SLOT(NotFatalExceptionManager(QString)));

	//List Update Signal Emitter.
	connect(listUpdateSignalEmitter, SIGNAL(UpdateList()), this, SLOT(RefreshSet()));

	//Single Instance.
	connect(instance, SIGNAL(ParameterReceived(QString)), this, SLOT(ParameterReceived(QString)));
}

/*Slot di refresh del set, richiamato periodicamente.*/
void LocalFileSharer::RefreshSet()
{
	int i, flag;

	/*SEZIONE CRITICA.*/
	sharedUserSetMutex->lock();
	std::set<UserData, UserDataComparator>::iterator it;

	//Ciclo esterno di aggiunta di nuovi utenti alla lista.
	for (it = sharedUsersSet->begin(); it != sharedUsersSet->end(); ++it)
	{
		i = 0;
		flag = 0;
		//Ciclo interno di verifica della presenza dell'utente nella lista mostrata a schermo.
		while ((flag == 0) && (i < ui->listWidget->count()))
		{
			//Il confronto viene fatto tramite verifica degli IP: se l'utente è già presente, viene settato il flag ad 1.
			if (it->getIP() == ui->listWidget->item(i)->data(Qt::UserRole).toString())
			{
				flag = 1;
				//Verifica dell'eventuale variazione del nome dell'utente.
				if (it->getName() != ui->listWidget->item(i)->data(Qt::UserRole + 1))
				{
					ui->listWidget->item(i)->setData(Qt::UserRole + 1, it->getName());
					ui->listWidget->item(i)->setText(it->getName());
				}
			}
			i++;
		}
		//Se il flag è rimasto a 0 allora l'utente si è appena connesso.
		if (flag == 0)
		{
			QListWidgetItem *item = new QListWidgetItem(it->getName());
			//Verifica di eventuali foto profilo salvate in precedenza.
			QString photoPath = profilePhotoPath;
			photoPath.append("/").append(it->getIP());
			if (QFile::exists(photoPath))
			{
				item->setIcon(QIcon(QPixmap(photoPath)));
			}
			else
			{
				item->setIcon(QIcon(QPixmap(":/LocalFileSharer/DefaultUser")));	
			}
			//Settaggio dei ruoli per l'utente corrente.
			item->setData(Qt::UserRole, it->getIP()); //Indirizzo IP utente.
			item->setData(Qt::UserRole + 1, it->getName()); //Nome utente.
			item->setData(Qt::UserRole + 2, 0); //Stato: 0, inattivo.
			//Aggiunta dell'elemento in lista.
			ui->listWidget->addItem(item);

			/*SEZIONE CRITICA.*/
			sharedUserDataMutex->lock();
			//Se la foto profilo non è quella di default, essa viene inoltrata all'altro utente.
			if (!sharedUserData->isDefaultImage())
			{
				TCPSendPhotoRunnable* newSendPhoto = new TCPSendPhotoRunnable(it->getIP(), sharedUserData->getImagePath());
				QThreadPool::globalInstance()->start(newSendPhoto);
			}
			/*FINE SEZIONE CRITICA.*/
			sharedUserDataMutex->unlock();
		}
	}

	//Ciclo esterno di rimozione degli utenti andati offline.
	for (i = 0; i < ui->listWidget->count(); i++)
	{
		flag = 0;
		it = sharedUsersSet->begin();
		//Ciclo interno di verifica della presenza dell'utente nella lista mostrata a schermo.
		while ((flag == 0) && (it != sharedUsersSet->end()))
		{
			if (it->getIP() == ui->listWidget->item(i)->data(Qt::UserRole).toString())
			{
				flag = 1;
			}
			++it;
		}
		//Se il flag è rimasto a 0 allora l'utente si è disconnesso.
		if (flag == 0)
		{
			QListWidgetItem *item = ui->listWidget->item(i);
			ui->listWidget->removeItemWidget(item);
			delete item;
		}
	}

	//Rimozione del contenuto dello sharedUsersSet.
	sharedUsersSet->clear();
	/*FINE SEZIONE CRITICA.*/
	sharedUserSetMutex->unlock();
}

/*Slot di invio di un nuovo file verso gli utenti della lista selezionati.*/
void LocalFileSharer::SendFile()
{
	//Creazione di una lista contenente gli elementi della lista presente a schermo selezionati in precedenza dall'utente.
	QList <QListWidgetItem*> list = ui->listWidget->selectedItems();
	int counter = 0;
	//Caso in cui l'utente non abbia selezionato alcun utente dalla lista.
	if (list.size() == 0)
	{
		QMessageBox::information(this, tr("Information"), "You must select at least one recipient from the list above.");
	}
	//Caso in cui l'utente abbia selezionato almeno un utente dalla lista.
	else
	{
		QString file;
		//Se non è stato passato alcun parametro al programma mostro un dialog di scelta del file.
		if (parameter.isEmpty())
		{
			file = QFileDialog::getOpenFileName(this, ("Choose File To Send"), "/home");
			//Prelevo la lista aggiornata.
			list = ui->listWidget->selectedItems();
		}
		else
		{
			file = parameter;
		}
		//Se l'utente ha selezionato un file entro.
		if (file.length() != 0)
		{
			//Nel tempo impiegato dall'utente per scegliere un file, gli utenti presenti nella lista potrebbero essersi disconnessi.
			if (list.size() == 0)
			{
				QMessageBox::information(this, tr("Information"), "The user(s) that you selected went offline.");
			}
			else
			{
				//Ciclo effettuato su ogni elemento della lista.
				for (int i = 0; i < list.size(); ++i)
				{
					try
					{
						//Se lo stato dell'utente è inattivo (0), esso verrà aggiornato in upload (1) e gli verrà inoltrata una richiesta di invio file.
						if (list.at(i)->data(Qt::UserRole + 2).toInt() == 0)
						{
							UpdateState(list.at(i)->data(Qt::UserRole).toString(), 1);
							SendFileRequestToUser(list.at(i)->data(Qt::UserRole).toString(), list.at(i)->text(), file);
							counter++;
						}
					}
					catch (...)
					{
						NotFatalExceptionManager("Your request to send the file failed.");
					}
				}
				//Azzeramento della variabile parameter.
				parameter = "";
				//Nel caso in cui tutti gli utenti selezionati hanno un download o un upload in corso verrà mostrato all'utente un messaggio di avviso.
				if (counter == 0)
				{
					QMessageBox::information(this, tr("Information"), "An operation is already in progress for each of the selected users. Wait for them to complete before making new ones.");
				}
			}
		}
	}
}

/*Slot di creazione di una richiesta d'invio di un file verso l'utente selezionato.*/
void LocalFileSharer::SendFileRequestToUser(QString ip, QString userName, QString file)
{
	/*SEZIONE CRITICA.*/
	sharedUserDataMutex->lock();
	QString senderUserName = sharedUserData->getName();
	/*FINE SEZIONE CRITICA.*/
	sharedUserDataMutex->unlock();

	//Creazione di un nuovo thread di invio del file e delle sue connessioni.
	TCPSendFileThread *newSendFile = new TCPSendFileThread(senderUserName, userName, ip, file);
	connect(newSendFile, SIGNAL(finished()), newSendFile, SLOT(deleteLater()));
	connect(newSendFile, SIGNAL(UpdateBar(QString, qint64, qint64, quint16)), this, SLOT(UpdateDownloadBar(QString, qint64, qint64, quint16)));
	connect(this, SIGNAL(CancelCurrentUpload(QString)), newSendFile, SLOT(CancelCurrentUploadSlot(QString)));
	connect(newSendFile, SIGNAL(ResetState(QString, int)), this, SLOT(UpdateState(QString, int)));
	connect(newSendFile, SIGNAL(SendFileRequestExceptionSignal(QString)), this, SLOT(NotFatalExceptionManager(QString)));
	//Avvio del thread.
	newSendFile->start();
}

/*Slot di cancellazione del download o dell'upload corrente.*/
void LocalFileSharer::Cancel()
{
	//Creazione di una lista contenente gli elementi della lista presente a schermo selezionati in precedenza dall'utente.
	QList <QListWidgetItem*> list = ui->listWidget->selectedItems();
	int counter = 0;
	//Caso in cui l'utente non abbia selezionato alcun utente dalla lista.
	if (list.size() == 0)
	{
		QMessageBox::information(this, tr("Information"), "You must choose at least one download or upload in progress from the list above.");
	}
	//Caso in cui l'utente abbia selezionato almeno un utente dalla lista.
	else
	{
		//Ciclo effettuato su ogni elemento della lista.
		for (int i = 0; i < list.size(); ++i)
		{
			try
			{
				qDebug() << list.at(i)->text() << list.at(i)->data(Qt::UserRole + 2).toInt();
				//Se lo stato dell'utente è upload (1) o download(2) verrà emesso un segnale di cancellazione dell'operazione in corso e il suo stato verrà messo su inattivo(0).
				if (list.at(i)->data(Qt::UserRole + 2).toInt() == 1 || list.at(i)->data(Qt::UserRole + 2).toInt() == 2)
				{
					if (list.at(i)->data(Qt::UserRole + 2).toInt() == 1)
					{
						emit CancelCurrentUpload(list.at(i)->data(Qt::UserRole).toString());
					}
					else
					{
						emit CancelCurrentDownload(list.at(i)->data(Qt::UserRole).toString());
					}
					UpdateState(list.at(i)->data(Qt::UserRole).toString(), 0);
					counter++;
				}
			}
			catch (...) 
			{
				NotFatalExceptionManager("Your request to cancel the operation failed.");
			}
		}
		//Nel caso in cui tutti gli utenti selezionati sono inattivi verrà mostrato all'utente un messaggio di avviso.
		if (counter == 0)
		{
			QMessageBox::information(this, tr("Information"), "No downloads or uploads in progress for the selected users.");
		}
	}
}

/*Slot di richiesta di ricezione file.*/
void LocalFileSharer::FileReception(QString ip, QString userName, QString fileName, qint64 fileSize)
{
	//0 = accettazione della richiesta, 1= rifiuto della richiesta.
	quint16 choice = 0;

	//Prelievo delle informazioni relative al drive in cui si trova la cartella di salvataggio dei file ricevuti.
	/*SEZIONE CRITICA.*/
	sharedDownloadFolderMutex->lock();
	QStorageInfo storage = QStorageInfo(*sharedDownloadFolder);
	/*FINE SEZIONE CRITICA.*/
	sharedDownloadFolderMutex->unlock();

	//Se lo spazio a disposizione nel drive in cui si trova la cartella di salvataggio dei file ricevuti è inferiore rispetto alla dimensione del file, la richiesta di invio verrà automaticamente rifiutata.
	if (storage.bytesAvailable() < fileSize)
	{
		choice = 1;
		NotFatalExceptionManager(userName + " sent you a request to receive a new file, but the space available in the drive containing the destination folder is insufficient. The request was automatically canceled.");
	}
	else
	{
		//Prelievo del valore della variabile condivisa autosave.
		/*SEZIONE CRITICA.*/
		sharedAutosaveMutex->lock();
		int autosaveValue = *sharedAutosave;
		/*FINE SEZIONE CRITICA.*/
		sharedAutosaveMutex->unlock();

		//Se l'autosave è disabilitato, verrà creata una finestra che consente all'utente di scegliere di accettare o rifiutare la richiesta.
		if (autosaveValue == 1)
		{
			QString message = "The user " + userName + " would like to send you the file " + fileName + ". Would you like to accept it ? ";
			int button = QMessageBox::information(this, tr("Local File Sharer"), tr(message.toStdString().c_str()), QMessageBox::Yes | QMessageBox::No);
			//Se l'utente ha rifiutato la richiesta, choice verrà posto pari ad uno.
			if (button == QMessageBox::No)
			{
				choice = 1;
			}
		}

		//Se l'utente ha accettato la richiesta, lo stato dell'utente avente come indirizzo ip "ip" verrà aggiornato in download (2).
		if (choice == 0)
		{
			UpdateState(ip, 2);
		}
	}
	//Segnalazione della scelta effettuata dall'utente.
	emit FileTransferChoiceTCPServer(ip, choice);
}

/*Slot di aggiornamento della barra di download o di upload del file per l'utente avente come indirizzo ip "ip".*/
void LocalFileSharer::UpdateDownloadBar(QString ip, qint64 value, qint64 estimatedRemainingTime, quint16 sender)
{
	for (int i = 0; i < ui->listWidget->count(); i++)
	{
		if (ip == ui->listWidget->item(i)->data(Qt::UserRole).toString())
		{
			//Upload in atto.
			if (sender == 1)
			{
				if (value == 0)
				{
					ui->listWidget->item(i)->setText(ui->listWidget->item(i)->data(Qt::UserRole + 1).toString() + " - Uploading: " + QString::number(value) + "% - Estimated remaining time: computing...");
				}
				else
				{
					ui->listWidget->item(i)->setText(ui->listWidget->item(i)->data(Qt::UserRole + 1).toString() + " - Uploading: " + QString::number(value) + "% - Estimated remaining time: " + QString::number(estimatedRemainingTime) + "s");
				}
				//Grazie alla presenza della variabile sender, se un utente va offline e poi torna online lo stato non viene perso.
				ui->listWidget->item(i)->setData(Qt::UserRole + 2, 1);
			}
			//Download in atto.
			else if (sender == 2)
			{
				if (value == 0)
				{
					ui->listWidget->item(i)->setText(ui->listWidget->item(i)->data(Qt::UserRole + 1).toString() + " - Downloading: " + QString::number(value) + "% - Estimated remaining time: computing...");
				}
				else
				{
					ui->listWidget->item(i)->setText(ui->listWidget->item(i)->data(Qt::UserRole + 1).toString() + " - Downloading: " + QString::number(value) + "% - Estimated remaining time: " + QString::number(estimatedRemainingTime) + "s");
				}
				//Grazie alla presenza della variabile sender, se un utente va offline e poi torna online lo stato non viene perso.
				ui->listWidget->item(i)->setData(Qt::UserRole + 2, 2);
			}
			//Variazione dinamica del colore in base alla percentuale di completamento dell'upload o del download.
			if (value < 5)
			{
				ui->listWidget->item(i)->setBackgroundColor(QColor("#FF8F00"));
			}
			else if (value >= 5 && value < 10)
			{
				ui->listWidget->item(i)->setBackgroundColor(QColor("#FF9F00"));
			}
			else if (value >= 10 && value < 15)
			{
				ui->listWidget->item(i)->setBackgroundColor(QColor("#FFAF00"));
			}
			else if (value >= 15 && value < 20)
			{
				ui->listWidget->item(i)->setBackgroundColor(QColor("#FFBF00"));
			}
			else if (value >= 20 && value < 25)
			{
				ui->listWidget->item(i)->setBackgroundColor(QColor("#FFCF00"));
			}
			else if (value >= 25 && value < 30)
			{
				ui->listWidget->item(i)->setBackgroundColor(QColor("#FFDF00"));
			}
			else if (value >= 30 && value < 35)
			{
				ui->listWidget->item(i)->setBackgroundColor(QColor("#FFEF00"));
			}
			else if (value >= 35 && value < 40)
			{
				ui->listWidget->item(i)->setBackgroundColor(QColor("#FFFF00"));
			}
			else if (value >= 40 && value < 45)
			{
				ui->listWidget->item(i)->setBackgroundColor(QColor("#F0FF00"));
			}
			else if (value >= 45 && value < 50)
			{
				ui->listWidget->item(i)->setBackgroundColor(QColor("#E7FF00"));
			}
			else if (value >= 50 && value < 55)
			{
				ui->listWidget->item(i)->setBackgroundColor(QColor("#DFFF00"));
			}
			else if (value >= 55 && value < 60)
			{
				ui->listWidget->item(i)->setBackgroundColor(QColor("#D7FF00"));
			}
			else if (value >= 60 && value < 65)
			{
				ui->listWidget->item(i)->setBackgroundColor(QColor("#CFFF00"));
			}
			else if (value >= 65 && value < 70)
			{
				ui->listWidget->item(i)->setBackgroundColor(QColor("#BFFF00"));
			}
			else if (value >= 70 && value < 75)
			{
				ui->listWidget->item(i)->setBackgroundColor(QColor("#AFFF00"));
			}
			else if (value >= 75 && value < 80)
			{
				ui->listWidget->item(i)->setBackgroundColor(QColor("#80FF00"));
			}
			else if (value >= 80 && value < 85)
			{
				ui->listWidget->item(i)->setBackgroundColor(QColor("#60FF00"));
			}
			else if (value >= 85 && value < 90)
			{
				ui->listWidget->item(i)->setBackgroundColor(QColor("#40FF00"));
			}
			else if (value >= 90 && value < 95)
			{
				ui->listWidget->item(i)->setBackgroundColor(QColor("#20FF00"));
			}
			else
			{
				ui->listWidget->item(i)->setBackgroundColor(QColor("#00FF00"));
			}
		}
	}
}

/*Slot di aggiornamento della foto profilo dell'utente avente come indirizzo ip "ip".*/
void LocalFileSharer::UpdateUserProfilePhoto(QString ip)
{
	QString downloadPath = profilePhotoPath;
	downloadPath.append("/").append(ip);

	for (int i = 0; i < ui->listWidget->count(); i++)
	{
		if (ip == ui->listWidget->item(i)->data(Qt::UserRole).toString())
		{
			ui->listWidget->item(i)->setIcon(QIcon(QPixmap(downloadPath)));
		}
	}
}

/*Slot di invio della nuova foto profilo a tutti gli utenti della rete.*/
void LocalFileSharer::SendProfilePhotoToAllUsers()
{
	/*SEZIONE CRITICA.*/
	sharedUserSetMutex->lock();
	for (std::set<UserData, UserDataComparator>::iterator it = sharedUsersSet->begin(); it != sharedUsersSet->end(); ++it)
	{
		/*SEZIONE CRITICA.*/
		sharedUserDataMutex->lock();
		TCPSendPhotoRunnable* newSendPhoto = new TCPSendPhotoRunnable(it->getIP(), sharedUserData->getImagePath());
		/*FINE SEZIONE CRITICA.*/
		sharedUserDataMutex->unlock();
		QThreadPool::globalInstance()->start(newSendPhoto);
	}
	/*FINE SEZIONE CRITICA.*/
	sharedUserSetMutex->unlock();
}

/*Slot di aggiornamento dello stato per l'utente avente come indirizzo ip "ip". 0=inattivo, 1=upload, 2=download.*/
void LocalFileSharer::UpdateState(QString ip, int state)
{
	for (int i = 0; i < ui->listWidget->count(); i++)
	{
		if (ip == ui->listWidget->item(i)->data(Qt::UserRole).toString())
		{
			ui->listWidget->item(i)->setData(Qt::UserRole + 2, state);
			if (state == 0)
			{
				ui->listWidget->item(i)->setText(ui->listWidget->item(i)->data(Qt::UserRole + 1).toString());
				ui->listWidget->item(i)->setBackgroundColor(QColor("#FFFFFF"));
			}
		}
	}
}

/*Slot di swapping dello stato da Online ad Offline e viceversa con parametri.*/
void LocalFileSharer::ChangeOnlineStatusWithParameters(int value)
{
	if (value == 0)
	{
		SetOnline();
	}
	else
	{
		SetOffline();
	}

}

/*Slot di swapping dello stato da Online ad Offline e viceversa senza parametri.*/
void LocalFileSharer::ChangeOnlineStatusWithoutParameters()
{
	/*SEZIONE CRITICA: la proteggo con un mutex.*/
	isOnlineMutex->lock();
	bool state = *isOnline;
	/*FINE SEZIONE CRITICA.*/
	isOnlineMutex->unlock();
	if (state == true)
	{
		SetOffline();
	}
	else
	{
		SetOnline();
	}
}

void LocalFileSharer::SetOnline()
{
	/*SEZIONE CRITICA.*/
	isOnlineMutex->lock();
	*isOnline = true;
	/*FINE SEZIONE CRITICA.*/
	isOnlineMutex->unlock();

	//Aggiornamento dello slider.
	ui->OnlineOfflineLabel->setText("Online");
	ui->OnlineOfflineLabel->setStyleSheet("QLabel { color : green; }");
	setStyleSheet("QSlider::handle:horizontal {background-color: green;}");

	//Aggiornamento della Tray Icon.
	this->trayIcon->setIcon(QIcon(":/LocalFileSharer/Online"));
	trayIcon->setToolTip("Local File Sharer.\nStatus: Online.");
	stateAction->setText("Go Offline");
}

void LocalFileSharer::SetOffline()
{
	/*SEZIONE CRITICA:.*/
	isOnlineMutex->lock();
	*isOnline = false;
	/*FINE SEZIONE CRITICA.*/
	isOnlineMutex->unlock();

	//Aggiornamento dello slider.
	ui->OnlineOfflineLabel->setText("Offline");
	ui->OnlineOfflineLabel->setStyleSheet("QLabel { color : red; }");
	setStyleSheet("QSlider::handle:horizontal {background-color: red;}");

	//Aggiornamento della Tray Icon.
	this->trayIcon->setIcon(QIcon(":/LocalFileSharer/Offline"));
	trayIcon->setToolTip("Local File Sharer.\nStatus: Offline.");
	stateAction->setText("Go Online");
}

/*Slot che consente l'accesso alle impostazioni del programma e la loro variazione.*/
void LocalFileSharer::Settings()
{
	//Creazione della finestra contenente le impostazioni del programma.
	SettingsWindow set(sharedUserDataMutex, sharedUserData, sharedDownloadFolderMutex, sharedDownloadFolder, sharedAutosaveMutex, sharedAutosave, settingsFileExists);
	QObject::connect(&set, SIGNAL(SettingsWindowException(QString)), this, SLOT(NotFatalExceptionManager(QString)));
	set.exec();
	//Aggiornamento della GUI a seguito delle modifiche effettuate.
	UpdateUIPhotoAndName();
	//Verifica del cambiamento della foto profilo dell'utente: in caso affermativo, essa verrà inoltrata a tutti gli utenti della rete.
	if (set.PhotoChanged())
	{
		/*SEZIONE CRITICA.*/
		sharedUserDataMutex->lock();
		bool isDefault = sharedUserData->isDefaultImage();
		/*FINE SEZIONE CRITICA.*/
		sharedUserDataMutex->unlock();
		if (!isDefault)
		{
			SendProfilePhotoToAllUsers();
		}
	}
}

/*Metodo di lettura del file contenente le impostazioni del programma.*/
void LocalFileSharer::FetchFileSettings()
{
	//Apertura del file contenente le impostazioni del programma. Se esso non esiste, verrà visualizzata la finestra di settaggio delle impostazioni del programma.
	QFile file(qApp->applicationDirPath() + "/settings");
	if (!file.exists())
	{
		Settings();
	}
	//File esistente.
	else
	{
		//Apertura del file contenente le impostazioni del programma.
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			FatalExceptionManager("There was a problem while loading the program settings.");
		}
		else
		{
			//Creazione di un QTextStream per la lettura del file.
			QTextStream in(&file);
			QString line;

			//Lettura del nome dell'utente.
			line = in.readLine();

			/*SEZIONE CRITICA.*/
			sharedUserDataMutex->lock();
			sharedUserData->setName(line);

			//Lettura del percorso in cui è situata la foto profilo.
			line = in.readLine();
			//Se la foto profilo è quella di default essa verrà prelevata dalle risorse del programma.
			if (line == "default")
			{
				sharedUserData->setImage(QPixmap(":/LocalFileSharer/DefaultUser"));
				//Dico che la foto appena settata è quella di default.
				sharedUserData->setDefaultImage(true);
			}
			//Altrimenti verrà letta dal file.
			else
			{
				//Verifica dell'esistenza effettiva del file contenente la foto profilo.
				QFile photo(line);
				if (photo.exists())
				{
					//Lettura e salvataggio della foto e del suo percorso.
					QPixmap newImage(line);
					sharedUserData->setImage(newImage);
					sharedUserData->setImagePath(line);
					//Il booleano riportante l'uso della foto profilo di default verrà impostato a false.
					sharedUserData->setDefaultImage(false);
				}
				else
				{
					sharedUserData->setImage(QPixmap(":/LocalFileSharer/DefaultUser"));
					//Il booleano riportante l'uso della foto profilo di default verrà impostato a true.
					sharedUserData->setDefaultImage(true);
				}
			}
			/*FINE SEZIONE CRITICA.*/
			sharedUserDataMutex->unlock();

			//Lettura del percorso di salvataggio delle foto.
			line = in.readLine();

			/*SEZIONE CRITICA.*/
			sharedDownloadFolderMutex->lock();
			//Verifica dell'esistenza del percorso di salvataggio dei file: se non esiste, verrà assegnato allo shared pointer il percorso di download di default del programma.
			QFile downloadFolder(line);
			if (downloadFolder.exists())
			{
				*sharedDownloadFolder = line;
			}
			else
			{
				*sharedDownloadFolder = "C:/LocalFileSharerDownloadFolder";
			}
			/*FINE SEZIONE CRITICA.*/
			sharedDownloadFolderMutex->unlock();

			//Lettura della preferenza legata al salvataggio automatico dei file.
			line = in.readLine();
			/*SEZIONE CRITICA.*/
			sharedAutosaveMutex->lock();
			*sharedAutosave = line.toInt();
			/*FINE SEZIONE CRITICA.*/
			sharedAutosaveMutex->unlock();

			//Aggiornamento della GUI a seguito dei dati letti in precedenza
			UpdateUIPhotoAndName();
			//Chiusura del file delle impostazioni del programma.
			file.close();
		}
	}
	//Indico che il file delle impostazioni è esistente.
	settingsFileExists = true;
}

/*Metodo di aggiornamento della foto e del nome dell'utente mostrati sulla GUI.*/
void LocalFileSharer::UpdateUIPhotoAndName()
{
	/*SEZIONE CRITICA.*/
	sharedUserDataMutex->lock();
	ui->UserPhotoLabel->setPixmap(sharedUserData->getImage());
	ui->UserNameLabel->setText(sharedUserData->getName());
	/*FINE SEZIONE CRITICA.*/
	sharedUserDataMutex->unlock();
}

/*Metodo di gestione delle eccezioni gravi.*/
void LocalFileSharer::FatalExceptionManager(QString exception)
{
	//Cancellazione delle richieste pendenti del ThreadPool.
	QThreadPool::globalInstance()->clear();
	QMessageBox::critical(this, tr("Local File Sharer"), tr(exception.toStdString().c_str()), QMessageBox::Ok);
	//Chiusura dell'applicazione.
	QApplication::quit();
}

/*Metodo di gestione delle eccezioni non gravi.*/
void LocalFileSharer::NotFatalExceptionManager(QString exception)
{
	QMessageBox::warning(this, tr("Local File Sharer"), tr(exception.toStdString().c_str()), QMessageBox::Ok);
}

/*Metodo di creazione ed inizializzazione della tray icon.*/
void LocalFileSharer::TrayIconManager()
{
	//Istanziazione e settaggio delle proprietà della Tray Icon.
	trayIcon = new QSystemTrayIcon(this);
	trayIcon->setIcon(QIcon(":/LocalFileSharer/Online"));
	trayIcon->setToolTip("Local File Sharer.\nStatus: Online.");

	//Creazione del menù della Tray Icon.
	trayIconMenu = new QMenu(this);
	openAction = new QAction(trUtf8("Open"), this);
	settingsAction = new QAction(trUtf8("Settings"), this);
	stateAction = new QAction(trUtf8("Go Offline"), this);
	quitAction = new QAction(trUtf8("Quit"), this);
	connect(openAction, SIGNAL(triggered()), this, SLOT(show()));
	connect(settingsAction, SIGNAL(triggered()), this, SLOT(Settings()));
	connect(stateAction, SIGNAL(triggered()), this, SLOT(ChangeOnlineStatusWithoutParameters()));
	connect(quitAction, SIGNAL(triggered()), this, SLOT(CloseApp()));
	trayIconMenu->addAction(openAction);
	trayIconMenu->addAction(settingsAction);
	trayIconMenu->addAction(stateAction);
	trayIconMenu->addAction(quitAction);

	//Settaggio del menù dell'icona e visualizzazione.
	trayIcon->setContextMenu(trayIconMenu);
	trayIcon->show();
	connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(IconActivated(QSystemTrayIcon::ActivationReason)));
}

/*Slot richiamato all'attivazione della Tray Icon.*/
void LocalFileSharer::IconActivated(QSystemTrayIcon::ActivationReason reason)
{
	switch (reason)
	{
	case QSystemTrayIcon::Trigger:
		show();
		break;
	case QSystemTrayIcon::DoubleClick:
		ChangeOnlineStatusWithoutParameters();
		break;
	default:;
	}
}

/*Slot che setta l'opzione Esci del Tray Menù come unico punto d'uscita del programma.*/
void LocalFileSharer::CloseApp()
{
	QCoreApplication::quit();
}

/*Override Del metodo CloseEvent, richiamato al click dell'icona "X" nella barra del titolo.
Cliccando tale icona, l'applicazione non verrà chiusa definitavemente ma solo "nascosta".*/
void LocalFileSharer::closeEvent(QCloseEvent *event)
{
	trayIcon->showMessage(tr("Local File Sharer"), ("Application is running in background."));
	event->ignore();
	hide();
}