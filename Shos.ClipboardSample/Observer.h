#pragma once

#include <vector>

template <typename THint>
class Observer
{
public:
	virtual void Update(const THint& hint) = 0;
};

template <typename THint>
class Observable
{
	std::vector<Observer<THint>*> observers;

public:
	void Add(Observer<THint>& observer)
	{
		observers.push_back(&observer);
	}

	void Remove(Observer<THint>& observer)
	{
		std::remove(observers.begin(), observers.end(), &observer);
	}

	void Update(const THint& hint)
	{
		for (auto observer : observers)
			observer->Update(hint);
	}
};
