#pragma once

#include <vector>
#include <memory>
#include <mb3/updatable.hpp>
#include <mb3/can.hpp>
 
/// @brief The base observable interface
class IObservable : public IUpdatable {
public:
    virtual ~IObservable() = default;
    virtual void update() { 
        _hasChanged = true;
    }
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
/// @tparam T The optional smart type @ref ICanSignal
/// @tparam I The input type for computeDisplayValue
template <typename D, typename T = D, typename I = D>
class TObservable : public IObservable {
public:
    using DisplayType = D;
    using DataType = T;
    using InputType = I;

    TObservable(DataType * p_value) : p_value(p_value) {
        ObservableManager::add(this);
        update();
        if constexpr (std::is_base_of<ICanSignal, T>::value) {
            auto signal = static_cast<ICanSignal*>(p_value);
            signal->callbacks.emplace_back(this);
        }
    }

    virtual ~TObservable() override = default;

    DisplayType newValue;

    virtual void update() override {
        newValue = computeDisplayValue((InputType)*p_value);
        _hasChanged |= displayValue != newValue;
        displayValue = newValue;
    }

    virtual DisplayType computeDisplayValue(InputType input) {
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

template <typename D, typename T = D, typename I = D>
class TAdjustedObservable : public TObservable<D, T, I> {
public:
    using DisplayType = D;
    using DataType = T;
    using InputType = I;

    TAdjustedObservable(DataType * p_value, DisplayType scalar, DisplayType offset) : TObservable<D, T, I>(p_value), scalar(scalar), offset(offset) {
    
    }
    virtual ~TAdjustedObservable() override = default;

    virtual DisplayType computeDisplayValue(InputType input) override {
        return (input * scalar) + offset;
    }

    DisplayType scalar;
    DisplayType offset;
};