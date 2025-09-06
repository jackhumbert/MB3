#pragma once

#include <vector>
#include <memory>

/// @brief The base observable interface
class IObservable {
public:
    virtual ~IObservable() = default;
    virtual void update() = 0;
    virtual bool hasChanged() {
        if (_hasChanged) {
            _hasChanged = false;
            return true;
        } else {
            return false;
        }
    }

protected:
    bool _hasChanged;
};

class ObservableManager {
public:
    template <typename T>
    static void add(T * observable) {
        _observables.push_back(std::unique_ptr<T>(observable));
    }

    inline static void update() {
        for (auto &observable : _observables) {
            observable->update();
        }
    }

    static std::vector<std::unique_ptr<IObservable>> _observables;
};

/// @brief A variable manager that can be bound to a pointer
/// @tparam D The primitive display type for formatting, math
/// @tparam T The optional smart type @ref IUpdatable
template <typename D, typename T = D>
class TObservable : public IObservable {
public:
    using DisplayType = D;
    using DataType = T;

    TObservable(DataType * p_value) : p_value(p_value) {
        ObservableManager::add(this);
        update();
    }

    virtual ~TObservable() override = default;

    DisplayType newValue;

    virtual void update() override {
        newValue = computeDisplayValue(*p_value);
        _hasChanged = displayValue != newValue;
        displayValue = newValue;
    }

    virtual DisplayType computeDisplayValue(DisplayType input) {
        return input;
    }

    DisplayType& operator*() {
        return displayValue;
    }

    DisplayType get() const {
        return displayValue;
    }

    DataType * p_value;
    DisplayType displayValue = 0;
};

template <typename D, typename T = D>
class TAdjustedObservable : public TObservable<D, T> {
public:
    TAdjustedObservable(T * p_value, D scalar, D offset) : scalar(scalar), offset(offset), TObservable<D, T>(p_value) {
    
    }
    virtual ~TAdjustedObservable() override = default;

    virtual D computeDisplayValue(D input) override {
        return (input * scalar) + offset;
    }

    D scalar;
    D offset;
};