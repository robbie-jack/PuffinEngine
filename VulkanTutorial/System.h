#pragma once

class System
{
public:

	virtual void Init() = 0;

	virtual bool Update(float dt) = 0;

	virtual void SendMessage() = 0;

	virtual ~System() {};

protected:
	bool running;
};