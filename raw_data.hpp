#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include <esp_heap_caps.h>
#include "log.hpp"
#include <string>
#include <functional>

#define BYTE_CEILING(b) (((b - 1) / 8) + 1)

class IUpdatable {
public:
    IUpdatable() {
        
    }

    virtual size_t size() = 0;
    virtual void update(uint64_t new_value) = 0;
    // virtual void * get() = 0;

    size_t offset = 0;
    std::string name;
    bool changed = false;
};

class IRawData {
public:
    virtual void update() = 0;
    virtual size_t size() = 0;
    virtual uint32_t id() = 0;
    virtual uint8_t * data() = 0;
    virtual void members() = 0;
    virtual const std::string& name() = 0;
};

template<typename T>
class RawData : public IRawData {
    using RawDataCallbackType = std::function<void(T&)>;
protected:
    static inline std::vector<std::shared_ptr<IUpdatable>> __members;

    size_t _size = 0;
    uint32_t _id = 0xFFFFFFFF;
    uint8_t * _data = nullptr;
    std::vector<std::shared_ptr<IUpdatable>> _members;
    std::string _name;
    bool allocated = false;

public:


    template <typename Type = uint8_t>
    class Updatable : public IUpdatable {
    public:
        using UpdatableCallbackType = std::function<void(Updatable&)>;

        Updatable(size_t size = 1, float scale = 1.0, float offset = 0.0) : __size(size), _scale(scale), _offset(offset) {
            // T::Updatable?
            __members.push_back(std::shared_ptr<Updatable>(this));
            _raw = (uint8_t*)heap_caps_calloc(1, BYTE_CEILING(size), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
            if (_raw == nullptr) {
                log("Error allocating %d bytes", BYTE_CEILING(size));
            }
        }

        Updatable(const std::string& str) : Updatable() {
            // log("Updatable(str)");
            name = str;
        }

        // ~Updatable() {
        //     if (_raw) {
        //         free(_raw);
        //     }
        // }

        virtual size_t size() {
            return __size;
        }

        Type& operator*() {
            return *(Type*)_raw;
        }

        float apply() {
            return ((*(Type*)_raw) * _scale) + _offset;
        }

        // virtual void * get() {
        //     return _raw;
        // }

        virtual void update(uint64_t new_value) {
            if (__size <= 8) {
                auto _new_value = (uint8_t)new_value;
                auto value = *(uint8_t*)_raw;
                if (_new_value != value) {
                    changed = true;
                    *(uint8_t*)_raw = _new_value;
                    // blog("%s updated: %02X", name.c_str(), _raw[0]);
                }
            } else if (__size <= 16) {
                auto _new_value = (uint16_t)new_value;
                auto value = *(uint16_t*)_raw;
                if (_new_value != value) {
                    changed = true;
                    *(uint16_t*)_raw = _new_value;
                    // blog("%s updated: %02X %02X", name.c_str(), _raw[0], _raw[1]);
                }
            } else if (__size <= 24) {
                auto _new_value = (uint32_t)new_value;
                auto value = *(uint32_t*)_raw;
                if (_new_value != value) {
                    changed = true;
                    *(uint32_t*)_raw = _new_value;
                    // blog("%s updated: %02X %02X %02X", name.c_str(), _raw[0], _raw[1], _raw[2]);
                }
            } else if (__size <= 32) {
                auto _new_value = (uint32_t)new_value;
                auto value = *(uint32_t*)_raw;
                if (_new_value != value) {
                    changed = true;
                    *(uint32_t*)_raw = _new_value;
                    // blog("%s updated: %02X %02X %02X", name.c_str(), _raw[0], _raw[1], _raw[2]);
                }
            }
            if (changed) {
                for (auto const & callback : callbacks) {
                    callback(*this);
                }
            }
        }

        std::vector<UpdatableCallbackType> callbacks;

    private:
        // static constexpr size_t __size = Size;
        size_t __size;
        // uint8_t _raw[BYTE_CEILING(Size)];
        uint8_t * _raw = nullptr;
        float _scale = 1.0;
        float _offset = 0.0;
    };

    RawData(const std::string& str, uint32_t id) : _name(str), _id(id) {
        // log("RawData()");
    }

    inline std::shared_ptr<IRawData> init() {
        // log("init()");
        size_t pos = 0;
        for (auto & member : __members) {
            member->offset = pos;
            static char tmp[6] = {0};
            if (member->name.empty()) {
                sprintf(tmp, "unk%02zX", pos);
                member->name = tmp;
                // log("Naming %s", member->name.c_str());
            } else {
                // log("Adding %s", member->name.c_str());
            }
            member->name = _name + "->" + member->name;
            pos += member->size();
            _members.emplace_back(member);
        }
        _size = pos;
        _data = (uint8_t*)heap_caps_calloc(1, BYTE_CEILING(_size), MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
        if (_data != nullptr)
            allocated = true;
        else {
            log("Error allocating %s", _name.c_str());
        }
        return std::shared_ptr<IRawData>(this);
    }

    virtual void update() {
        static size_t offset;
        static size_t byte_off;
        static size_t bit_off;
        static size_t mem_size;

        for (auto & member : _members) {
            offset = member->offset;
            byte_off = offset / 8;
            bit_off = offset % 8;
            mem_size = member->size();

            // log_d("Getting pointer for %s<%zu> at %02zX:%02zX(%zu) of %02zX/%zu", member->name.c_str(), mem_size, byte_off, bit_off, offset, _size, size());
            // vTaskDelay(100);

            uintptr_t p_byte = (uintptr_t)&_data[byte_off];

            if ((mem_size + bit_off) <= 8) {
                member->update((*(uint8_t*)p_byte >> bit_off) & ((1UL << mem_size) - 1));
            } else if ((mem_size + bit_off) <= 16) {
                member->update((*(uint16_t*)p_byte >> bit_off) & ((1UL << mem_size) - 1));
            } else if ((mem_size + bit_off) <= 32) {
                member->update((*(uint32_t*)p_byte >> bit_off) & ((1UL << mem_size) - 1));
            } else if ((mem_size + bit_off) <= 64) {
                member->update((*(uint64_t*)p_byte >> bit_off) & ((1UL << mem_size) - 1));
            }
        }
        for (const auto & callback : callbacks) {
            callback(*(T*)this);
        }
        for (auto & member : _members) {
            member->changed = false;
        }
    }

    virtual size_t size() {
        return BYTE_CEILING(_size);
    }

    virtual uint32_t id() {
        return _id;
    }

    virtual uint8_t * data() {
        return _data;
    }

    virtual void members() {
        log("%d members", _members.size());
        for (auto const & member : _members) {
            log("* %02X: %s", member->offset, member->name.c_str());
        }
    }
    
    virtual const std::string& name() {
        return _name;
    }

    std::vector<RawDataCallbackType> callbacks;

private:
    // ~RawData() {
    //     // log("~RawData()");
    //     if (allocated) {
    //         free(_data);
    //     }
    // }
};