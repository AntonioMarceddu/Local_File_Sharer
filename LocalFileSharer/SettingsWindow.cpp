#include "SettingsWindow.hpp"

SettingsWindow::SettingsWindow(std::shared_ptr<QMutex>& sharedUserDataMutex, std::shared_ptr<UserData>& sharedUserData, std::shared_ptr<QMutex>& sharedDownloadFolderMutex, std::shared_ptr<QString>& sharedDownloadFolder, std::shared_ptr<QMutex>& sharedAutosaveMutex, std::shared_ptr<int>& sharedAutosave, bool settingsFileExists, QWidget *parent) : ui(new Ui::Settings)
{
	this->sharedUserDataMutex = sharedUserDataMutex;
	this->sharedUserData = sharedUserData;
	this->sharedDownloadFolderMutex = sharedDownloadFolderMutex;
	this->sharedDownloadFolder = sharedDownloadFolder;
	this->sharedAutosaveMutex = sharedAutosaveMutex;
	this->sharedAutosave = sharedAutosave;
	this->settingsFileExists = settingsFileExists;
	changed = false;

	//Rimozione della barra del titolo dalla finestra.
	setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
	//Visualizzazione della finestra.
	ui->setupUi(this);

	//Se non si è in presenza di un un nuovo utente, verranno visualizzati nella finestra i dati salvati in precedenza.
	if (settingsFileExists == true)
	{
		/*SEZIONE CRITICA.*/
		sharedUserDataMutex->lock();
		if (this->sharedUserData->getName() != "")
		{
			ui->EditUserName->setText(this->sharedUserData->getName());
		}

		if (this->sharedUserData->isDefaultImage() == false)
		{
			ui->UserPhotoImage->setPixmap(this->sharedUserData->getImage());
		}
		/*FINE SEZIONE CRITICA.*/
		sharedUserDataMutex->unlock();

		/*SEZIONE CRITICA.*/
		sharedDownloadFolderMutex->lock();
		ui->EditDownloadFolder->setText(*sharedDownloadFolder);
		/*FINE SEZIONE CRITICA.*/
		sharedDownloadFolderMutex->unlock();

		/*SEZIONE CRITICA.*/
		sharedAutosaveMutex->lock();
		ui->AutosaveSlider->setValue(*sharedAutosave);
		ChangeAutosaveValue(*sharedAutosave);
		/*FINE SEZIONE CRITICA.*/
		sharedAutosaveMutex->unlock();
	}

	//Creazione delle connessioni tra i segnali e gli slot.
	QObject::connect(ui->ChangePhotoButton, SIGNAL(clicked()), this, SLOT(ChoosePhoto()));
	QObject::connect(ui->ChangeFolderButton, SIGNAL(clicked()), this, SLOT(ChangeFolder()));
	QObject::connect(ui->AutosaveSlider, SIGNAL(valueChanged(int)), this, SLOT(ChangeAutosaveValue(int)));
	QObject::connect(ui->SaveUserDataButton, SIGNAL(clicked()), this, SLOT(SaveButton()));
}

SettingsWindow::~SettingsWindow()
{
	//Rimozione dei puntatori nativi.
	delete ui;

	//Azzeramento dei puntatori, utile al decremento del reference count.
	sharedUserDataMutex = nullptr;
	sharedUserData = nullptr;
	sharedDownloadFolderMutex = nullptr;
	sharedDownloadFolder = nullptr;
	sharedAutosaveMutex = nullptr;
	sharedAutosave = nullptr;
}

/*Slot di scelta della foto profilo.*/
void SettingsWindow::ChoosePhoto()
{
	photo = QFileDialog::getOpenFileName(this, tr("Choose photo"), "/home", tr("Images (*.bmp *.png *.jpg)"));

	//Se l'utente ha effettivamente scelto una foto essa verrà mostrata in output.
	if (photo.length() != 0)
	{
		ui->UserPhotoImage->setPixmap(QPixmap(photo));
	}
}

/*Slot di scelta del percorso di salvataggio dei file ricevuti.*/
void SettingsWindow::ChangeFolder()
{
	QString dir = QFileDialog::getExistingDirectory(this, tr("Choose download directory"), "/home",	QFileDialog::ShowDirsOnly| QFileDialog::DontResolveSymlinks);

	//Se l'utente ha effettivamente scelto un percorso essa verrà mostrato in output.
	if (dir.length() != 0)
	{
		ui->EditDownloadFolder->setText(dir);
	}
}

/*Slot di swapping dell'autosave dei file da true a false e viceversa.*/
void SettingsWindow::ChangeAutosaveValue(int value)
{
	if (value == 0)
	{
		ui->AutosaveStatus->setText("On");
		ui->AutosaveStatus->setStyleSheet("QLabel { color : green; }");
		setStyleSheet("QSlider::handle:horizontal {background-color: green;}");
	}
	else
	{
		ui->AutosaveStatus->setText("Off");
		ui->AutosaveStatus->setStyleSheet("QLabel { color : red; }");
		setStyleSheet("QSlider::handle:horizontal {background-color: red;}");
	}
}

/*Slot di salvataggio dei dati dell'utente.*/
void SettingsWindow::SaveButton()
{
	QString name = ui->EditUserName->text();
	//Verifica della presenza di un nome utente della barra apposita.
	if (!name.isEmpty())
	{
		//Aggiornamento del valore della variabile condivisa recante il percorso di download dei file ricevuti.
		/*SEZIONE CRITICA.*/
		sharedDownloadFolderMutex->lock();
		*sharedDownloadFolder = ui->EditDownloadFolder->text();
		/*FINE SEZIONE CRITICA.*/
		sharedDownloadFolderMutex->unlock();

		//Aggiornamento del valore della variabile condivisa recante la volontà dell'utente di salvare i file ricevuti in modo automatico o manuale.
		/*SEZIONE CRITICA.*/
		sharedAutosaveMutex->lock();
		*sharedAutosave = ui->AutosaveSlider->value();
		/*FINE SEZIONE CRITICA.*/
		sharedAutosaveMutex->unlock();

		//Creazione o apertura del file contenente le impostazioni del programma per la scrittura dei dati aggiornati.
		/*SEZIONE CRITICA.*/
		sharedUserDataMutex->lock();
		QFile file(qApp->applicationDirPath() + "/settings");
		if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
		{
			QFileInfo info(file);
			emit SettingsWindowException("There was a problem while writing the program settings changes. " + info.absolutePath());
		}
		else
		{
			QTextStream stream(&file);
			//Salvataggio del nome scelto dall'utente.
			sharedUserData->setName(name);
			//Caso in cui l'utente non abbia cambiato la foto profilo.
			if (photo.length() == 0)
			{
				//Caso in cui l'utente abbia aperto per la prima volta l'applicazione.
				if (settingsFileExists == false)
				{
					sharedUserData->setImage(QPixmap(":/LocalFileSharer/DefaultUser"));
					sharedUserData->setDefaultImage(true);
					stream << name << "\n" << "default" << "\n" << ui->EditDownloadFolder->text() << "\n" << ui->AutosaveSlider->value();
				}
				//Caso in cui l'utente non abbia aperto per la prima volta l'applicazione.
				else
				{
					//Verifica dell'uso della foto profilo di default o di una foto personalizzata.
					if (sharedUserData->isDefaultImage() == true)
					{
						stream << name << "\n" << "default" << "\n" << ui->EditDownloadFolder->text() << "\n" << ui->AutosaveSlider->value();
					}
					else
					{
						stream << name << "\n" << sharedUserData->getImagePath() << "\n" << ui->EditDownloadFolder->text() << "\n" << ui->AutosaveSlider->value();
					}
				}
			}
			//Caso in cui l'utente abbia cambiato la foto profilo.
			else
			{
				changed = true;
				sharedUserData->setImage(QPixmap(photo));
				sharedUserData->setImagePath(photo);
				sharedUserData->setDefaultImage(false);
				stream << name << "\n" << photo << "\n" << ui->EditDownloadFolder->text() << "\n" << ui->AutosaveSlider->value();
			}
			/*FINE SEZIONE CRITICA.*/
			sharedUserDataMutex->unlock();
			//Chiusura del file e del widget corrente.
			file.close();
			close();
		}
	}
}

/*Slot che indica se la foto profilo è stata modificata o meno.*/
bool SettingsWindow::PhotoChanged()
{
	return changed;
}