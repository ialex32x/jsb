#ifndef JAVASCRIPT_INDEX_H
#define JAVASCRIPT_INDEX_H

#include "jsb_macros.h"

namespace jsb::internal
{
    struct Index64
    {
        jsb_force_inline static const Index64& none()
        {
            static Index64 index = {};
            return index;
        }

        jsb_force_inline Index64():
            index(0),
            revision(0)
        {
        }

        jsb_force_inline Index64(int32_t i, int32_t r):
            index(i),
            revision(r)
        {
        }

        jsb_force_inline explicit Index64(int64_t p_value) :
            index((int32_t)(p_value >> 32)),
            revision((int32_t)(p_value & 0xffff))
        {
        }

        //TODO issue on 32-bit
        jsb_force_inline explicit Index64(void* p_value) :
            index((int32_t)((uintptr_t)p_value >> 32)),
            revision((int32_t)((uintptr_t)p_value & 0xffff))
        {
        }

        jsb_force_inline Index64(const Index64& other) = default;
        jsb_force_inline Index64(Index64&& other) = default;
        jsb_force_inline ~Index64() = default;

        jsb_force_inline explicit operator int64_t() const { return (((int64_t)index) << 32) | ((int64_t)revision); }
        jsb_force_inline explicit operator uint64_t() const { return (((uint64_t)index) << 32) | ((uint64_t)revision); }

        jsb_force_inline explicit operator int32_t() const { return index; }
        jsb_force_inline explicit operator uint32_t() const { return (uint32_t) index; }

        jsb_force_inline explicit operator void*() const
        {
            //TODO issue on 32-bit
            static_assert(sizeof(void*) == sizeof(uint64_t));
            return (void*) (uint64_t) *this;
        }

        jsb_force_inline bool is_valid() const { return revision != 0; }

        jsb_force_inline int32_t get_index() const { return index; }
        jsb_force_inline int32_t get_revision() const { return revision; }

        jsb_force_inline Index64& operator=(const Index64& other) = default;
        jsb_force_inline Index64& operator=(Index64&& other) = default;

        jsb_force_inline friend bool operator==(const Index64& lhs, const Index64& rhs)
        {
            return lhs.index == rhs.index && lhs.revision == rhs.revision;
        }

        jsb_force_inline friend bool operator!=(const Index64& lhs, const Index64& rhs)
        {
            return lhs.index != rhs.index || lhs.revision != rhs.revision;
        }

        jsb_force_inline friend uint32_t operator^(const Index64& lhs, const Index64& rhs)
        {
            return lhs.hash() ^ rhs.hash();
        }

        jsb_force_inline uint32_t hash() const
        {
            return index ^ (revision << 24);
        }

    private:
        int32_t index;
        int32_t revision;
    };

    struct Index32
    {
        constexpr static uint8_t kRevisionBits = 6;
        constexpr static uint32_t kRevisionMask = 0x3f;
        constexpr static uint32_t kFullMask = 0xffffffff;

        jsb_force_inline static const Index32& none()
        {
            static Index32 index = {};
            return index;
        }

        jsb_force_inline Index32(): packed_(0) {}

        jsb_force_inline Index32(int32_t index, int32_t revision):
            packed_(((uint32_t)index << kRevisionBits) | ((uint32_t)revision & kRevisionMask))
        {
            // index overflow check
            jsb_check(!((uint32_t) index >> (sizeof(uint32_t) * 8 - kRevisionBits)));
        }

        jsb_force_inline explicit Index32(uint32_t p_value) : packed_(p_value)
        {
        }

        jsb_force_inline operator uint32_t() const { return packed_; }

        // use lower 32 bits as packed data
        jsb_force_inline explicit Index32(void* p_value) :
            packed_((uint32_t)((uintptr_t)p_value & kFullMask))
        {
            // unexpected value check on the higher part
            if constexpr (sizeof(uintptr_t) > sizeof(uint32_t))
            {
                jsb_check(!((uintptr_t)p_value >> sizeof(uint32_t) * 8));
            }
        }

        jsb_force_inline explicit operator void*() const
        {
            return (void*)(uintptr_t) packed_;
        }

        jsb_force_inline Index32(const Index32& other) = default;
        jsb_force_inline Index32(Index32&& other) = default;
        jsb_force_inline Index32& operator=(const Index32& other) = default;
        jsb_force_inline Index32& operator=(Index32&& other) = default;
        jsb_force_inline ~Index32() = default;

        jsb_force_inline bool is_valid() const { return packed_ != 0; }

        jsb_force_inline int32_t get_index() const { return (int32_t) (packed_ >> kRevisionBits); }
        jsb_force_inline int32_t get_revision() const { return (int32_t) (packed_ & kRevisionMask); }

        jsb_force_inline friend bool operator==(const Index32& lhs, const Index32& rhs)
        {
            return lhs.packed_ == rhs.packed_;
        }

        jsb_force_inline friend bool operator!=(const Index32& lhs, const Index32& rhs)
        {
            return lhs.packed_ != rhs.packed_;
        }

        jsb_force_inline friend uint32_t operator^(const Index32& lhs, const Index32& rhs)
        {
            return lhs.hash() ^ rhs.hash();
        }

        jsb_force_inline uint32_t hash() const
        {
            return packed_;
        }

    private:
        uint32_t packed_;
    };
}
#endif
