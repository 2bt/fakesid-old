#include <string>


#ifdef __ANDROID__
#define ASSET_DIR ""
#else
#define ASSET_DIR "assets/"
#endif

namespace android {
    std::string get_storage_dir();
} // namespace