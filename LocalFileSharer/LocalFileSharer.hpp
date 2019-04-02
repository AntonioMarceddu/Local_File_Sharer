#pragma once
//Standard C++.
#include <set>
#include <experimental/filesystem>

//Qt.
#include <QtWidgets/QMainWindow>
#include <QMessageBox>
#include <QInputDialog>
#include <QListWidgetItem>
#include <QDir>
#include <QLabel>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QCloseEvent>//override close event
#include <QApplication>

//Auto-generated Qt files.
#include "ui_MainWindow.h"

//Other imports.
#include "ListUpdateSignalEmitter.hpp"
#include "SettingsWindow.hpp"
#include "CommonNetworkImports.hpp"
#include "SingleInstance.hpp"

//Network threads and management.
#include "TCPServer.hpp"
#include "UDPReceiverThread.hpp"
#include "UDPSenderThread.hpp"
#include "TCPReceiverThread.hpp"
#include "TCPSendFileThread.hpp"
#include "TCPSendPhotoRunnable.hpp"


class LocalFileSharer : public QMainWindow
{
	Q_OBJECT

public:
	explicit LocalFileSharer(QString parameter, QWidget *parent = 0);
	~LocalFileSharer();

signals:
	/*Segnale di comunicazione della scelta effettuata dall'utente.*/
	void FileTransferChoiceTCPServer(QString ip, quint16 choice);
	/*Segnali di annullamento dell'upload o del download corrente.*/
	void CancelCurrentUpload(QString ip);
	void CancelCurrentDownload(QString ip);

public slots:
	/*Slot di segnalazione della corretta ricezione dei parametri inviate da altre istanze del programma.*/
	void ParameterReceived(QString parameter);
	/*Slot di refresh del set.*/
	void RefreshSet();
	/*Slot di invio di un nuovo file verso gli utenti della lista selezionati.*/
	void SendFile();
	/*Slot di creazione di una richiesta d'invio di un file verso l'utente selezionato.*/
	void SendFileRequestToUser(QString ip, QString userName, QString file);
	/*Slot di cancellazione del download o dell'upload corrente.*/
	void Cancel();
	/*Slot di richiesta di ricezione di un nuovo file.*/
	void FileReception(QString ip, QString user, QString fileName, qint64 fileSize);
	/*Slot di aggiornamento della barra di download o di upload del file per l'utente avente come indirizzo ip "ip".*/
	void UpdateDownloadBar(QString ip, qint64 value, qint64 estimatedRemainingTime, quint16 sender);
	/*Slot di aggiornamento della foto profilo dell'utente avente come indirizzo ip "ip".*/
	void UpdateUserProfilePhoto(QString ip);
	/*Slot di invio della nuova foto profilo a tutti gli utenti della rete.*/
	void SendProfilePhotoToAllUsers();
	/*Slot di aggiornamento dello stato per l'utente avente come indirizzo ip "ip". 0=inattivo, 1=upload, 2=download.*/
	void UpdateState(QString ip, int state);
	/*Slot di swapping dello stato da Online ad Offline e viceversa, con parametri e senza.*/
	void ChangeOnlineStatusWithParameters(int value);
	void ChangeOnlineStatusWithoutParameters();
	void SetOnline();
	void SetOffline();
	/*Slot che consente l'accesso alle impostazioni del programma e la loro variazione.*/
	void Settings();
	/*Slot di gestione delle eccezioni gravi e non gravi.*/
	void FatalExceptionManager(QString exception);
	void NotFatalExceptionManager(QString exception);
	/*Slot richiamato all'attivazione della Tray Icon.*/
	void IconActivated(QSystemTrayIcon::ActivationReason);
	/*Slot che si occupa di settare l'opzione Esci del Tray Menù come unico punto d'uscita del programma.*/
	void CloseApp();

protected:
	/*Override Del metodo CloseEvent, richiamato al click dell'icona "X" nella barra del titolo.
	Cliccando tale icona, l'applicazione non verrà chiusa definitavemente ma solo "nascosta".*/
	void closeEvent(QCloseEvent *event) override;

private:
	/*METODI.*/
	/*Metodo di istanziazione delle variabili condivise.*/
	void InstantiateSharedVariables();
	/*Metodo di connessione dei segnali con i corrispettivi slot.*/
	void CreateConnections();
	/*Metodo di lettura del file contenente le impostazioni del programma.*/
	void FetchFileSettings();
	/*Metodo di aggiornamento della foto e del nome dell'utente mostrati sulla GUI.*/
	void UpdateUIPhotoAndName();
	/*Metodo di creazione ed inizializzazione della tray icon.*/
	void TrayIconManager();

	/*THREAD E GESTIONE.*/
	/*Gestore delle connessioni TCP in entrata.*/
	TCPServer *tcpServer;
	/*Thread di ricezione dei messaggi UDP multicast.*/
	UDPReceiverThread *udpReceiverThread;
	/*Thread di invio dei messaggi UDP online.*/
	UDPSenderThread *udpSenderThread;
	/*Thread di aggiornamento periodico della lista degli utenti.*/
	ListUpdateSignalEmitter *listUpdateSignalEmitter;

	/*MUTEX.*/
	/*Mutex condiviso relativo alla variabile sharedUserSet.*/
	std::shared_ptr<QMutex> sharedUserSetMutex;
	/*Mutex condiviso relativo alla variabile sharedUserData.*/
	std::shared_ptr<QMutex> sharedUserDataMutex;
	/*Mutex condiviso relativo alla variabile isOnline.*/
	std::shared_ptr<QMutex> isOnlineMutex;
	/*Mutex condiviso relativo alla variabile SharedDownloadFolder.*/
	std::shared_ptr<QMutex> sharedDownloadFolderMutex;
	/*Mutex condiviso relativo alla variabile autosaveMutex.*/
	std::shared_ptr<QMutex> sharedAutosaveMutex;

	/*VARIABILI SHARED.*/
	/*Variabile di tipo UserData, contenente i dati dell'utente.*/
	std::shared_ptr<UserData> sharedUserData;
	/*Set contenente gli utenti presenti nella rete locale.*/
	std::shared_ptr<std::set<UserData, UserDataComparator>> sharedUsersSet;
	/*Variabile riportante lo stato dell'utente: true=online, false=offline.*/
	std::shared_ptr<bool> isOnline;
	/*Variabile recante il percorso in cui salvare i file ricevuti.*/
	std::shared_ptr<QString> sharedDownloadFolder;
	/*Variabile recante la volontà dell'utente di salvare i file ricevuti in modo automatico o manuale.*/
	std::shared_ptr<int> sharedAutosave;

	/*VARIABILE GUI.*/
	Ui::MainWindow *ui;

	/*VARIABILI TRAY ICON.*/
	/*Tray Icon.*/
	QSystemTrayIcon* trayIcon;
	/*Menù.*/
	QMenu *trayIconMenu;
	/*Opzioni del menù.*/
	QAction *openAction;
	QAction *settingsAction;
	QAction *stateAction;
	QAction *quitAction;

	/*ALTRE VARIABILI.*/
	/*Variabile booleana recante un valore vero o falso a seconda dell'esistenza o meno del file contenente le impostazioni del programma.*/
	bool settingsFileExists = false;
	/*Variabile di gestione delle istanze multiple del programma e di ricezione parametri.*/
	SingleInstance *instance;
	/*Parametro ricevuto dall'esterno, utile per inviare file tramite il Windows Context Menù.*/
	QString parameter = "";
};