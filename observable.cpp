#include "observable.hpp"

std::vector<std::unique_ptr<IObservable>> ObservableManager::_observables;
