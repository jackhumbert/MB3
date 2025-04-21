#pragma once

#include <vector>
#include <memory>

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

template <typename T, typename D = T>
class TObservable : public IObservable {
public:
    TObservable(T * p_value) : p_value(p_value) {
        ObservableManager::add(this);
        update();
    }

    virtual ~TObservable() override = default;

    virtual void update() override {
        auto newValue = computeDisplayValue(*p_value);
        _hasChanged = displayValue != newValue;
        displayValue = newValue;
    }

    virtual D computeDisplayValue(T input) {
        return input;
    }

    D& operator*() {
        return displayValue;
    }

    // virtual void construct(const lv_obj_class_t * cls);
    // virtual void destruct(const lv_obj_class_t * cls);
    // virtual void event(const lv_obj_class_t * cls, lv_event_t * event);

    // static Observable * create(lv_obj_t * parent);
    // static void construct_cb(const lv_obj_class_t * cls, lv_obj_t * obj);
    // static void destruct_cb(const lv_obj_class_t * cls, lv_obj_t * obj);
    // static void event_cb(const lv_obj_class_t * cls, lv_event_t * event);

    T * p_value;
    D displayValue;
};

template <typename T, typename D = T>
class TAdjustedObservable : public TObservable<T, D> {
public:
    TAdjustedObservable(T * p_value, D scalar, D offset) : scalar(scalar), offset(offset), TObservable<T, D>(p_value) {
    
    }
    virtual ~TAdjustedObservable() override = default;

    virtual D computeDisplayValue(T input) override {
        return (input * scalar) + offset;
    }

    D scalar;
    D offset;
};