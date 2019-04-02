#pragma once
//Qt.
#include <QPixmap>
#include <QString>

class UserData
{
	public:

		//Costruttori.
		UserData(QString name, QString ip)
		{
			this->name = name;
			this->ip = ip;
		};

		UserData(QString name, QString ip, QPixmap image, bool isDefault)
		{
			this->name = name;
			this->ip = ip;
			this->image = image;
			this->isDefault = isDefault;
		};

		//Metodo di prelievo del nome utente.
		QString getName() const
		{
			return name;
		};

		//Metodo di settaggio del nome utente.
		void setName(QString newName)
		{
			name = newName;
		};

		//Metodo di prelievo dell'ip: usata per il confronto dei messaggi ricevuti dagli utenti della rete.
		QString getIP() const
		{
			return ip;
		};

		//Metodo di settaggio dell'ip.
		void setIP(QString newIP)
		{
			ip = newIP;
		};

		//Metodo di prelievo della foto profilo dell'utente.
		QPixmap getImage() const
		{
			return image;
		};

		//Metodo di settaggio della foto profilo dell'utente.
		void setImage(QPixmap newImage)
		{
			image = newImage;
			isDefault = false;
		};

		//Metodo di prelievo del percorso della foto profilo dell'utente.
		QString getImagePath() const
		{
			return path;
		};

		//Metodo di settaggio del percorso della foto profilo dell'utente.
		void setImagePath(QString newPath)
		{
			path = newPath;
		};

		//Metodo che consente di modificare la variabile di notifica dell'uso della foto profilo di default o di una foto personalizzata. 
		void setDefaultImage(bool value)
		{
			isDefault = value;
		};

		//Metodo che consente di conoscere il valore della variabile di notifica dell'uso della foto profilo di default o di una foto personalizzata. 
		bool isDefaultImage() const
		{
			return isDefault;
		};

	private:
		QString name;
		QString ip;
		QPixmap image;
		QString path;
		bool isDefault = true;
};