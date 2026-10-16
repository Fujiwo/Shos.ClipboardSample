#pragma once

#include <vector>

// A minimal Observer pattern.
// Receives change notifications from an Observable.
template <typename THint>
class Observer
{
public:
	virtual ~Observer() = default;

	// Called by the observable when it has changed.
	virtual void Update(THint& hint) = 0;
};

// Notifies the registered observers of changes.
// The observers are not owned: an observer must be removed (or outlive the observable) before it is destroyed.
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

	// Notifies all the observers of the change.
	void Update(THint& hint)
	{
		for (auto* observer : observers)
			observer->Update(hint);
	}
};
