#pragma once
#include "UserData.hpp"

class UserDataComparator
{
	public:
		bool operator() (const UserData& user1, const UserData& user2) const
		{
			return  user1.getIP() < user2.getIP();
		}
};