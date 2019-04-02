#include "ListUpdateSignalEmitter.hpp"

void ListUpdateSignalEmitter::run()
{
	while (true)
	{
		//Il thread emetter� un nuovo segnale di aggiornamento della lista ogni 3*broadcastSendSleepTime millisecondi.
		msleep(3*broadcastSendSleepTime);
		emit UpdateList();
	}
}