#pragma once
//Qt.
#include <QMutex>
#include <QDialog>
#include <QDebug>
#include <QPixmap>
#include <QFileDialog>

//Auto-generated Qt files.
#include "ui_Settings.h"

//Other imports.
#include "UserData.hpp"

class SettingsWindow : public QDialog
{
	Q_OBJECT
	public:
		SettingsWindow(std::shared_ptr<QMutex>& sharedUserDataMutex, std::shared_ptr<UserData>& sharedUserData, std::shared_ptr<QMutex>& sharedDownloadFolderMutex, std::shared_ptr<QString>& sharedDownloadFolder, std::shared_ptr<QMutex>& autosaveMutex, std::shared_ptr<int>& autosave, bool userDataExists, QWidget *parent = 0);
		~SettingsWindow();
		/*Metodo che consente di sapere se la foto del profilo è cambiata o meno.*/
		bool PhotoChanged();

	signals:
		/*Segnale emesso in caso di errore.*/
		void SettingsWindowException(QString exception);

	public slots:
		/*Slot di scelta della foto profilo.*/
		void ChoosePhoto();
		/*Slot di scelta del percorso di salvataggio dei file ricevuti.*/
		void ChangeFolder();
		/*Slot di swapping dell'autosave dei file da true a false e viceversa.*/
		void ChangeAutosaveValue(int value);
		/*Slot di salvataggio dei dati dell'utente.*/
		void SaveButton();

	private:
		/*GUI.*/
		Ui::Settings*ui;

		/*MUTEX.*/
		/*Mutex condiviso relativo alla variabile sharedUserData.*/
		std::shared_ptr<QMutex> sharedUserDataMutex;
		/*Mutex condiviso relativo alla variabile SharedDownloadFolder.*/
		std::shared_ptr<QMutex> sharedDownloadFolderMutex;
		/*Mutex condiviso relativo alla variabile autosaveMutex.*/
		std::shared_ptr<QMutex> sharedAutosaveMutex;

		/*VARIABILI SHARED.*/
		/*Variabile di tipo UserData, contenente i dati dell'utente.*/
		std::shared_ptr<UserData> sharedUserData;
		/*Variabile recante il percorso in cui salvare i file ricevuti.*/
		std::shared_ptr<QString> sharedDownloadFolder;
		/*Variabile recante la volontà dell'utente di salvare i file ricevuti in modo automatico o manuale.*/
		std::shared_ptr<int> sharedAutosave;

		/*VARIABILI.*/
		/*Variabile booleana recante un valore vero o falso a seconda dell'esistenza o meno del file contenente le impostazioni del programma.*/
		bool settingsFileExists;
		/*Stringa contenente il percorso della foto.*/
		QString photo;
		/*Stringa contenente il percorso in cui salvare i file ricevuti.*/
		QString path;
		/*Variabile che indica se la foto profilo è stata cambiata o meno.*/
		bool changed;
};