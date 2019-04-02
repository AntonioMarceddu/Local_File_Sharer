# Local File Sharer

<p align="center">
	Screenshot of the Local File Sharer program. Profile photos made with https://www.faceyourmanga.com/
   <img src="https://github.com/AntonioMarceddu/Local_File_Sharer/blob/master/LocalFileSharer/Resources/LocalFileSharer1.png"> 
</p>

Description available in english and italian below.

## English
Local File Sharer is a project realized by Antonio Marceddu and Mirko Crobu (https://github.com/MirCr) for the System Programming exam of the Polytechnic of Turin. It was created in C++ with the addition of the Qt framework, and, even though it was built in the Windows environment, is largely cross-platform and easily exportable to other operating systems.

It is a complete software for sharing files between two or more computers that are part of the same Local Area Network (LAN). The following features are made available to users:
- you can choose a username and a profile photo, with which each user will present himself to the other users of the local network;
- you can accept or reject any request to send files manually or you can activate the automatic saving function;
- several sending and receiving operations can be performed simultaneously, one for each individual user;
- for each operation, the percentage and estimated completion time is displayed on the screen;
- by going offline, you do not reveal to the other hosts your presence on the network and you have the possibility to send files to other users without them being able to do the same;
- both those who send and those who receive a file can choose to cancel the operation at any time;
- on Windows, by adding a special key in the system registry, it is possible to share a file by opening the contextual menu (by right-clicking) on ​​a file and choosing the item created ad hoc.

The project consists of the following files:
- main.cpp -> program entry point;
- LocalFileSharer.cpp/.hpp -> class that handles everything related to the main GUI (MainWindow.ui) and the tray icon;
- SettingsWindow.cpp/.hpp -> class that manages the program and user settings management screen (Settings.ui);
- UDPSenderThread.cpp/.hpp -> thread of communication of the name of the user to the other users of the local network;
- UDPReceiverThread.cpp/.hpp -> thread receiving names of other users present in the local network;
- TCPSendFileThread.cpp/.hpp -> thread sending the file chosen by the user to the selected user(s);
- TCPSendPhotoRunnable.cpp/.hpp -> runnable for sending the user's profile picture;
- TCPServer.cpp/.hpp -> manager for files and profile photos receiver threads;
- TCPReceiverThread.cpp/.hpp -> thread for receiving a single file or a single profile photo;
- SingleInstance.cpp/.hpp -> class of maintenance of a single instance of the program, necessary for the realization of the last feature of the program described above;
- ListUpdateSignalEmitter.cpp/.hpp -> on-screen list updater thread;
- UserData.hpp -> class containing all the information related to each user;
- UserDataComparator.hpp -> customized comparator for the UserData class;
- CommonNetworkImports.hpp -> header containing definitions common to network threads.

The code is entirely commented in Italian, so as to facilitate understanding for anyone who wants to learn the Qt framework or wants to make a branch of the project.

<p align="center">
	Screenshot of the Local File Sharer program. Profile photos made with https://www.faceyourmanga.com/
   <img src="https://github.com/AntonioMarceddu/Local_File_Sharer/blob/master/LocalFileSharer/Resources/LocalFileSharer2.png"> 
</p>

## Italiano
Local File Sharer è un progetto realizzato da Antonio Marceddu e Mirko Crobu (https://github.com/MirCr) per l'esame di Programmazione di Sistema del Politecnico di Torino. E' stato realizzato in C++ con l'aggiunta del framework Qt, e, anche se è stato realizzato in ambiente Windows, è in gran parte multipiattaforma e facilmente esportabile su altri sistemi operativi.

E' un software completo per la condivisione di file tra due o più computer che fanno parte della stessa rete locale (LAN). Vengono rese disponibili agli utenti le le seguenti features:
- si può scegliere uno username e una foto profilo, con cui ogni utente si presenterà agli altri utenti della rete locale;
- si può accettare o rifiutare ogni richiesta di invio file manualmente oppure si può attivare la funzione di salvataggio automatico;
- si possono effettuare più operazioni di invio e di ricezione contemporaneamente, uno per ogni singolo utente;
- per ogni operazione, viene visualizzata a schermo la percentuale e la stima del tempo di completamento;
- andando offline, non si rivela agli altri host la propria presenza nella rete e si ha la possibilità di inviare file agli altri utenti senza che questi possano fare altrettanto;
- sia chi invia che chi riceve un file può scegliere di annullare l'operazione in qualsiasi momento;
- su Windows, aggiungendo un'apposita chiave nel registro di sistema, è possibile condividere un file aprendo il menu contestuale (facendo click con il tasto destro) su un file e scegliendo la voce creata ad hoc. 

Il progetto è composto dai seguenti file:
- main.cpp -> punto di ingresso del programma;
- LocalFileSharer.cpp/.hpp -> classe che gestisce tutto ciò che riguarda la GUI principale (MainWindow.ui) e la tray icon;
- SettingsWindow.cpp/.hpp -> classe che gestisce la schermata di gestione delle impostazioni del programma e dell'utente (Settings.ui);
- UDPSenderThread.cpp/.hpp -> thread di comunicazione del nome dell'utente agli altri utenti della rete locale;
- UDPReceiverThread.cpp/.hpp -> thread di ricezione dei nomi degli altri utenti presenti nella rete locale;
- TCPSendFileThread.cpp/.hpp -> thread di invio del file scelto dall'utente verso l'utente/gli utenti selezionati;
- TCPSendPhotoRunnable.cpp/.hpp -> runnable di invio della foto profilo dell'utente;
- TCPServer.cpp/.hpp -> gestore dei thread di ricezione dei file e delle foto profilo;
- TCPReceiverThread.cpp/.hpp -> thread di ricezione di un singolo file o di una singola foto profilo;
- SingleInstance.cpp/.hpp -> classe di mantenimento di una singola istanza del programma, necessaria alla realizzazione dell'ultima feature del programma descritta sopra;
- ListUpdateSignalEmitter.cpp/.hpp -> thread di aggiornamento della lista a schermo;
- UserData.hpp -> classe contenente tutte le informazioni legate ad ogni utente;
- UserDataComparator.hpp -> comparatore personalizzato per la classe UserData;
- CommonNetworkImports.hpp -> header contenente definizioni comuni ai thread di rete.

Il codice è interamente commentato in italiano, così da facilitare la comprensione per chiunque voglia apprendere il framework Qt o voglia fare un branch del progetto.

<p align="center">
	Screenshot of the Local File Sharer program. Profile photos made with https://www.faceyourmanga.com/
   <img src="https://github.com/AntonioMarceddu/Local_File_Sharer/blob/master/LocalFileSharer/Resources/LocalFileSharer3.png"> 
</p>