#ifndef GEOSERVER_CUSTOM_STRUCTS_HPP
#define GEOSERVER_CUSTOM_STRUCTS_HPP

namespace geoserver_api
{
    template <typename T>
    struct data_clb_pointer
    {
        size_t size = 0;
        T* p = NULL;

        void reset(size_t start_idx=0)
        {
            if (size == 0) return;
            if (start_idx >= size)
            {
                fprintf(stderr, "Pointer reset index out of " \
                    "range: %ld, but size is: %ld", start_idx, size);
                return;
            }
            size_t reset_size = size - start_idx;
            if (p) memset(p + start_idx, 0, reset_size);
        }
    };
}

#endif