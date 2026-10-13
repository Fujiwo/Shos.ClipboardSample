#pragma once

#include <vector>

template <typename THint>
class Observer
{
public:
	virtual ~Observer() = default;

	virtual void Update(THint& hint) = 0;
};

template <typename THint>
class Observable
{
	std::vector<Observer<THint>*> observers;

public:
	void AddObserver(Observer<THint>& observer)
	{
		observers.push_back(&observer);
	}

	void RemoveObserver(Observer<THint>& observer)
	{
		std::erase(observers, &observer);
	}

	void Update(THint& hint)
	{
		for (auto* observer : observers)
			observer->Update(hint);
	}
};
