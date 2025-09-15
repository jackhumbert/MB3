#pragma once

#include <cstdint>
#include <vector>
#include <map>
#include <memory>
#include <esp_heap_caps.h>
#include <esp32-hal-log.h>
#include <string>
#include <functional>
#include <cmath>

#define BYTE_CEILING(b) (((b - 1) / 8) + 1)

class ICanFrame;

/// @brief The base CAN data interface
class CanFrameTypes {
public:
    // static inline std::vector<std::shared_ptr<ICanFrame>> types;
    static inline std::map<uint32_t, std::shared_ptr<ICanFrame>> types;
};

/// @brief The base CAN signal interface
class ICanSignal {
public:
    ICanSignal() {
        
    }

    virtual size_t size() = 0;
    virtual void update(uint64_t new_value) = 0;
    virtual void * get_raw() = 0;
    virtual operator float() = 0;

    size_t offset = 0;
    std::string name;
    bool changed = false;
    ICanFrame * parent = nullptr;
};

class ICanFrame {
public:
    virtual void update() = 0;
    virtual size_t size() = 0;
    virtual uint32_t id() = 0;
    virtual uint8_t * data() = 0;
    virtual void members() = 0;
    virtual const std::string& name() = 0;
    virtual void update_from_member(ICanSignal&) = 0;
};

/// @brief A CAN frame base class
/// @tparam T derived type (for static access)
template<typename T>
class CanFrame : public ICanFrame {
    using CanFrameCallbackType = std::function<void(T&)>;
protected:
    static inline std::vector<std::shared_ptr<ICanSignal>> __members;

    size_t _size = 0;
    uint32_t _id = 0xFFFFFFFF;
    uint8_t * _data = nullptr;
    uint64_t last_data = 0;
    std::vector<std::shared_ptr<ICanSignal>> _members;
    std::string _name;
    bool allocated = false;

public:

    /// @brief The main updatable implementation
    /// @tparam Type What the member data should be stored as
    template <typename Type = uint8_t>
    class CanSignal : public ICanSignal {
    public:
        using CanSignalCallbackType = std::function<void(CanSignal&)>;

        CanSignal(size_t size = 1, float scale = 1.0, float offset = 0.0) : __size(size), _scale(scale), _offset(offset) {
            // T::CanSignal?
            __members.push_back(std::shared_ptr<CanSignal>(this));
            _raw = (uint8_t*)heap_caps_calloc(1, BYTE_CEILING(size), MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
            // _raw = (uint8_t*)heap_caps_calloc(1, BYTE_CEILING(size), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
            if (_raw == nullptr) {
                log_e("Error allocating %d bytes", BYTE_CEILING(size));
            }
        }

        CanSignal(const std::string& str) : CanSignal() {
            // log("CanSignal(str)");
            name = str;
        }

        // ~CanSignal() {
        //     if (_raw) {
        //         free(_raw);
        //     }
        // }

        /// @brief size in bits
        virtual size_t size() {
            return __size;
        }

        // Type& operator*() {
        //     return *(Type*)_raw;
        // }

        virtual operator float() override {
            return apply();
        }

        template <typename ReturnType>
        ReturnType get() {
            return *(ReturnType*)_raw;
        }

        CanSignal& operator=(const float& rhs) {
            auto new_value = ((rhs - _offset) / _scale);
            update(std::round(new_value));
            parent->update_from_member(*this);
            return *this;
        }

        CanSignal& operator=(const uint16_t& rhs) {
            update(rhs);
            parent->update_from_member(*this);
            return *this;
        }

        float apply() {
            return (((float)*(Type*)_raw) * _scale) + _offset;
        }

        // virtual void * get() {
        //     return _raw;
        // }

        virtual void * get_raw() {
            return (void*)_raw;
        }

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

        std::vector<CanSignalCallbackType> callbacks;

    private:
        // static constexpr size_t __size = Size;
        size_t __size;
        // uint8_t _raw[BYTE_CEILING(Size)];
        uint8_t * _raw = nullptr;
        float _scale = 1.0;
        float _offset = 0.0;
    };

    CanFrame(const std::string& str, uint32_t id) : _id(id), _name(str)  {
        // log("CanFrame()");
    }

    inline std::shared_ptr<ICanFrame> init() {
        // log("init()");
        size_t pos = 0;
        for (auto & member : __members) {
            member->offset = pos;
            member->parent = this;
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
        // _data = (uint8_t*)heap_caps_calloc(1, BYTE_CEILING(_size), MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
        _data = (uint8_t*)heap_caps_calloc(1, BYTE_CEILING(_size), MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
        if (_data != nullptr)
            allocated = true;
        else {
            log_e("Error allocating %s", _name.c_str());
        }
        return std::shared_ptr<ICanFrame>(this);
    }

    virtual void update() {
        static size_t offset;
        static size_t byte_off;
        static size_t bit_off;
        static size_t mem_size;

        // if (BYTE_CEILING(_size) == 1) {
        //     uint8_t d = *(uint8_t*)_data;
        //     if (d == last_data)
        //         return;
        //     last_data = d;
        // } else if (BYTE_CEILING(_size) <= 2) {
        //     uint16_t d = *(uint16_t*)_data;
        //     if (d == last_data)
        //         return;
        //     last_data = d;
        // } else if (BYTE_CEILING(_size) <= 4) {
        //     uint32_t d = *(uint32_t*)_data;
        //     if (d == last_data)
        //         return;
        //     last_data = d;
        // } else if (BYTE_CEILING(_size) <= 8) {
        //     uint64_t d = *(uint64_t*)_data;
        //     if (d == last_data)
        //         return;
        //     last_data = d;
        // }

        updated = true;

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

            // uint64_t p_byte = *(uint64_t*)&_data[byte_off];

            // member->update((p_byte >> member->offset) & ((1ULL << member->size()) - 1));
        }
        for (const auto & callback : callbacks) {
            callback(*(T*)this);
        }
        for (auto & member : _members) {
            member->changed = false;
        }
    }

    virtual void update_from_member(ICanSignal& member) {
        // uint64_t * ptr = (uint64_t*)_data;
        // *ptr &= ~(((1UL << member.size()) - 1) << member.offset);
        // *ptr |= *(uint64_t*)member.get_raw() << member.offset;

        size_t offset = member.offset;
        size_t byte_off = offset / 8;
        size_t bit_off = offset % 8;
        size_t mem_size = member.size();

        if ((mem_size + bit_off) <= 8) {
            uint8_t * p_byte = (uint8_t *)&_data[byte_off];
            auto raw = *(uint8_t*)member.get_raw();
            *p_byte &= ~(((1UL << mem_size) - 1) << bit_off);
            *p_byte |= (raw << bit_off);
        } else if ((mem_size + bit_off) <= 16) {
            uint16_t * p_byte = (uint16_t *)&_data[byte_off];
            auto raw = *(uint16_t*)member.get_raw();
            *p_byte &= ~(((1UL << mem_size) - 1) << bit_off);
            *p_byte |= (raw << bit_off);
        } else if ((mem_size + bit_off) <= 32) {
            uint32_t * p_byte = (uint32_t *)&_data[byte_off];
            auto raw = *(uint32_t*)member.get_raw();
            *p_byte &= ~(((1UL << mem_size) - 1) << bit_off);
            *p_byte |= (raw << bit_off);
        } else if ((mem_size + bit_off) <= 64) {
            uint64_t * p_byte = (uint64_t *)&_data[byte_off];
            auto raw = *(uint64_t*)member.get_raw();
            *p_byte &= ~(((1UL << mem_size) - 1) << bit_off);
            *p_byte |= (raw << bit_off);
        }

    }

    // size in bytes
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
        log_d("%d members", _members.size());
        for (auto const & member : _members) {
            log_d("* %02X: %s", member->offset, member->name.c_str());
        }
    }
    
    virtual const std::string& name() {
        return _name;
    }

    std::vector<CanFrameCallbackType> callbacks;
    bool updated = false;

private:
    // ~CanFrame() {
    //     // log("~CanFrame()");
    //     if (allocated) {
    //         free(_data);
    //     }
    // }
};