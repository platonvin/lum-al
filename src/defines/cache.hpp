#include "volk.h" //idk why but it is neither shipped with Linux Vulkan SDK nor bundled in vcpkg vulkan-sdk-components
#include <functional>
#include <vector>


// can be template like this

// template<typename ResourceType>
// class CachedResource {
//     ResourceType old_resource = nullptr; // Vulkan VK_NULL_HANDLE is nullptr

//     VkResult bind(ResourceType new_resource, auto bind_fn){
//         if(old_resource == new_resource){
//             // don't bind
//             return VK_SUCCESS;
//         } else {
//             VkResult res = bind_fn();
//             return res;
//         }
//     } 
// };

// but macros are more convinient

#define CONCAT_DETAIL(x, y) x##y
#define CONCAT(x, y) CONCAT_DETAIL(x, y)

#define _INIT_BIND_CACHED(__Resource) static decltype(__Resource) CONCAT(__OLD_RESOURCE, __LINE__) = nullptr;
// if previous bound resorce is the same then do not rebind to the same
// does not take arguments in count
#define BIND_CACHED(__Resource, __BIND_FUNCTION) _INIT_BIND_CACHED(__Resource) do {\
    if(CONCAT(__OLD_RESOURCE, __LINE__) != __Resource) {                           \
        __BIND_FUNCTION;                                                           \
        CONCAT(__OLD_RESOURCE, __LINE__) = __Resource;                             \
    } else {                                                                       \
        /*nothing*/                                                                \
    }                                                                              \
} while(0);
