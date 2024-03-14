#ifndef JAVASCRIPT_POINTER_STORE_H
#define JAVASCRIPT_POINTER_STORE_H

#include "jsb_pch.h"

namespace jsb
{
    struct PointerStore
    {
        template<typename T>
        std::shared_ptr<T> access(void* p_runtime)
        {
            std::shared_ptr<T> rval;
            lock_.lock();
            if (all_runtimes_.has(p_runtime))
            {
                rval = ((T*) p_runtime)->shared_from_this();
            }
            lock_.unlock();
            return rval;
        }

        void add(void* p_runtime)
        {
            lock_.lock();
            jsb_check(!all_runtimes_.has(p_runtime));
            all_runtimes_.insert(p_runtime);
            lock_.unlock();
        }

        void remove(void* p_runtime)
        {
            lock_.lock();
            jsb_check(all_runtimes_.has(p_runtime));
            all_runtimes_.erase(p_runtime);
            lock_.unlock();
        }

        jsb_force_inline static PointerStore& get_shared()
        {
            static PointerStore global_store;
            return global_store;
        }

    private:
        BinaryMutex lock_;
        HashSet<void*> all_runtimes_;
    };

}

#endif
