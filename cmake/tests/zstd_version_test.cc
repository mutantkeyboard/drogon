#include <zstd.h>

int main()
{
    const char* version = ZSTD_versionString();
    unsigned version_number = ZSTD_versionNumber();
    return 0;
}